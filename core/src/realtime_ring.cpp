// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/realtime_ring.hpp>

#include <algorithm>
#include <bit>

namespace softphone::core {
namespace {

[[nodiscard]] std::size_t roundUp(std::size_t wanted) noexcept
{
    return wanted <= 1 ? 1 : std::bit_ceil(wanted);
}

}

RealtimeRing::RealtimeRing(std::size_t samples, std::size_t runs)
    : samples_(roundUp(samples)), runs_(roundUp(runs)), mask_{samples_.size() - 1},
      runMask_{runs_.size() - 1}
{}

void RealtimeRing::write(std::uint64_t position, const std::int16_t *samples,
                         std::size_t count) noexcept
{
    if (begins_.load(std::memory_order_relaxed) == kNothingYet) {
        begins_.store(position, std::memory_order_release);
    }

    clock_.store(position + count, std::memory_order_release);

    if (samples == nullptr || count == 0) {
        return;
    }

    const auto written = written_.load(std::memory_order_relaxed);
    const auto published = published_.load(std::memory_order_relaxed);

    const auto room = samples_.size() -
                      static_cast<std::size_t>(written - taken_.load(std::memory_order_acquire));

    if (count > room) {
        dropped_.fetch_add(count, std::memory_order_relaxed);
        pending_ += count;
        return;
    }

    bool extending = false;
    if (published > 0) {
        auto &open = runs_[static_cast<std::size_t>(published - 1) & runMask_];
        extending = open.start + open.count.load(std::memory_order_relaxed) == position;
    }

    if (!extending) {

        if (published - consumed_.load(std::memory_order_acquire) >= runs_.size()) {
            dropped_.fetch_add(count, std::memory_order_relaxed);
            pending_ += count;
            return;
        }
    }

    const auto at = static_cast<std::size_t>(written) & mask_;
    const auto first = std::min(count, samples_.size() - at);
    std::copy_n(samples, first, samples_.begin() + static_cast<std::ptrdiff_t>(at));
    if (first < count) {
        std::copy_n(samples + first, count - first, samples_.begin());
    }

    written_.store(written + count, std::memory_order_release);

    if (extending) {
        auto &open = runs_[static_cast<std::size_t>(published - 1) & runMask_];
        open.count.fetch_add(count, std::memory_order_release);
        return;
    }

    auto &fresh = runs_[static_cast<std::size_t>(published) & runMask_];
    fresh.start = position;
    fresh.lostBefore = pending_;
    fresh.count.store(count, std::memory_order_relaxed);
    pending_ = 0;
    published_.store(published + 1, std::memory_order_release);
}

bool RealtimeRing::read(Frame &frame, std::span<std::int16_t> out) noexcept
{
    if (out.empty()) {
        return false;
    }

    for (;;) {
        const auto consumed = consumed_.load(std::memory_order_relaxed);
        const auto published = published_.load(std::memory_order_acquire);
        if (consumed >= published) {
            return false;
        }

        auto &head = runs_[static_cast<std::size_t>(consumed) & runMask_];
        const auto count = head.count.load(std::memory_order_acquire);

        if (offset_ >= count) {

            if (consumed + 1 >= published) {
                return false;
            }
            consumed_.store(consumed + 1, std::memory_order_release);
            offset_ = 0;
            continue;
        }

        const auto wanted = std::min<std::uint64_t>(count - offset_, out.size());
        const auto taken = taken_.load(std::memory_order_relaxed);

        const auto at = static_cast<std::size_t>(taken) & mask_;
        const auto first =
            std::min<std::size_t>(static_cast<std::size_t>(wanted), samples_.size() - at);
        std::copy_n(samples_.begin() + static_cast<std::ptrdiff_t>(at), first, out.begin());
        if (first < wanted) {
            std::copy_n(samples_.begin(), static_cast<std::size_t>(wanted) - first,
                        out.begin() + static_cast<std::ptrdiff_t>(first));
        }

        frame.position = head.start + offset_;
        frame.count = static_cast<std::size_t>(wanted);

        frame.lostBefore = offset_ == 0 ? head.lostBefore : 0;

        offset_ += wanted;
        taken_.store(taken + wanted, std::memory_order_release);
        return true;
    }
}

std::size_t RealtimeRing::waiting() const noexcept
{
    const auto written = written_.load(std::memory_order_acquire);
    const auto taken = taken_.load(std::memory_order_acquire);
    return static_cast<std::size_t>(written - taken);
}

void RealtimeRing::restart() noexcept
{
    written_.store(0, std::memory_order_relaxed);
    taken_.store(0, std::memory_order_relaxed);
    published_.store(0, std::memory_order_relaxed);
    consumed_.store(0, std::memory_order_relaxed);
    offset_ = 0;
    pending_ = 0;
    clock_.store(0, std::memory_order_relaxed);
    dropped_.store(0, std::memory_order_relaxed);
    begins_.store(kNothingYet, std::memory_order_release);
}

}
