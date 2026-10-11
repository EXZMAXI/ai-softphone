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

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace softphone::core {

inline constexpr unsigned kRealtimeDepthSeconds = 4;

class RealtimeRing {
public:

    struct Frame {

        std::uint64_t position{0};

        std::size_t count{0};

        std::uint64_t lostBefore{0};
    };

    explicit RealtimeRing(std::size_t samples, std::size_t runs = 64);

    RealtimeRing(const RealtimeRing &) = delete;
    RealtimeRing &operator=(const RealtimeRing &) = delete;
    RealtimeRing(RealtimeRing &&) = delete;
    RealtimeRing &operator=(RealtimeRing &&) = delete;
    ~RealtimeRing() = default;

    [[nodiscard]] static std::size_t roomFor(unsigned sampleRate,
                                             unsigned seconds = kRealtimeDepthSeconds) noexcept
    {
        return static_cast<std::size_t>(sampleRate) * seconds;
    }

    void write(std::uint64_t position, const std::int16_t *samples, std::size_t count) noexcept;

    [[nodiscard]] bool read(Frame &frame, std::span<std::int16_t> out) noexcept;

    [[nodiscard]] std::uint64_t clock() const noexcept
    {
        return clock_.load(std::memory_order_acquire);
    }

    static constexpr std::uint64_t kNothingYet = ~std::uint64_t{0};

    [[nodiscard]] std::uint64_t begins() const noexcept
    {
        return begins_.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::uint64_t dropped() const noexcept
    {
        return dropped_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] std::size_t waiting() const noexcept;

    void restart() noexcept;

private:

    struct Run {
        std::uint64_t start{0};

        std::atomic<std::uint64_t> count{0};

        std::uint64_t lostBefore{0};
    };

    std::vector<std::int16_t> samples_;
    std::vector<Run> runs_;

    const std::size_t mask_;
    const std::size_t runMask_;

    std::atomic<std::uint64_t> written_{0};
    std::atomic<std::uint64_t> taken_{0};

    std::atomic<std::uint64_t> published_{0};
    std::atomic<std::uint64_t> consumed_{0};

    std::uint64_t offset_{0};

    std::uint64_t pending_{0};

    std::atomic<std::uint64_t> clock_{0};
    std::atomic<std::uint64_t> dropped_{0};
    std::atomic<std::uint64_t> begins_{kNothingYet};
};

}
