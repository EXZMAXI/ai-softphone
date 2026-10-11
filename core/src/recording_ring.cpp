// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/recording_ring.hpp>

#include <algorithm>
#include <bit>

namespace softphone::core {
namespace {

[[nodiscard]] std::size_t roundUp(std::size_t wanted) noexcept
{
    return wanted <= 1 ? 1 : std::bit_ceil(wanted);
}

}

RecordingRing::RecordingRing(std::size_t samples, std::size_t frames)
    : samples_(roundUp(samples)), slots_(roundUp(frames)), sampleMask_{samples_.size() - 1},
      slotMask_{slots_.size() - 1}
{}

void RecordingRing::write(std::uint64_t position, const std::int16_t *samples,
                          std::size_t count) noexcept
{
    if (begins_.load(std::memory_order_relaxed) == kNothingYet) {
        begins_.store(position, std::memory_order_release);
    }

    clock_.store(position + count, std::memory_order_release);

    if (samples == nullptr || count == 0) {
        return;
    }

    const std::uint64_t written = written_.load(std::memory_order_relaxed);
    const std::uint64_t published = published_.load(std::memory_order_relaxed);

    const std::size_t roomForSamples =
        samples_.size() -
        static_cast<std::size_t>(written - taken_.load(std::memory_order_acquire));
    const std::size_t roomForFrames =
        slots_.size() -
        static_cast<std::size_t>(published - consumed_.load(std::memory_order_acquire));

    if (count > roomForSamples || roomForFrames == 0) {
        dropped_.fetch_add(count, std::memory_order_relaxed);
        return;
    }

    const std::size_t at = static_cast<std::size_t>(written) & sampleMask_;
    const std::size_t first = std::min(count, samples_.size() - at);
    std::copy_n(samples, first, samples_.begin() + static_cast<std::ptrdiff_t>(at));
    if (first < count) {
        std::copy_n(samples + first, count - first, samples_.begin());
    }

    Slot &slot = slots_[static_cast<std::size_t>(published) & slotMask_];
    slot.position = position;
    slot.start = written;
    slot.count = count;

    written_.store(written + count, std::memory_order_relaxed);
    published_.store(published + 1, std::memory_order_release);
}

bool RecordingRing::read(Frame &frame, std::span<std::int16_t> out) noexcept
{
    const std::uint64_t consumed = consumed_.load(std::memory_order_relaxed);
    if (consumed == published_.load(std::memory_order_acquire)) {
        return false;
    }

    const Slot &slot = slots_[static_cast<std::size_t>(consumed) & slotMask_];
    const std::size_t count = slot.count;

    if (count > out.size()) {
        dropped_.fetch_add(count, std::memory_order_relaxed);
        taken_.store(slot.start + count, std::memory_order_release);
        consumed_.store(consumed + 1, std::memory_order_release);
        return false;
    }

    const std::size_t at = static_cast<std::size_t>(slot.start) & sampleMask_;
    const std::size_t first = std::min(count, samples_.size() - at);
    std::copy_n(samples_.begin() + static_cast<std::ptrdiff_t>(at), first, out.begin());
    if (first < count) {
        std::copy_n(samples_.begin(), count - first,
                    out.begin() + static_cast<std::ptrdiff_t>(first));
    }

    frame.position = slot.position;
    frame.count = count;

    taken_.store(slot.start + count, std::memory_order_release);
    consumed_.store(consumed + 1, std::memory_order_release);
    return true;
}

void RecordingRing::restart() noexcept
{
    written_.store(0, std::memory_order_relaxed);
    taken_.store(0, std::memory_order_relaxed);
    published_.store(0, std::memory_order_relaxed);
    consumed_.store(0, std::memory_order_relaxed);
    clock_.store(0, std::memory_order_relaxed);
    dropped_.store(0, std::memory_order_relaxed);
    begins_.store(kNothingYet, std::memory_order_release);
}

std::size_t RecordingRing::waiting() const noexcept
{
    return static_cast<std::size_t>(written_.load(std::memory_order_acquire) -
                                    taken_.load(std::memory_order_acquire));
}

}
