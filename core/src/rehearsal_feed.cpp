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
#include <mutex>

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

    std::size_t in{0};

    bool oneRoom{false};

    std::mutex holding;
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

void RehearsalFeed::loadSides(std::vector<std::int16_t> &&near, std::vector<std::int16_t> &&far)
{
    const std::lock_guard<std::mutex> held{held_->holding};

    held_->nearAudio.clear();
    held_->farAudio.clear();
    held_->sent = 0;
    held_->clock = 0;
    held_->skipped = 0;
    held_->nearRing->restart();
    held_->farRing->restart();

    if (far.empty()) {

        held_->farAudio = std::move(near);
        held_->nearAudio.assign(held_->farAudio.size(), 0);
        held_->in = held_->farAudio.size();
        return;
    }

    held_->nearAudio = std::move(near);
    held_->farAudio = std::move(far);
    held_->in = held_->farAudio.size();
}

void RehearsalFeed::expect(std::chrono::milliseconds length, bool oneRoom)
{
    const std::lock_guard<std::mutex> held{held_->holding};

    held_->sent = 0;
    held_->clock = 0;
    held_->skipped = 0;
    held_->in = 0;
    held_->oneRoom = oneRoom;
    held_->nearRing->restart();
    held_->farRing->restart();

    const auto frames =
        length.count() <= 0
            ? std::size_t{0}
            : static_cast<std::size_t>(length.count() * static_cast<std::int64_t>(rate_) / 1000);

    held_->nearAudio.assign(frames, 0);
    held_->farAudio.assign(frames, 0);
}

void RehearsalFeed::feedSides(std::span<const std::int16_t> near, std::span<const std::int16_t> far)
{
    const std::lock_guard<std::mutex> held{held_->holding};

    const auto room = held_->farAudio.size() - std::min(held_->in, held_->farAudio.size());
    if (room == 0) {
        return;
    }

    const auto theirs = held_->oneRoom || far.empty() ? near : far;
    const auto mine = held_->oneRoom ? std::span<const std::int16_t>{} : near;

    const auto count = std::min(room, theirs.size());
    if (count == 0) {
        return;
    }

    std::copy_n(theirs.begin(), count,
                held_->farAudio.begin() + static_cast<std::ptrdiff_t>(held_->in));
    if (!mine.empty()) {
        std::copy_n(mine.begin(), std::min(count, mine.size()),
                    held_->nearAudio.begin() + static_cast<std::ptrdiff_t>(held_->in));
    }

    held_->in += count;
}

void RehearsalFeed::load(const std::vector<std::int16_t> &interleaved, unsigned channels,
                         unsigned rate, Sides sides)
{
    const std::lock_guard<std::mutex> held{held_->holding};

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
        held_->in = held_->farAudio.size();
        return;
    }

    channelAt(held_->farAudio, interleaved, channels, fileRate, rate_, 0);
    held_->nearAudio.assign(held_->farAudio.size(), 0);
    held_->in = held_->farAudio.size();
}

bool RehearsalFeed::ready() const
{
    const std::lock_guard<std::mutex> held{held_->holding};
    return held_->in > 0;
}

std::chrono::milliseconds RehearsalFeed::length() const
{
    const std::lock_guard<std::mutex> held{held_->holding};
    return std::chrono::milliseconds{static_cast<std::int64_t>(held_->in) * 1000 /
                                     static_cast<std::int64_t>(rate_)};
}

void RehearsalFeed::upTo(std::chrono::milliseconds position)
{
    const std::lock_guard<std::mutex> held{held_->holding};

    if (held_->in == 0 || position.count() < 0) {
        return;
    }

    const auto wanted = std::min<std::size_t>(
        held_->in,
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
    const std::lock_guard<std::mutex> held{held_->holding};

    held_->sent = 0;
    held_->clock = 0;
    held_->skipped = 0;
    held_->nearRing->restart();
    held_->farRing->restart();
}

std::uint64_t RehearsalFeed::sentSamples() const
{
    const std::lock_guard<std::mutex> held{held_->holding};
    return held_->clock;
}

std::uint64_t RehearsalFeed::skippedSamples() const
{
    const std::lock_guard<std::mutex> held{held_->holding};
    return held_->skipped;
}

}
