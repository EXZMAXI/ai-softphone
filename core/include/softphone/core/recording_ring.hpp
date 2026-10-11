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

class RecordingRing {
public:

    static constexpr std::uint64_t kNothingYet = ~std::uint64_t{0};

    struct Frame {
        std::uint64_t position{0};
        std::size_t count{0};
    };

    RecordingRing(std::size_t samples, std::size_t frames);

    RecordingRing(const RecordingRing &) = delete;
    RecordingRing &operator=(const RecordingRing &) = delete;
    RecordingRing(RecordingRing &&) = delete;
    RecordingRing &operator=(RecordingRing &&) = delete;
    ~RecordingRing() = default;

    void write(std::uint64_t position, const std::int16_t *samples, std::size_t count) noexcept;

    [[nodiscard]] bool read(Frame &frame, std::span<std::int16_t> out) noexcept;

    [[nodiscard]] std::uint64_t begins() const noexcept
    {
        return begins_.load(std::memory_order_acquire);
    }

    void restart() noexcept;

    [[nodiscard]] std::uint64_t clock() const noexcept
    {
        return clock_.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::uint64_t dropped() const noexcept
    {
        return dropped_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] std::size_t waiting() const noexcept;

private:
    struct Slot {
        std::uint64_t position{0};
        std::uint64_t start{0};
        std::size_t count{0};
    };

    std::vector<std::int16_t> samples_;
    std::vector<Slot> slots_;

    const std::size_t sampleMask_;
    const std::size_t slotMask_;

    std::atomic<std::uint64_t> written_{0};
    std::atomic<std::uint64_t> taken_{0};

    std::atomic<std::uint64_t> published_{0};
    std::atomic<std::uint64_t> consumed_{0};

    std::atomic<std::uint64_t> clock_{0};
    std::atomic<std::uint64_t> dropped_{0};
    std::atomic<std::uint64_t> begins_{kNothingYet};
};

}
