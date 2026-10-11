// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/softphone/engine_host.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>
#include <softphone/core/settings_groups.hpp>
#include <softphone/media/mp3.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>

#include <exception>
#include <utility>

namespace softphone::plugins::softphone {

namespace {

[[nodiscard]] std::string groupText(const core::StoredConfiguration &stored, const char *name)
{
    const auto held = stored.owned.find(name);
    return held == stored.owned.end() ? std::string{} : held->second.value;
}

}

namespace {

std::optional<core::LogLevel> environmentLogLevel()
{
    return core::logLevelFromEnvironment();
}

}

EngineHost::EngineHost(bool useNullAudioDevice, std::optional<core::LogLevel> level,
                       std::filesystem::path configDirectory, std::filesystem::path dataDirectory,
                       std::string systemLanguage, core::ConfigDocument document)
    : configStore_{document.bound()
                       ? core::ConfigStore{std::move(document)}
                       : core::ConfigStore{configDirectory.empty() ? core::defaultConfigDirectory()
                                                                   : std::move(configDirectory)}}
{

    const bool given = !dataDirectory.empty();
    dataRoot_ = given ? std::move(dataDirectory) : core::dataDirectory();

    recordingsRoot_ = given ? dataRoot_ / "recordings" : core::recordingsDirectory();

    static_cast<void>(configStore_.load(stored_));
    knownAccounts_ = stored_.accounts;

    const auto diagnostics = core::diagnosticsFrom(groupText(stored_, "diagnostics"));
    const auto chosen = level ? *level : environmentLogLevel().value_or(diagnostics.level);
    core::Logger::instance().setLevel(chosen);
    core::Logger::instance().setSink(core::makeSink(
        given ? core::logFileIn(dataRoot_) : core::defaultLogFile(), diagnostics.systemLog));

    sip::PjsipOptions options;
    options.logLevel = core::pjsipLogLevelFor(chosen);
    options.useNullAudioDevice = useNullAudioDevice;

    backend_ = std::make_unique<sip::PjsipBackend>(loop_, *keeping_.store, options);
    engine_ = std::make_unique<core::Engine>(*backend_, loop_);

    engine_->setSelfRefresh(false);

    calls_ = std::make_unique<core::CallController>(
        *backend_, loop_, core::CallPolicy{}, [this](const core::CallInfo &call) {

            if (redial_) {
                redial_->onCallChanged(call);
            }

            if (recording_ && recordingArmed_) {
                recording_->callsChanged(calls_->calls());
            }
            if (callListener_) {
                callListener_(call);
            }
        });

    lamps_ = std::make_unique<core::BusyLampController>(
        *backend_,
        [this](const std::string &buttonId, core::LampState lamp) {
            if (lampListener_) {
                lampListener_(buttonId, lamp);
            }
        },
        [this](const std::string &number, const std::string &accountId) {
            const auto found =
                std::ranges::find(knownAccounts_, accountId, &core::AccountConfig::id);
            if (found == knownAccounts_.end()) {
                const core::LogAttribute fields[] = {{"line", accountId}};
                core::Logger::instance().log(
                    core::LogLevel::Warning, "sip",
                    "cannot complete " + number + ": no such line is known", fields);
                return std::string{};
            }
            return core::dialTarget(number, *found);
        });

    redial_ = std::make_unique<core::RedialController>(
        *calls_, loop_, core::RedialPolicy{}, [this](const core::RedialController::State &state) {
            if (redialListener_) {
                redialListener_(state);
            }
        });

    recordings_ = std::make_unique<storage::SqliteStore>();

    {
        auto content = core::appearanceFrom(groupText(stored_, "appearance")).language;
        if (content.empty()) {
            if (const auto *asked = std::getenv("SOFTPHONE_LANG"); asked != nullptr) {
                content = asked;
            }
        }
        if (content.empty()) {
            content = std::move(systemLanguage);
        }
        content = content.substr(0, std::min<std::size_t>(2, content.size()));
        for (auto &letter : content) {
            letter = static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
        }
        recordings_->setContentLanguage(content);
    }
    if (const auto status = recordings_->open(storage::databaseFileIn(dataRoot_));
        status != core::StoreStatus::Ok) {
        core::Logger::instance().log(
            core::LogLevel::Warning, "ui",
            "recordings will not be listed: " + std::string{toString(status)} + " (" +
                recordings_->lastError() + ")");
        recordings_.reset();
    }

    library_ = std::make_unique<core::RecordingLibrary>(recordings_.get(), recordingsRoot_);

    library_->setPolicy(core::recordingFrom(groupText(stored_, "recording")).retention);

    library_->setProber([](const std::filesystem::path &file) {
        core::RecordingLibrary::FileFacts facts;

        media::Mp3Decoder decoder;
        if (!media::succeeded(decoder.open(file))) {
            return facts;
        }

        const auto &info = decoder.info();
        facts.readable = true;
        facts.sampleRate = info.sampleRate;
        facts.channels = info.channels;
        facts.duration = std::chrono::milliseconds{
            info.sampleRate == 0 ? 0 : info.frames * 1000 / info.sampleRate};
        return facts;
    });

    recording_ = std::make_unique<core::RecordingController>(
        *backend_,
        [](const std::filesystem::path &file,
           const core::EncoderSettings &format) -> std::unique_ptr<core::IAudioEncoder> {
            auto encoder = std::make_unique<media::Mp3Encoder>();
            if (!core::succeeded(encoder->open(file, format))) {
                return nullptr;
            }
            return encoder;
        },
        recordingsRoot_, recordings_.get(), backend_.get());

    recording_->setRoomCheck([this]() { return library_->hasRoom(); });

    backend_->setCallObserver(calls_.get());

    for (const auto &account : stored_.accounts) {
        calls_->setAccountPolicy(account.id, core::callPolicyFor(account));
    }

}

EngineHost::~EngineHost()
{
    stop();
}

std::optional<std::string> EngineHost::start()
{
    try {
        backend_->start();
    } catch (const std::exception &error) {
        return std::string{error.what()};
    }

    backend_->setCallObserver(calls_.get());
    backend_->setTraceSink(traceSink_);

    loop_.restart();

    thread_ = std::thread{[this]() { loop_.run(); }};

    loop_.post([this]() {

        static_cast<void>(library_->finish());
        static_cast<void>(library_->recover());
        static_cast<void>(library_->sweep());
        armTheSweep();
    });

    return std::nullopt;
}

void EngineHost::stop()
{
    if (sweepTimer_ != core::IScheduler::TimerId{}) {
        loop_.cancel(sweepTimer_);
        sweepTimer_ = core::IScheduler::TimerId{};
    }

    loop_.post([this]() { stopPlayingHere(); });

    if (thread_.joinable()) {

        loop_.post([this]() {
            for (const auto &call : calls_->calls()) {
                calls_->hangUp(call.id);
            }
            engine_->stopAll();
        });
        loop_.post([this]() { loop_.stop(); });
        thread_.join();
    }

    if (backend_) {
        backend_->setCallObserver(nullptr);
        backend_->stop();
    }
}

void EngineHost::setStackLogLevel(core::LogLevel level)
{

    const auto pjsipLevel = core::pjsipLogLevelFor(level);

    sip::setStackLogLevel(pjsipLevel);
}

void EngineHost::setTraceSink(core::ISipTraceSink *sink) noexcept
{

    traceSink_ = sink;
    backend_->setTraceSink(sink);
}

void EngineHost::pollMedia(std::function<void(std::vector<core::CallMedia>)> report)
{
    if (!report) {
        return;
    }

    loop_.post([this, report = std::move(report)]() {
        std::vector<core::CallMedia> media;
        for (const auto &call : calls_->calls()) {
            core::CallMedia entry;
            entry.callId = call.id;
            entry.number = core::sipUserPart(call.remoteUri);
            entry.name = call.remoteDisplayName;
            entry.state = call.state;
            entry.direction = call.direction;

            if (call.connectedAt.has_value()) {
                entry.duration = std::max(std::chrono::seconds{0},
                                          std::chrono::duration_cast<std::chrono::seconds>(
                                              scheduler().now() - *call.connectedAt));
            }

            if (auto stats = backend_->callMediaStats(call.id)) {
                entry.hasMedia = true;
                entry.stats = std::move(*stats);
            }

            media.push_back(std::move(entry));
        }
        report(std::move(media));
    });
}

void EngineHost::setCallListener(core::CallController::CallCallback listener)
{
    callListener_ = std::move(listener);
}

void EngineHost::setRedialListener(core::RedialController::StateCallback listener)
{
    redialListener_ = std::move(listener);
}

void EngineHost::setLampListener(core::BusyLampController::StateCallback listener)
{
    lampListener_ = std::move(listener);
}

void EngineHost::setKnownAccounts(std::vector<core::AccountConfig> accounts)
{
    knownAccounts_ = std::move(accounts);

}

const core::AccountConfig *EngineHost::accountById(const std::string &accountId) const
{
    const auto found = std::ranges::find(knownAccounts_, accountId, &core::AccountConfig::id);
    return found == knownAccounts_.end() ? nullptr : &*found;
}

void EngineHost::startTone(const std::string &wavPath, bool loop, core::ToneOutput output,
                           core::ToneLevel level)
{
    backend_->startTone(wavPath, loop, output, level);
}

void EngineHost::setToneEndedCallback(std::function<void()> callback)
{
    backend_->setToneEndedCallback(std::move(callback));
}

void EngineHost::stopTone()
{
    backend_->stopTone();
}

void EngineHost::armRecording(bool armed)
{
    post([this, armed]() {
        if (recordingArmed_ == armed) {
            return;
        }
        recordingArmed_ = armed;

        if (!armed && recording_) {
            recording_->stopAll();
        }
    });
}

void EngineHost::armLamps(bool armed)
{
    post([this, armed]() {
        if (lampsArmed_ == armed) {
            return;
        }
        lampsArmed_ = armed;
        applyLamps();
    });
}

void EngineHost::setButtons(std::vector<core::PhoneButton> buttons, std::string dialAccountId)
{
    wantedButtons_ = std::move(buttons);
    wantedDialAccount_ = std::move(dialAccountId);
    applyLamps();
}

void EngineHost::applyLamps()
{
    if (!lamps_) {
        return;
    }

    if (!lampsArmed_) {
        lamps_->setButtons({}, {});
        return;
    }
    lamps_->setButtons(wantedButtons_, wantedDialAccount_);
}

void EngineHost::post(std::function<void()> work)
{
    loop_.post(std::move(work));
}

secrets::SecretStatus EngineHost::storedPasswordStatus() const
{
    const auto lines = enabledAccounts(stored_);
    if (lines.empty()) {
        return secrets::SecretStatus::NotFound;
    }

    const auto name = core::credentialName(lines.front());
    if (name.empty()) {
        return secrets::SecretStatus::NotFound;
    }

    std::string password;
    return keeping_.store->fetch(name, password);
}

secrets::SecretStatus EngineHost::rememberPassword(const core::AccountConfig &account,
                                                   const std::string &password)
{
    const auto name = core::credentialName(account);
    if (name.empty()) {

        return secrets::SecretStatus::Failed;
    }
    return keeping_.store->store(name, password);
}

secrets::SecretStatus EngineHost::forgetPassword(const core::AccountConfig &account)
{
    const auto name = core::credentialName(account);
    if (name.empty()) {
        return secrets::SecretStatus::NotFound;
    }
    return keeping_.store->erase(name);
}

secrets::SecretStatus EngineHost::refilePassword(const Refiling &move)
{
    std::string password;
    if (const auto status = keeping_.store->fetch(move.from, password);
        status != secrets::SecretStatus::Ok) {
        return status;
    }

    if (const auto status = keeping_.store->store(move.to, password);
        status != secrets::SecretStatus::Ok) {
        return status;
    }

    return keeping_.store->erase(move.from);
}

void EngineHost::armTheSweep()
{

    sweepTimer_ = loop_.scheduleAfter(std::chrono::hours{6}, [this]() {
        static_cast<void>(library_->sweep());
        stopRecordingIfTheDiskIsFull();
        armTheSweep();
    });
}

void EngineHost::stopRecordingIfTheDiskIsFull()
{
    if (!recording_ || !recording_->recording() || library_->hasRoom()) {
        return;
    }

    core::Logger::instance().log(core::LogLevel::Warning, "recording",
                                 "stopping the recording: the disk is down to its last "
                                 "reserve and the phone will not fill it");
    recording_->stopAll();
}

void EngineHost::applyRetention(const core::RetentionPolicy &retention)
{

    if (library_->policy() != retention) {
        library_->setPolicy(retention);
        static_cast<void>(library_->sweep());
    }
}

void EngineHost::setRecordingWrittenListener(std::function<void(const core::Recording &)> written)
{
    if (!recording_) {
        return;
    }
    recording_->setWrittenListener(std::move(written));
}

void EngineHost::setRecordingAskListener(std::function<void(std::string)> ask)
{
    if (!recording_) {
        return;
    }
    recording_->setAskListener([ask = std::move(ask)](const std::string &callId) {
        if (ask) {
            ask(callId);
        }
    });
}

void EngineHost::setRecordingRefusalListener(
    std::function<void(std::string, core::EncoderStatus)> told)
{
    recordingRefused_ = std::move(told);
}

void EngineHost::startRecording(const std::string &callId)
{
    if (!recording_) {
        return;
    }

    if (const auto status = recording_->start(callId, calls_->calls()); !core::succeeded(status)) {
        const core::LogAttribute fields[] = {{"call", callId}};
        core::Logger::instance().log(
            core::LogLevel::Warning, "ui",
            "the recording would not start: " + std::string{core::toString(status)}, fields);
        if (recordingRefused_) {
            recordingRefused_(callId, status);
        }
    }
}

void EngineHost::setPlaybackFailureListener(std::function<void(std::filesystem::path)> told)
{
    playbackFailed_ = std::move(told);
}

void EngineHost::setPlaybackListener(std::function<void(Playback)> listener)
{
    playbackListener_ = std::move(listener);
}

void EngineHost::publishPlayback(bool ended)
{
    if (!playbackListener_) {
        return;
    }

    Playback where;
    where.playing = playing_;
    where.positionMs = stream_ ? stream_->positionMs() : 0;
    where.durationMs = stream_ ? stream_->durationMs() : 0;
    where.ended = ended;
    where.file = playing_ || stream_ ? playingFile_ : std::filesystem::path{};
    playbackListener_(where);
}

void EngineHost::armThePlayhead()
{

    playbackTimer_ = loop_.scheduleAfter(std::chrono::milliseconds{50}, [this]() {
        playbackTimer_ = core::IScheduler::TimerId{};
        if (!stream_) {
            return;
        }

        if (stream_->finished()) {

            backend_->playFrom(nullptr);
            playing_ = false;

            stream_->seek(0);

            publishPlayback(true);
            return;
        }

        publishPlayback(false);
        armThePlayhead();
    });
}

void EngineHost::playRecording(const std::filesystem::path &file, bool start)
{
    loop_.post([this, file, start]() {
        stopPlayingHere();

        playingFile_ = file;
        const unsigned rate = backend_->playbackRate();
        stream_ = std::make_unique<media::Mp3Stream>(file, rate == 0 ? 16000u : rate);
        if (!stream_->start()) {
            core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                         "the recording would not open: " + core::asUtf8(file));
            stream_.reset();
            publishPlayback(false);

            if (playbackFailed_) {
                playbackFailed_(file);
            }
            return;
        }

        stream_->setSpeed(wantedSpeed_);
        stream_->setChannels(wantedNear_, wantedFar_);

        if (!start) {

            playing_ = false;
            publishPlayback(false);
            return;
        }

        backend_->playFrom(stream_.get());
        playing_ = true;
        publishPlayback(false);
        armThePlayhead();
    });
}

void EngineHost::stopPlayingHere()
{
    if (playbackTimer_ != core::IScheduler::TimerId{}) {
        loop_.cancel(playbackTimer_);
        playbackTimer_ = core::IScheduler::TimerId{};
    }

    if (backend_) {
        backend_->playFrom(nullptr);
    }
    stream_.reset();
    playing_ = false;
    playingFile_.clear();
}

void EngineHost::stopPlaying()
{
    loop_.post([this]() {
        stopPlayingHere();
        publishPlayback(false);
    });
}

void EngineHost::pausePlaying(bool paused)
{
    loop_.post([this, paused]() {
        if (!stream_) {
            return;
        }

        if (paused) {
            if (playbackTimer_ != core::IScheduler::TimerId{}) {
                loop_.cancel(playbackTimer_);
                playbackTimer_ = core::IScheduler::TimerId{};
            }
            backend_->playFrom(nullptr);
            playing_ = false;
        } else {
            backend_->playFrom(stream_.get());
            playing_ = true;
            armThePlayhead();
        }
        publishPlayback(false);
    });
}

void EngineHost::seekPlayback(std::uint64_t positionMs)
{
    loop_.post([this, positionMs]() {
        if (!stream_) {
            return;
        }

        const bool wasPlaying = playing_;
        backend_->playFrom(nullptr);
        stream_->seek(positionMs);
        if (wasPlaying) {
            backend_->playFrom(stream_.get());
        }
        publishPlayback(false);
    });
}

void EngineHost::setPlaybackSpeed(double speed)
{
    loop_.post([this, speed]() {
        wantedSpeed_ = speed;
        if (stream_) {
            stream_->setSpeed(speed);
        }
    });
}

void EngineHost::setPlaybackChannels(bool nearSide, bool farSide)
{
    loop_.post([this, nearSide, farSide]() {
        wantedNear_ = nearSide;
        wantedFar_ = farSide;
        if (stream_) {
            stream_->setChannels(nearSide, farSide);
        }
    });
}

core::RecordingFeed *EngineHost::listenTo(const std::string &sessionId,
                                          const std::vector<std::string> &callIds)
{
    if (backend_ == nullptr) {
        return nullptr;
    }

    auto &feed = backend_->openRecording(sessionId);

    feed.openListening();

    backend_->recordCalls(sessionId, callIds, false);
    return &feed;
}

void EngineHost::stopListening(const std::string &sessionId)
{
    if (backend_ == nullptr) {
        return;
    }
    backend_->recordCalls(sessionId, {}, false);
    backend_->closeRecording(sessionId);
}

}
