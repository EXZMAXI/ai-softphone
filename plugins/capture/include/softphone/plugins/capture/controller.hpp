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

#include <softphone/capture/capture_session.hpp>
#include <softphone/capture/detector.hpp>
#include <softphone/capture/system_capture.hpp>
#include <softphone/plugins/capture/control.hpp>

#include <softphone/core/audio_encoder.hpp>
#include <softphone/core/recordings.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace softphone::plugins::capture {

namespace cap = softphone::capture;

class CaptureController final : public ICaptureControl {
public:
    using EncoderMaker = std::function<std::unique_ptr<core::IAudioEncoder>(
        const std::filesystem::path &, const core::EncoderSettings &)>;

    CaptureController(cap::ISystemCapture *capture, EncoderMaker maker, std::filesystem::path root,
                      core::IRecordingStore *store,
                      std::unique_ptr<cap::IConversationDetector> detector = {});

    CaptureController(const CaptureController &) = delete;
    CaptureController &operator=(const CaptureController &) = delete;
    CaptureController(CaptureController &&) = delete;
    CaptureController &operator=(CaptureController &&) = delete;
    ~CaptureController();

    void setSettings(const Settings &settings) override;

    [[nodiscard]] Settings settings() const override;

    void setWritten(WrittenCallback written) override;
    void setExcluded(ExcludedCallback excluded) override;
    void setFound(FoundCallback found) override;
    void setLost(LostCallback lost) override;

    void setMeetingPhrase(PhraseSource words) override;
    void setListener(StateCallback listener) override;

    [[nodiscard]] cap::CaptureError start(std::string what, std::string process = {}) override;

    [[nodiscard]] bool notACall(const std::string &recordingId) override;

    bool rename(std::string what) override;

    void stop() override;

    core::Recording stopAndTell();

    [[nodiscard]] State state() const override;

    [[nodiscard]] core::RecordingFeed *listenTo() override;

    [[nodiscard]] Levels takeLevels() override;

    [[nodiscard]] core::ISignalTaps &drawings() noexcept override
    {
        return drawings_;
    }

    void suppress(bool suppressed) override;

    void yieldTheSoundCard(bool wanted) override;

    void watchWith(const cap::Endings &when) override;

    void watch();

    void sayWhy(std::string_view what) const;

    [[nodiscard]] bool suppressed() const override;

    [[nodiscard]] bool suppressedNow() const;

private:

    class Drawings final : public core::ISignalTaps {
    public:

        void setChanged(std::function<void()> changed)
        {
            changed_ = std::move(changed);
        }

        void watchSignals(const std::string &name) override
        {
            const bool wanted = !name.empty();
            if (wanted == watching_) {
                return;
            }
            watching_ = wanted;
            if (!watching_) {
                signals_.clear();
            }
            if (changed_) {
                changed_();
            }
        }

        [[nodiscard]] core::CallSignals &feed() noexcept override
        {
            return signals_;
        }

        [[nodiscard]] bool watching() const noexcept
        {
            return watching_;
        }

    private:
        core::CallSignals signals_{16000};
        bool watching_{false};
        std::function<void()> changed_;
    };

    Drawings drawings_;

    void announce();

    void armIfWanted();

    void applyDetector();

    bool wantsDetector_{false};

    [[nodiscard]] core::ConversationRecorder::EncoderFactory encoderFactory();
    [[nodiscard]] EncoderMaker encoderFactoryMaker() const;

    std::filesystem::path pending_;

    [[nodiscard]] cap::CaptureError openTheDevices();

    std::unique_ptr<cap::IConversationDetector> detector_;

    bool detecting_{false};
    std::mutex detectorMutex_;

    mutable std::mutex mutex_;

    cap::ISystemCapture *capture_{nullptr};
    EncoderMaker maker_;
    std::filesystem::path root_;
    core::IRecordingStore *store_{nullptr};

    std::unique_ptr<cap::CaptureSession> session_;
    Settings settings_;
    WrittenCallback written_;
    ExcludedCallback excludedChanged_;
    FoundCallback found_;
    LostCallback lost_;
    PhraseSource meetingPhrase_;
    StateCallback listener_;

    core::Recording row_;
    bool running_{false};
    bool suppressed_{false};

    bool cardWanted_{false};
    bool yielding_{false};

    bool onlyTheTapGivesWay_{false};

    cap::Endings endings_;

    struct Gone {
        bool near{false};
        bool far{false};
    };

    Gone gone_;
    std::thread watching_;
    std::atomic<bool> watchingOn_{false};
    std::mutex watchSleeping_;
    std::condition_variable watchWake_;
    cap::CaptureError problem_{cap::CaptureError::None};
};

}
