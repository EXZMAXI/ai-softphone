// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#pragma once

#include <softphone/plugins/softphone/credential_refiling.hpp>

#include <softphone/core/busy_lamp.hpp>
#include <softphone/core/call_controller.hpp>
#include <softphone/core/config_store.hpp>
#include <softphone/core/engine.hpp>
#include <softphone/core/event_loop.hpp>
#include <softphone/core/logging.hpp>
#include <softphone/core/recording_controller.hpp>
#include <softphone/core/recording_library.hpp>
#include <softphone/core/redial.hpp>
#include <softphone/media/mp3_stream.hpp>
#include <softphone/secrets/credential_store.hpp>
#include <softphone/sip/pjsip_backend.hpp>
#include <softphone/storage/sqlite_store.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <thread>

namespace softphone::plugins::softphone {

class EngineHost {
public:

    explicit EngineHost(bool useNullAudioDevice = false,
                        std::optional<core::LogLevel> level = std::nullopt,
                        std::filesystem::path configDirectory = {},
                        std::filesystem::path dataDirectory = {}, std::string systemLanguage = {},
                        core::ConfigDocument document = {});

    EngineHost(const EngineHost &) = delete;
    EngineHost &operator=(const EngineHost &) = delete;
    EngineHost(EngineHost &&) = delete;
    EngineHost &operator=(EngineHost &&) = delete;
    ~EngineHost();

    [[nodiscard]] std::optional<std::string> start();

    void stop();

    [[nodiscard]] core::Engine &engine() noexcept
    {
        return *engine_;
    }

    [[nodiscard]] core::CallController &calls() noexcept
    {
        return *calls_;
    }

    [[nodiscard]] core::RedialController &redial() noexcept
    {
        return *redial_;
    }

    void setRedialListener(core::RedialController::StateCallback listener);

    [[nodiscard]] core::BusyLampController &lamps() noexcept
    {
        return *lamps_;
    }

    void setLampListener(core::BusyLampController::StateCallback listener);

    [[nodiscard]] core::RecordingController &recording() noexcept
    {
        return *recording_;
    }

    [[nodiscard]] core::RecordingFeed *listenTo(const std::string &sessionId,
                                                const std::vector<std::string> &callIds);

    void stopListening(const std::string &sessionId);

    [[nodiscard]] bool running() const noexcept
    {
        return thread_.joinable();
    }

    void armRecording(bool armed);

    void armLamps(bool armed);

    void setButtons(std::vector<core::PhoneButton> buttons, std::string dialAccountId);

    [[nodiscard]] bool lampsArmed() const noexcept
    {
        return lampsArmed_;
    }

    [[nodiscard]] bool recordingArmed() const noexcept
    {
        return recordingArmed_;
    }

    [[nodiscard]] core::RecordingLibrary &library() noexcept
    {
        return *library_;
    }

    void applyRetention(const core::RetentionPolicy &retention);

    void setRecordingAskListener(std::function<void(std::string)> ask);

    void setRecordingWrittenListener(std::function<void(const core::Recording &)> written);

    void setRecordingRefusalListener(std::function<void(std::string, core::EncoderStatus)> told);

    void startRecording(const std::string &callId);

    struct Playback {
        bool playing{false};
        std::uint64_t positionMs{0};
        std::uint64_t durationMs{0};

        bool ended{false};

        std::filesystem::path file;
    };

    void setPlaybackListener(std::function<void(Playback)> listener);

    void setPlaybackFailureListener(std::function<void(std::filesystem::path)> told);

    void playRecording(const std::filesystem::path &file, bool start = true);
    void stopPlaying();
    void pausePlaying(bool paused);
    void seekPlayback(std::uint64_t positionMs);
    void setPlaybackSpeed(double speed);
    void setPlaybackChannels(bool nearSide, bool farSide);

    void setKnownAccounts(std::vector<core::AccountConfig> accounts);

    [[nodiscard]] const core::AccountConfig *accountById(const std::string &accountId) const;

    [[nodiscard]] sip::PjsipBackend &backend() noexcept
    {
        return *backend_;
    }

    [[nodiscard]] core::IAudioDeviceBackend &audio() noexcept
    {
        return *backend_;
    }

    [[nodiscard]] core::ISignalTaps &taps() noexcept
    {
        return *backend_;
    }

    [[nodiscard]] core::ICodecBackend &codecs() noexcept
    {
        return *backend_;
    }

    void post(std::function<void()> work);

    [[nodiscard]] core::IScheduler &scheduler() noexcept
    {
        return loop_;
    }

    void setStackLogLevel(core::LogLevel level);

    void setTraceSink(core::ISipTraceSink *sink) noexcept;

    void pollMedia(std::function<void(std::vector<core::CallMedia>)> report);

    void setCallListener(core::CallController::CallCallback listener);

    void startTone(const std::string &wavPath, bool loop, core::ToneOutput output,
                   core::ToneLevel level);
    void stopTone();

    void setToneEndedCallback(std::function<void()> callback);

    [[nodiscard]] core::ConfigStore &configuration() noexcept
    {
        return configStore_;
    }

    [[nodiscard]] secrets::ICredentialStore &keyring() noexcept
    {
        return *keeping_.store;
    }

    [[nodiscard]] secrets::StoreKind keyringKind() const noexcept
    {
        return keeping_.kind;
    }

    [[nodiscard]] const core::StoredConfiguration &storedConfiguration() const noexcept
    {
        return stored_;
    }

    [[nodiscard]] secrets::SecretStatus storedPasswordStatus() const;

    [[nodiscard]] secrets::SecretStatus rememberPassword(const core::AccountConfig &account,
                                                         const std::string &password);

    [[nodiscard]] secrets::SecretStatus forgetPassword(const core::AccountConfig &account);

    [[nodiscard]] secrets::SecretStatus refilePassword(const Refiling &move);

private:
    core::EventLoop loop_;

    secrets::ChosenStore keeping_{secrets::makeCredentialStore()};
    core::ConfigStore configStore_;

    std::filesystem::path dataRoot_;

    std::filesystem::path recordingsRoot_;
    std::unique_ptr<sip::PjsipBackend> backend_;
    std::unique_ptr<core::Engine> engine_;
    std::unique_ptr<core::CallController> calls_;
    std::unique_ptr<core::RedialController> redial_;
    std::unique_ptr<core::BusyLampController> lamps_;

    void applyLamps();

    std::vector<core::PhoneButton> wantedButtons_;
    std::string wantedDialAccount_;
    bool lampsArmed_{false};
    std::unique_ptr<storage::SqliteStore> recordings_;
    std::unique_ptr<core::RecordingLibrary> library_;
    std::unique_ptr<core::RecordingController> recording_;

    core::ISipTraceSink *traceSink_{nullptr};

    bool recordingArmed_{false};

    void armTheSweep();

    void stopRecordingIfTheDiskIsFull();

    core::IScheduler::TimerId sweepTimer_{};

    std::unique_ptr<media::Mp3Stream> stream_;
    bool playing_{false};

    double wantedSpeed_{1.0};
    bool wantedNear_{true};
    bool wantedFar_{true};
    core::IScheduler::TimerId playbackTimer_{};
    std::function<void(Playback)> playbackListener_;

    std::filesystem::path playingFile_;
    std::function<void(std::string, core::EncoderStatus)> recordingRefused_;
    std::function<void(std::filesystem::path)> playbackFailed_;

    void armThePlayhead();
    void publishPlayback(bool ended);

    void stopPlayingHere();
    std::thread thread_;
    core::StoredConfiguration stored_;

    std::vector<core::AccountConfig> knownAccounts_;
    core::CallController::CallCallback callListener_;
    core::RedialController::StateCallback redialListener_;
    core::BusyLampController::StateCallback lampListener_;
};

}
