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

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace softphone::core {

inline constexpr unsigned kRehearsalRate = 16000;

class RehearsalFeed {
public:

    enum class Sides {

        NearAndFar,

        FarOnly,
    };

    explicit RehearsalFeed(unsigned rate);

    RehearsalFeed(const RehearsalFeed &) = delete;
    RehearsalFeed &operator=(const RehearsalFeed &) = delete;
    RehearsalFeed(RehearsalFeed &&) = delete;
    RehearsalFeed &operator=(RehearsalFeed &&) = delete;
    ~RehearsalFeed();

    [[nodiscard]] RealtimeRing *nearListening() noexcept;
    [[nodiscard]] RealtimeRing *farListening() noexcept;

    [[nodiscard]] unsigned sampleRate() const noexcept
    {
        return rate_;
    }

    void load(const std::vector<std::int16_t> &interleaved, unsigned channels, unsigned rate,
              Sides sides);

    [[nodiscard]] bool ready() const noexcept;

    [[nodiscard]] std::chrono::milliseconds length() const noexcept;

    void upTo(std::chrono::milliseconds position);

    void rewind();

    [[nodiscard]] std::uint64_t sentSamples() const noexcept;

    [[nodiscard]] std::uint64_t skippedSamples() const noexcept;

private:
    struct Held;
    std::unique_ptr<Held> held_;
    unsigned rate_;
};

}
