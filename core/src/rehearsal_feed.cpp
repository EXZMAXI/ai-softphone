// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/rehearsal_feed.hpp>

#include <softphone/core/resample.hpp>

#include <algorithm>
#include <cstddef>

namespace softphone::core {
namespace {

void channelAt(std::vector<std::int16_t> &out, const std::vector<std::int16_t> &in,
               unsigned channels, unsigned from, unsigned to, unsigned channel)
{
    out.clear();
    if (channels == 0 || in.empty() || channel >= channels) {
        return;
    }

    const auto frames = in.size() / channels;
    std::vector<std::int16_t> one;
    one.reserve(frames);
    for (std::size_t at = 0; at < frames; ++at) {
        one.push_back(in[at * channels + channel]);
    }

    if (from == to || from == 0 || to == 0) {
        out = std::move(one);
        return;
    }

    out.reserve(resampledLength(one.size(), from, to));
    ResampleState held;
    resample(held, one, from, to, out);
}

}

struct RehearsalFeed::Held {

    std::vector<std::int16_t> nearAudio;
    std::vector<std::int16_t> farAudio;

    std::unique_ptr<RealtimeRing> nearRing;
    std::unique_ptr<RealtimeRing> farRing;

    std::size_t sent{0};

    std::uint64_t clock{0};

    std::uint64_t skipped{0};
};

RehearsalFeed::RehearsalFeed(unsigned rate)
    : held_{std::make_unique<Held>()}, rate_{rate == 0 ? 16000U : rate}
{
    held_->nearRing = std::make_unique<RealtimeRing>(RealtimeRing::roomFor(rate_));
    held_->farRing = std::make_unique<RealtimeRing>(RealtimeRing::roomFor(rate_));
}

RehearsalFeed::~RehearsalFeed() = default;

RealtimeRing *RehearsalFeed::nearListening() noexcept
{
    return held_->nearRing.get();
}

RealtimeRing *RehearsalFeed::farListening() noexcept
{
    return held_->farRing.get();
}

void RehearsalFeed::load(const std::vector<std::int16_t> &interleaved, unsigned channels,
                         unsigned rate, Sides sides)
{
    held_->nearAudio.clear();
    held_->farAudio.clear();
    held_->sent = 0;
    held_->clock = 0;
    held_->skipped = 0;
    held_->nearRing->restart();
    held_->farRing->restart();

    if (channels == 0 || interleaved.empty()) {
        return;
    }

    const auto fileRate = rate == 0 ? rate_ : rate;

    if (sides == Sides::NearAndFar && channels >= 2) {
        channelAt(held_->nearAudio, interleaved, channels, fileRate, rate_, 0);
        channelAt(held_->farAudio, interleaved, channels, fileRate, rate_, 1);
        return;
    }

    channelAt(held_->farAudio, interleaved, channels, fileRate, rate_, 0);
    held_->nearAudio.assign(held_->farAudio.size(), 0);
}

bool RehearsalFeed::ready() const noexcept
{
    return !held_->farAudio.empty();
}

std::chrono::milliseconds RehearsalFeed::length() const noexcept
{
    if (held_->farAudio.empty()) {
        return std::chrono::milliseconds::zero();
    }
    return std::chrono::milliseconds{static_cast<std::int64_t>(held_->farAudio.size()) * 1000 /
                                     static_cast<std::int64_t>(rate_)};
}

void RehearsalFeed::upTo(std::chrono::milliseconds position)
{
    if (held_->farAudio.empty() || position.count() < 0) {
        return;
    }

    const auto wanted = std::min<std::size_t>(
        held_->farAudio.size(),
        static_cast<std::size_t>(position.count() * static_cast<std::int64_t>(rate_) / 1000));

    if (wanted <= held_->sent) {

        held_->sent = wanted;
        return;
    }

    const auto count = wanted - held_->sent;

    static constexpr std::int64_t kSeekAboveMs = 2000;
    if (count > static_cast<std::size_t>(kSeekAboveMs * static_cast<std::int64_t>(rate_) / 1000)) {
        held_->sent = wanted;
        held_->skipped += count;
        return;
    }

    held_->farRing->write(held_->clock, held_->farAudio.data() + held_->sent, count);
    if (held_->nearAudio.size() >= wanted) {
        held_->nearRing->write(held_->clock, held_->nearAudio.data() + held_->sent, count);
    } else {
        held_->nearRing->write(held_->clock, nullptr, 0);
    }

    held_->sent = wanted;
    held_->clock += count;
}

void RehearsalFeed::rewind()
{
    held_->sent = 0;
    held_->clock = 0;
    held_->skipped = 0;
    held_->nearRing->restart();
    held_->farRing->restart();
}

std::uint64_t RehearsalFeed::sentSamples() const noexcept
{
    return held_->clock;
}

std::uint64_t RehearsalFeed::skippedSamples() const noexcept
{
    return held_->skipped;
}

}
