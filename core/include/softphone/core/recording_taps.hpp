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

#include <softphone/core/realtime_ring.hpp>
#include <softphone/core/recording_ring.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace softphone::core {

inline constexpr unsigned kRecordingDepthSeconds = 30;

class RecordingFeed {
public:

    explicit RecordingFeed(unsigned sampleRate, unsigned frameSamples,
                           unsigned depthSeconds = kRecordingDepthSeconds);

    [[nodiscard]] unsigned sampleRate() const noexcept
    {
        return sampleRate_;
    }

    [[nodiscard]] unsigned frameSamples() const noexcept
    {
        return frameSamples_;
    }

    [[nodiscard]] RecordingRing &nearSide() noexcept
    {
        return near_;
    }

    [[nodiscard]] RecordingRing &farSide() noexcept
    {
        return far_;
    }

    struct Stem {
        std::string callId;
        RecordingRing *ring{nullptr};
    };

    RecordingRing &openStem(const std::string &callId);

    [[nodiscard]] std::vector<Stem> stems() const;

    void restart() noexcept;

    void openListening();

    [[nodiscard]] bool listening() const noexcept
    {
        return listening_.load(std::memory_order_acquire);
    }

    [[nodiscard]] RealtimeRing *nearListening() noexcept
    {
        return nearListening_.get();
    }

    [[nodiscard]] RealtimeRing *farListening() noexcept
    {
        return farListening_.get();
    }

private:
    unsigned sampleRate_;
    unsigned frameSamples_;

    unsigned depthSeconds_;

    RecordingRing near_;
    RecordingRing far_;

    struct Slot {
        std::string callId;
        std::unique_ptr<RecordingRing> ring;
    };

    mutable std::mutex mutex_;
    std::vector<Slot> stems_;

    std::unique_ptr<RealtimeRing> nearListening_;
    std::unique_ptr<RealtimeRing> farListening_;

    std::atomic<bool> listening_{false};
};

class IRecordingTaps {
public:
    IRecordingTaps() = default;
    IRecordingTaps(const IRecordingTaps &) = delete;
    IRecordingTaps &operator=(const IRecordingTaps &) = delete;
    IRecordingTaps(IRecordingTaps &&) = delete;
    IRecordingTaps &operator=(IRecordingTaps &&) = delete;
    virtual ~IRecordingTaps() = default;

    [[nodiscard]] virtual RecordingFeed &openRecording(const std::string &recordingId) = 0;

    virtual void recordCalls(const std::string &recordingId,
                             const std::vector<std::string> &callIds, bool separately) = 0;

    virtual void closeRecording(const std::string &recordingId) = 0;
};

}
