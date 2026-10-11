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

#include <softphone/capture/system_capture.hpp>

#include <softphone/core/audio_signal.hpp>
#include <softphone/core/conversation_recorder.hpp>
#include <softphone/core/recording_taps.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <vector>

namespace softphone::capture {

struct Endings {

    std::chrono::milliseconds ceiling{std::chrono::hours{8}};

    std::chrono::milliseconds quiet{std::chrono::minutes{10}};

    std::chrono::milliseconds gone{std::chrono::seconds{5}};
};

enum class Ending {
    Nothing,

    Roll,

    Quiet,
};

[[nodiscard]] Ending endingFor(const Endings &when, std::chrono::milliseconds elapsed,
                               std::chrono::milliseconds quietFor);

class CaptureSession {
public:
    CaptureSession(ISystemCapture &capture, core::ConversationRecorder::EncoderFactory factory);

    CaptureSession(const CaptureSession &) = delete;
    CaptureSession &operator=(const CaptureSession &) = delete;
    CaptureSession(CaptureSession &&) = delete;
    CaptureSession &operator=(CaptureSession &&) = delete;
    ~CaptureSession();

    struct Started {
        core::EncoderStatus encoder{core::EncoderStatus::Ok};
        CaptureError capture{CaptureError::None};

        [[nodiscard]] bool ok() const noexcept
        {
            return encoder == core::EncoderStatus::Ok && capture == CaptureError::None;
        }
    };

    [[nodiscard]] Started start(const CaptureRequest &request);

    [[nodiscard]] CaptureError arm(const CaptureRequest &request,
                                   std::chrono::milliseconds preRoll);

    [[nodiscard]] core::EncoderStatus beginRecording();

    core::RecordingOutcome endRecording();

    void disarm();

    [[nodiscard]] bool armed() const noexcept
    {
        return armed_.load(std::memory_order_acquire);
    }

    core::RecordingOutcome stop();

    [[nodiscard]] bool running() const noexcept;

    [[nodiscard]] std::chrono::milliseconds elapsed() const;

    [[nodiscard]] core::RecordingFeed *listenTo();

    void keepAudio(bool keeping);

    [[nodiscard]] CaptureStatus captureStatus() const;

    struct Peaks {
        unsigned near{0};
        unsigned far{0};
    };

    [[nodiscard]] Peaks peaksSinceLastAsked() noexcept;

    [[nodiscard]] std::chrono::milliseconds
    quietFor(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now()) const;

    [[nodiscard]] std::chrono::milliseconds sinceDelivery(
        Side side,
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now()) const;

    void drawInto(core::CallSignals *taps) noexcept
    {
        signals_.store(taps, std::memory_order_release);
    }

    [[nodiscard]] std::uint64_t shed() const noexcept
    {
        return shed_.load(std::memory_order_relaxed);
    }

private:

    [[nodiscard]] std::optional<std::uint64_t>
    positionFor(Side side, std::chrono::steady_clock::time_point at, std::size_t count) noexcept;

    void handOver(Side side, std::uint64_t position,
                  std::span<const std::int16_t> samples) noexcept;

    ISystemCapture &capture_;
    std::unique_ptr<core::RecordingFeed> feed_;
    std::unique_ptr<core::ConversationRecorder> recorder_;
    core::ConversationRecorder::EncoderFactory factory_;

    unsigned sampleRate_{16000};
    CaptureRequest request_;
    std::chrono::milliseconds preRoll_{0};

    std::chrono::steady_clock::time_point keepFrom_{std::chrono::steady_clock::time_point::min()};

    std::atomic<bool> anchored_{false};
    std::chrono::steady_clock::time_point origin_{};

    std::atomic<std::uint64_t> nextNear_{0};
    std::atomic<std::uint64_t> nextFar_{0};

    std::atomic<std::uint64_t> shed_{0};

    std::atomic<unsigned> peakNear_{0};
    std::atomic<unsigned> peakFar_{0};

    std::atomic<unsigned> loudestNear_{0};
    std::atomic<unsigned> loudestFar_{0};

    std::atomic<std::int64_t> heardAt_{0};

    std::atomic<std::int64_t> deliveredNear_{0};
    std::atomic<std::int64_t> deliveredFar_{0};

    std::atomic<core::CallSignals *> signals_{nullptr};

    struct Held {
        std::vector<std::int16_t> samples;
        std::vector<std::chrono::steady_clock::time_point> when;
        std::vector<std::uint32_t> counts;
        std::atomic<std::uint64_t> written{0};
        std::uint32_t places{0};
        std::uint32_t stride{0};
    };

    void keep(Held &held, std::span<const std::int16_t> samples,
              std::chrono::steady_clock::time_point at) noexcept;

    [[nodiscard]] std::uint64_t surviving(const Held &held, std::uint64_t from,
                                          std::uint64_t written) const noexcept;

    void replayHeld();

    Held heldNear_;
    Held heldFar_;
    std::atomic<bool> armed_{false};
    std::atomic<bool> recording_{false};

    std::atomic<bool> running_{false};
    mutable std::mutex mutex_;
    core::RecordingOutcome outcome_{};
    bool stopped_{false};
};

}
