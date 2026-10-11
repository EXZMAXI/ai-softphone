// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/timeline_writer.hpp>

#include <algorithm>
#include <cstring>

namespace softphone::core {

TimelineWriter::TimelineWriter(unsigned channels, IAudioEncoder &encoder, std::uint64_t start)
    : encoder_{encoder}, channels_(std::max(1u, std::min(channels, 2u))), start_{start},
      head_{start}
{}

void TimelineWriter::reserve(Channel &channel, std::size_t count)
{
    if (channel.pending.size() >= count) {
        return;
    }
    channel.pending.resize(count, 0);
    channel.covered.resize(count, 0);
}

void TimelineWriter::offer(unsigned channel, std::uint64_t position,
                           std::span<const std::int16_t> samples)
{
    if (channel >= channels_.size() || samples.empty()) {
        return;
    }

    if (position + samples.size() <= head_) {
        late_ += samples.size();
        return;
    }
    if (position < head_) {
        const auto stale = static_cast<std::size_t>(head_ - position);
        late_ += stale;
        samples = samples.subspan(stale);
        position = head_;
    }

    Channel &wanted = channels_[channel];

    if (position + samples.size() <= settled_ && mutedAt(wanted, position)) {
        return;
    }

    const auto offset = static_cast<std::size_t>(position - head_);
    reserve(wanted, offset + samples.size());

    const auto at = static_cast<std::ptrdiff_t>(offset);
    std::copy(samples.begin(), samples.end(), wanted.pending.begin() + at);
    std::fill_n(wanted.covered.begin() + at, samples.size(), std::uint8_t{1});
}

void TimelineWriter::settled(std::uint64_t upTo)
{
    settled_ = std::max(settled_, upTo);
}

void TimelineWriter::mute(std::uint64_t from, bool muted)
{
    for (unsigned channel = 0; channel < channels_.size(); ++channel) {
        mute(channel, from, muted);
    }
}

void TimelineWriter::mute(unsigned channel, std::uint64_t from, bool muted)
{
    if (channel >= channels_.size()) {
        return;
    }

    const std::uint64_t at = std::max(from, head_);

    std::vector<Mute> &mutes = channels_[channel].mutes;
    if (!mutes.empty() && mutes.back().from >= at) {
        mutes.back().muted = muted;
        return;
    }
    mutes.push_back(Mute{at, muted});
}

bool TimelineWriter::mutedAt(const Channel &channel, std::uint64_t position)
{
    bool muted = channel.muted;
    for (const Mute &change : channel.mutes) {
        if (change.from > position) {
            break;
        }
        muted = change.muted;
    }
    return muted;
}

std::optional<std::uint64_t> TimelineWriter::quietFrom() const
{
    std::uint64_t began = 0;

    for (const Channel &channel : channels_) {

        bool muted = channel.muted;
        std::uint64_t since = head_;
        for (const Mute &change : channel.mutes) {
            if (change.muted != muted) {
                muted = change.muted;
                since = change.from;
            }
        }

        if (!muted) {
            return std::nullopt;
        }
        began = std::max(began, since);
    }

    return began;
}

EncoderStatus TimelineWriter::writeThrough(std::size_t count)
{
    if (count == 0) {
        return status_;
    }

    for (Channel &channel : channels_) {
        reserve(channel, count);
    }

    for (Channel &channel : channels_) {
        std::size_t at = 0;
        std::size_t next = 0;

        while (at < count) {
            while (next < channel.mutes.size() && channel.mutes[next].from <= head_ + at) {
                channel.muted = channel.mutes[next].muted;
                ++next;
            }

            std::size_t until = count;
            if (next < channel.mutes.size()) {
                until = std::min<std::size_t>(
                    count, static_cast<std::size_t>(channel.mutes[next].from - head_));
            }
            const std::size_t segment = until - at;
            const auto first = static_cast<std::ptrdiff_t>(at);

            if (channel.muted) {

                std::fill_n(channel.pending.begin() + first, segment, std::int16_t{0});
                std::fill_n(channel.covered.begin() + first, segment, std::uint8_t{1});
            } else {

                gaps_ += static_cast<std::uint64_t>(std::count(
                    channel.covered.begin() + first,
                    channel.covered.begin() + static_cast<std::ptrdiff_t>(until), std::uint8_t{0}));
            }

            at = until;
        }

        channel.mutes.erase(channel.mutes.begin(),
                            channel.mutes.begin() + static_cast<std::ptrdiff_t>(next));
    }

    if (status_ == EncoderStatus::Ok) {
        const std::span<const std::int16_t> left{channels_[0].pending.data(), count};
        const std::span<const std::int16_t> right =
            channels_.size() > 1 ? std::span<const std::int16_t>{channels_[1].pending.data(), count}
                                 : std::span<const std::int16_t>{};

        status_ = encoder_.write(left, right);
    }

    for (Channel &channel : channels_) {
        const auto used = static_cast<std::ptrdiff_t>(count);
        channel.pending.erase(channel.pending.begin(), channel.pending.begin() + used);
        channel.covered.erase(channel.covered.begin(), channel.covered.begin() + used);
    }

    head_ += count;
    return status_;
}

EncoderStatus TimelineWriter::advance(std::uint64_t upTo)
{
    if (upTo <= head_) {
        return status_;
    }

    constexpr std::uint64_t kSlice = 16000;

    while (upTo > head_ && status_ == EncoderStatus::Ok) {
        const auto step = static_cast<std::size_t>(std::min(kSlice, upTo - head_));
        writeThrough(step);
    }

    if (upTo > head_) {
        head_ = upTo;
    }

    return status_;
}

EncoderStatus TimelineWriter::finish()
{

    std::size_t remaining = 0;
    for (const Channel &channel : channels_) {
        remaining = std::max(remaining, channel.pending.size());
    }

    if (const auto quiet = quietFrom()) {
        const auto room = *quiet > head_ ? static_cast<std::size_t>(*quiet - head_) : 0U;
        remaining = std::min(remaining, room);
    }

    if (remaining > 0) {
        writeThrough(remaining);
    }

    const EncoderStatus closed = encoder_.finish();
    if (status_ == EncoderStatus::Ok) {
        status_ = closed;
    }
    return status_;
}

std::uint64_t TimelineWriter::written() const noexcept
{
    return head_ - start_;
}

}
