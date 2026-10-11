// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/capture/controller.hpp>

#include <softphone/capture/applications.hpp>
#include <softphone/capture/echo_encoder.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/recording_controller.hpp>
#include <softphone/core/uuid.hpp>

#include <algorithm>
#include <string_view>
#include <system_error>
#include <utility>

namespace softphone::plugins::capture {
namespace {

constexpr unsigned kRate = 16000;

constexpr unsigned kFrame = kRate / 50;

}

CaptureController::CaptureController(cap::ISystemCapture *capture, EncoderMaker maker,
                                     std::filesystem::path root, core::IRecordingStore *store,
                                     std::unique_ptr<cap::IConversationDetector> detector)

    : detector_{detector ? std::move(detector) : cap::makeConversationDetector()},
      capture_{capture}, maker_{std::move(maker)}, root_{std::move(root)}, store_{store}
{

    drawings_.setChanged([this]() {
        const std::lock_guard guard{mutex_};
        if (session_) {
            session_->drawInto(drawings_.watching() ? &drawings_.feed() : nullptr);
        }
    });
}

CaptureController::~CaptureController()
{

    watchingOn_.store(false, std::memory_order_release);
    watchWake_.notify_all();
    if (watching_.joinable()) {
        watching_.join();
    }

    static_cast<void>(stopAndTell());
}

void CaptureController::setSettings(const Settings &settings)
{
    bool stopNow = false;
    {
        const std::lock_guard guard{mutex_};
        settings_ = settings;

        stopNow = running_ && !settings_.enabled;
    }
    if (stopNow) {
        static_cast<void>(stopAndTell());
    }
    {
        const std::lock_guard guard{mutex_};
        armIfWanted();
    }
    applyDetector();
    announce();
}

void CaptureController::armIfWanted()
{
    if (capture_ == nullptr) {
        return;
    }

    const bool watching =
        settings_.onFinding != OnFinding::Never ||
        std::ranges::any_of(settings_.perApplication, [](const ApplicationPolicy &one) {
            return one.answer != OnFinding::Never;
        });

    const bool wanted = settings_.enabled && !suppressed_ && !yielding_ &&
                        (watching || settings_.preRollSeconds > 0);

    wantsDetector_ = settings_.enabled && watching;

    if (!wanted) {
        if (session_ && !running_) {
            session_->disarm();
            session_.reset();
        }
        return;
    }

    problem_ = openTheDevices();
    if (problem_ != cap::CaptureError::None) {
        session_.reset();
    }
}

cap::CaptureError CaptureController::openTheDevices()
{

    if (!session_) {
        session_ = std::make_unique<cap::CaptureSession>(*capture_, encoderFactory());
    }
    if (session_->armed()) {
        return cap::CaptureError::None;
    }

    cap::CaptureRequest request;
    request.sampleRate = kRate;
    request.frameSamples = kFrame;
    request.outputDevice = settings_.outputDevice;
    request.inputDevice = settings_.inputDevice;

    session_->drawInto(drawings_.watching() ? &drawings_.feed() : nullptr);

    return session_->arm(request, std::chrono::seconds{settings_.preRollSeconds});
}

void CaptureController::applyDetector()
{
    const std::lock_guard guard{detectorMutex_};

    cap::DetectorSettings watching;
    bool wanted = false;
    {
        const std::lock_guard held{mutex_};
        wanted = wantsDetector_;
        watching.excluded = neverReport(settings_.onFinding, settings_.perApplication);
        watching.ownProcessId = settings_.ownProcessId;

        watching.conversing = running_;
    }

    if (!wanted) {
        if (detector_ && detecting_) {
            detector_->stop();
            detecting_ = false;
        }
        return;
    }

    if (!detector_->available()) {
        return;
    }

    if (detecting_) {
        return;
    }

    detecting_ = true;
    static_cast<void>(detector_->start(
        watching,
        [this](const cap::Conversation &what) {

            if (suppressedNow()) {
                sayWhy("a conversation begins, and this phone is on a call, so it is left alone");
                return;
            }

            PhraseSource words;
            {
                const std::lock_guard held{mutex_};
                words = meetingPhrase_;
            }
            const auto phrase = words ? words() : std::string{};

            auto name = cap::nameFor(what.application, what.title);
            if (const auto venue = cap::venueOf(what.application);
                !venue.empty() && !phrase.empty()) {
                const auto at = phrase.find("%1");
                name = at == std::string::npos
                           ? phrase
                           : phrase.substr(0, at) + std::string{venue} + phrase.substr(at + 2);
            }
            sayWhy("a conversation begins");
            if (name.empty()) {
                name = "Another application";
            }

            OnFinding answer = OnFinding::Never;
            FoundCallback ask;
            {
                const std::lock_guard held{mutex_};
                answer = policyFor(settings_.onFinding, settings_.perApplication, what.application);
                ask = found_;
            }

            core::Logger::instance().log(core::LogLevel::Info, "capture",
                                         std::string{"a conversation in "} + what.application +
                                             ", and the answer for it is " +
                                             std::string{asText(answer)});

            if (answer == OnFinding::Record) {
                static_cast<void>(start(std::move(name), what.application));
                return;
            }
            if (answer == OnFinding::Ask && ask) {

                ask(std::move(name), what.application);
            }
        },
        [this]() {
            sayWhy("a conversation ends");
            LostCallback gone;
            {
                const std::lock_guard held{mutex_};
                gone = lost_;
            }

            if (gone) {
                gone();
            }
            static_cast<void>(stopAndTell());
        }));
}

void CaptureController::watchWith(const cap::Endings &when)
{
    const std::lock_guard guard{mutex_};
    endings_ = when;
}

void CaptureController::sayWhy(std::string_view what) const
{

    if (detector_ == nullptr) {
        return;
    }
    auto seen = detector_->trail();
    if (seen.empty()) {
        return;
    }
    core::Logger::instance().log(core::LogLevel::Debug, "capture",
                                 std::string{what} + ", and this is what was seen:\n" + seen);
}

CaptureController::Settings CaptureController::settings() const
{
    const std::lock_guard guard{mutex_};
    return settings_;
}

void CaptureController::setWritten(WrittenCallback written)
{
    const std::lock_guard guard{mutex_};
    written_ = std::move(written);
}

void CaptureController::setExcluded(ExcludedCallback excluded)
{
    const std::lock_guard guard{mutex_};
    excludedChanged_ = std::move(excluded);
}

void CaptureController::setFound(FoundCallback found)
{
    const std::lock_guard guard{mutex_};
    found_ = std::move(found);
}

void CaptureController::setLost(LostCallback lost)
{
    const std::lock_guard guard{mutex_};
    lost_ = std::move(lost);
}

void CaptureController::setMeetingPhrase(PhraseSource words)
{
    const std::lock_guard guard{mutex_};
    meetingPhrase_ = std::move(words);
}

bool CaptureController::notACall(const std::string &recordingId)
{
    ExcludedCallback tell;
    std::vector<ApplicationPolicy> wanted;
    {
        const std::lock_guard guard{mutex_};
        if (store_ == nullptr) {
            return false;
        }

        core::Recording row;
        if (!core::succeeded(store_->recording(recordingId, row))) {
            return false;
        }

        if (row.source != core::RecordingSource::External || row.remoteNumber.empty()) {
            return false;
        }

        const auto plain = cap::plainly(row.remoteNumber);
        const auto found =
            std::ranges::find_if(settings_.perApplication, [&plain](const ApplicationPolicy &one) {
                return cap::plainly(one.process) == plain;
            });
        if (found == settings_.perApplication.end()) {
            settings_.perApplication.push_back(
                ApplicationPolicy{row.remoteNumber, OnFinding::Never});
        } else {
            found->answer = OnFinding::Never;
        }
        wanted = settings_.perApplication;
        tell = excludedChanged_;

        std::vector<std::string> files;
        static_cast<void>(store_->removeRecording(recordingId, files));
        for (const auto &file : files) {
            std::error_code ignored;
            std::filesystem::remove(root_ / file, ignored);
        }

        armIfWanted();
    }
    applyDetector();
    announce();
    if (tell) {
        tell(std::move(wanted));
    }
    return true;
}

void CaptureController::setListener(StateCallback listener)
{
    const std::lock_guard guard{mutex_};
    listener_ = std::move(listener);
}

void CaptureController::suppress(bool suppressed)
{
    {
        const std::lock_guard guard{mutex_};
        if (suppressed_ == suppressed) {
            return;
        }
        suppressed_ = suppressed;
    }

    {
        const std::lock_guard guard{mutex_};
        armIfWanted();
    }
    applyDetector();
    announce();
}

void CaptureController::yieldTheSoundCard(bool wanted)
{
    bool moved = false;
    bool halfWay = false;
    {
        const std::lock_guard guard{mutex_};
        if (cardWanted_ == wanted) {
            return;
        }
        cardWanted_ = wanted;

        const auto must = capture_ != nullptr ? capture_->mustYieldTheSoundCard()
                                              : cap::ISystemCapture::Yields::Nothing;
        halfWay = must == cap::ISystemCapture::Yields::WhatTheMachinePlays;
        onlyTheTapGivesWay_ = halfWay;

        const bool yielding = wanted && must == cap::ISystemCapture::Yields::Everything;
        moved = yielding != yielding_;
        yielding_ = yielding;
    }

    if (halfWay && capture_ != nullptr) {
        capture_->holdOffWhatTheMachinePlays(wanted);
    }

    if (!moved) {

        announce();
        return;
    }

    {
        const std::lock_guard guard{mutex_};
        armIfWanted();
    }
    applyDetector();
    announce();
}

bool CaptureController::suppressedNow() const
{

    const std::lock_guard guard{mutex_};
    return suppressed_;
}

bool CaptureController::suppressed() const
{
    const std::lock_guard guard{mutex_};
    return suppressed_;
}

cap::CaptureError CaptureController::start(std::string what, std::string process)
{
    core::Recording row;
    {
        const std::lock_guard guard{mutex_};
        if (capture_ == nullptr) {
            problem_ = cap::CaptureError::NoBackend;
            return problem_;
        }
        if (!settings_.enabled || running_) {

            problem_ = cap::CaptureError::Refused;
            return problem_;
        }

        const auto now = std::chrono::system_clock::now();
        row.id = core::makeUuid();
        row.source = core::RecordingSource::External;
        row.startedAt = now;
        row.filePath = core::RecordingController::relativePath(row.id, now);
        row.sampleRate = kRate;
        row.channels = 2;

        row.consent = core::ConsentPosture::Silent;

        row.remoteName = std::move(what);
        row.remoteNumber = std::move(process);

        pending_ = root_ / row.filePath;

        if (const auto opened = openTheDevices(); opened != cap::CaptureError::None) {
            session_.reset();
            problem_ = opened;
            return problem_;
        }

        if (const auto opened = session_->beginRecording(); !core::succeeded(opened)) {
            problem_ = cap::CaptureError::Refused;
            return problem_;
        }

        problem_ = cap::CaptureError::None;
        running_ = true;
        row_ = row;

        gone_ = Gone{};

        row_.unfinished = true;
        if (store_ != nullptr) {
            static_cast<void>(store_->save(row_));
        }

        if (!watchingOn_.exchange(true, std::memory_order_acq_rel)) {
            watching_ = std::thread{[this] { watch(); }};
        }
    }
    announce();
    return cap::CaptureError::None;
}

void CaptureController::watch()
{
    using namespace std::chrono_literals;

    while (watchingOn_.load(std::memory_order_acquire)) {
        {
            std::unique_lock sleeping{watchSleeping_};
            watchWake_.wait_for(sleeping, 1s,
                                [this] { return !watchingOn_.load(std::memory_order_acquire); });
        }
        if (!watchingOn_.load(std::memory_order_acquire)) {
            return;
        }

        auto ending = cap::Ending::Nothing;
        std::string what;
        std::string process;
        std::chrono::milliseconds quietly{0};

        struct Remark {
            bool gone{false};
            bool back{false};
            std::chrono::milliseconds since{0};
        };

        Remark nearSide;
        Remark farSide;

        {
            const std::lock_guard guard{mutex_};
            if (!running_ || !session_) {
                continue;
            }

            const auto now = std::chrono::steady_clock::now();
            const auto look = [this, now](cap::Side side, bool &reported, Remark &remark) {
                const auto since = session_->sinceDelivery(side, now);
                if (since >= endings_.gone) {
                    if (!reported) {
                        reported = true;
                        remark.gone = true;
                        remark.since = since;
                    }
                } else if (reported) {
                    reported = false;
                    remark.back = true;
                }
            };
            look(cap::Side::Near, gone_.near, nearSide);
            look(cap::Side::Far, gone_.far, farSide);

            quietly = session_->quietFor();
            ending = cap::endingFor(endings_, session_->elapsed(), quietly);
            what = row_.remoteName;
            process = row_.remoteNumber;
        }

        const auto tell = [](std::string_view which, const Remark &remark) {
            if (remark.gone) {
                core::Logger::instance().log(
                    core::LogLevel::Warning, "capture",
                    std::string{which} + " has carried nothing for " +
                        std::to_string(remark.since.count() / 1000) +
                        " seconds; what it should have carried is silence in the file");
            } else if (remark.back) {
                core::Logger::instance().log(core::LogLevel::Info, "capture",
                                             std::string{which} + " is arriving again");
            }
        };
        tell("the microphone", nearSide);
        tell("what this machine is playing", farSide);

        if (ending == cap::Ending::Nothing) {
            continue;
        }

        core::Logger::instance().log(
            core::LogLevel::Info, "capture",
            ending == cap::Ending::Roll
                ? std::string{"this capture has run to the ceiling; closing it and opening another"}
                : std::string{"neither side of this capture has carried anything for "} +
                      std::to_string(quietly.count() / 1000) +
                      " seconds; closing it — see CaptureSession::quietFor");

        static_cast<void>(stopAndTell());

        if (ending == cap::Ending::Roll) {
            static_cast<void>(start(std::move(what), std::move(process)));
        }
    }
}

CaptureController::EncoderMaker CaptureController::encoderFactoryMaker() const
{
    return maker_;
}

core::ConversationRecorder::EncoderFactory CaptureController::encoderFactory()
{
    return [this](const std::string &leg) -> std::unique_ptr<core::IAudioEncoder> {

        if (!leg.empty()) {
            return std::unique_ptr<core::IAudioEncoder>{};
        }
        auto below = maker_(pending_, core::EncoderSettings{kRate, 2, 32});
        if (!below) {
            return std::unique_ptr<core::IAudioEncoder>{};
        }

        return std::unique_ptr<core::IAudioEncoder>{std::make_unique<cap::EchoCancellingEncoder>(
            std::move(below), cap::makeEchoCanceller(kRate, kFrame))};
    };
}

void CaptureController::stop()
{
    static_cast<void>(stopAndTell());
}

core::Recording CaptureController::stopAndTell()
{
    core::Recording written;
    WrittenCallback tell;
    {
        const std::lock_guard guard{mutex_};
        if (!running_ || !session_) {
            return {};
        }
        running_ = false;

        const auto outcome = session_->endRecording();

        row_.duration = outcome.duration;
        row_.bytes = outcome.bytes;
        row_.hasGaps = outcome.hasGaps();

        row_.unfinished = false;

        if (store_ != nullptr) {

            core::Recording stored;
            if (core::succeeded(store_->recording(row_.id, stored))) {
                core::keepWhatAPersonChanged(stored, row_);
            }
            static_cast<void>(store_->save(row_));
        }
        written = row_;
        tell = written_;
        row_ = {};

        armIfWanted();
    }
    applyDetector();
    announce();

    if (tell) {
        tell(written);
    }
    return written;
}

bool CaptureController::rename(std::string what)
{
    std::string called;
    std::string row;
    {
        const std::lock_guard guard{mutex_};
        if (!running_ || what.empty()) {
            return false;
        }
        row_.remoteName = std::move(what);
        called = row_.remoteName;
        row = row_.id;

        if (store_ != nullptr) {
            static_cast<void>(store_->save(row_));
        }
    }

    {
        const core::LogAttribute fields[] = {{"name", called}, {"recording", row}};
        core::Logger::instance().log(core::LogLevel::Info, "capture",
                                     "the conversation being recorded was renamed", fields);
    }

    announce();
    return true;
}

CaptureController::State CaptureController::state() const
{
    const std::lock_guard guard{mutex_};
    State state;
    state.available = capture_ != nullptr;
    state.watching = detector_ && detector_->available();
    state.enabled = settings_.enabled;
    state.running = running_;
    state.problem = problem_;

    state.heldOff = !running_ && (suppressed_ || (cardWanted_ && !onlyTheTapGivesWay_));
    state.what = row_.remoteName;
    state.elapsed = session_ ? session_->elapsed() : std::chrono::milliseconds{0};

    state.recordingId = running_ ? row_.id : std::string{};
    return state;
}

core::RecordingFeed *CaptureController::listenTo()
{
    const std::lock_guard guard{mutex_};
    return session_ ? session_->listenTo() : nullptr;
}

CaptureController::Levels CaptureController::takeLevels()
{
    const std::lock_guard guard{mutex_};
    if (!session_) {
        return {};
    }
    const auto peaks = session_->peaksSinceLastAsked();
    return {peaks.near, peaks.far};
}

void CaptureController::announce()
{
    StateCallback listener;
    {
        const std::lock_guard guard{mutex_};
        listener = listener_;
    }
    if (listener) {
        listener();
    }
}

}
