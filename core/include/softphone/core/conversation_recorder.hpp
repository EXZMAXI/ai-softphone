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

#include <softphone/core/audio_encoder.hpp>
#include <softphone/core/recording_taps.hpp>
#include <softphone/core/timeline_writer.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace softphone::core {

struct StemOutcome {
    std::string callId;

    std::chrono::milliseconds offset{0};

    std::chrono::milliseconds duration{0};
    std::uint64_t bytes{0};
    EncoderStatus status{EncoderStatus::Ok};
};

struct RecordingOutcome {
    EncoderStatus status{EncoderStatus::Ok};

    std::chrono::milliseconds duration{0};

    std::uint64_t bytes{0};

    std::uint64_t gapSamples{0};

    std::vector<StemOutcome> stems;

    [[nodiscard]] bool hasGaps() const noexcept
    {
        return gapSamples > 0;
    }
};

class ConversationRecorder {
public:

    using EncoderFactory = std::function<std::unique_ptr<IAudioEncoder>(const std::string &callId)>;

    ConversationRecorder(RecordingFeed &feed, EncoderFactory factory);

    ConversationRecorder(const ConversationRecorder &) = delete;
    ConversationRecorder &operator=(const ConversationRecorder &) = delete;
    ConversationRecorder(ConversationRecorder &&) = delete;
    ConversationRecorder &operator=(ConversationRecorder &&) = delete;
    ~ConversationRecorder();

    [[nodiscard]] EncoderStatus start();

    RecordingOutcome stop();

    [[nodiscard]] bool running() const noexcept
    {
        return running_.load(std::memory_order_acquire);
    }

    void keepAudio(bool keeping);

    void keepNearSide(bool keeping);

    void keepFarSide(bool keeping);

    [[nodiscard]] std::chrono::milliseconds elapsed() const noexcept;

    static constexpr std::chrono::milliseconds kLag{100};

    static constexpr std::chrono::milliseconds kCadence{200};

private:
    void run();

    void drain();

    [[nodiscard]] bool findTheStart();

    struct Stem {
        std::string callId;
        RecordingRing *ring{nullptr};
        std::unique_ptr<IAudioEncoder> encoder;
        std::unique_ptr<TimelineWriter> writer;
        std::uint64_t offset{0};
    };

    void openStems();

    void advanceTo(std::uint64_t position);

    [[nodiscard]] std::uint64_t upTo(std::uint64_t wanted) const;

    RecordingFeed &feed_;
    EncoderFactory factory_;

    std::unique_ptr<IAudioEncoder> encoder_;
    std::unique_ptr<TimelineWriter> writer_;

    std::vector<Stem> stems_;

    struct Switch {
        std::uint64_t position{0};
        bool keeping{true};

        enum class Side { Near, Far, Both };
        Side side{Side::Both};
    };

    std::mutex switchMutex_;
    std::vector<Switch> switches_;

    bool keepingNear_{true};
    bool keepingFar_{true};

    std::uint64_t origin_{0};
    bool begun_{false};

    std::atomic<std::uint64_t> written_{0};

    std::thread thread_;
    std::atomic<bool> running_{false};

    std::mutex mutex_;
    std::condition_variable wake_;
    bool stopping_{false};

    std::vector<std::int16_t> scratch_;

    RecordingOutcome outcome_;
    bool stopped_{false};
};

}
