// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/recording_taps.hpp>

#include <algorithm>

namespace softphone::core {
namespace {

[[nodiscard]] std::size_t depthInSamples(unsigned sampleRate, unsigned seconds) noexcept
{
    return static_cast<std::size_t>(sampleRate) * std::max(1U, seconds);
}

[[nodiscard]] std::size_t depthInFrames(unsigned sampleRate, unsigned frameSamples,
                                        unsigned seconds) noexcept
{
    const std::size_t frames =
        frameSamples == 0 ? 0 : depthInSamples(sampleRate, seconds) / frameSamples;
    return frames < 256 ? 256 : frames;
}

}

RecordingFeed::RecordingFeed(unsigned sampleRate, unsigned frameSamples, unsigned depthSeconds)
    : sampleRate_{sampleRate == 0 ? 16000u : sampleRate},
      frameSamples_{frameSamples == 0 ? sampleRate_ / 50 : frameSamples},
      depthSeconds_{std::max(kRecordingDepthSeconds, depthSeconds)},
      near_{depthInSamples(sampleRate_, depthSeconds_),
            depthInFrames(sampleRate_, frameSamples_, depthSeconds_)},
      far_{depthInSamples(sampleRate_, depthSeconds_),
           depthInFrames(sampleRate_, frameSamples_, depthSeconds_)}
{}

RecordingRing &RecordingFeed::openStem(const std::string &callId)
{
    const std::lock_guard<std::mutex> held{mutex_};

    for (Slot &slot : stems_) {
        if (slot.callId == callId) {
            return *slot.ring;
        }
    }

    auto ring =
        std::make_unique<RecordingRing>(depthInSamples(sampleRate_, depthSeconds_),
                                        depthInFrames(sampleRate_, frameSamples_, depthSeconds_));
    RecordingRing &made = *ring;
    stems_.push_back(Slot{callId, std::move(ring)});
    return made;
}

std::vector<RecordingFeed::Stem> RecordingFeed::stems() const
{
    const std::lock_guard<std::mutex> held{mutex_};

    std::vector<Stem> all;
    all.reserve(stems_.size());
    for (const Slot &slot : stems_) {
        all.push_back(Stem{slot.callId, slot.ring.get()});
    }
    return all;
}

void RecordingFeed::openListening()
{
    const std::lock_guard<std::mutex> held{mutex_};

    if (!nearListening_) {
        nearListening_ = std::make_unique<RealtimeRing>(RealtimeRing::roomFor(sampleRate_));
        farListening_ = std::make_unique<RealtimeRing>(RealtimeRing::roomFor(sampleRate_));
    }

    listening_.store(true, std::memory_order_release);
}

void RecordingFeed::restart() noexcept
{
    near_.restart();
    far_.restart();

    listening_.store(false, std::memory_order_release);
    if (nearListening_) {
        nearListening_->restart();
        farListening_->restart();
    }

    const std::lock_guard<std::mutex> held{mutex_};
    stems_.clear();
}

}
