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
#include <softphone/core/resample.hpp>
#include <softphone/core/streaming_asr.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace softphone::core {

class RealtimeReader {
public:

    RealtimeReader(RealtimeRing &ring, unsigned sourceRate, AsrWireFormat wire,
                   std::chrono::milliseconds leastPerMessage = std::chrono::milliseconds::zero());

    struct Pumped {

        std::size_t sent{0};

        std::chrono::milliseconds lost{0};

        std::chrono::milliseconds lostAt{0};

        bool refused{false};
    };

    [[nodiscard]] Pumped pump(IStreamingAsrSession &into);

    bool quiet(IStreamingAsrSession &into, std::chrono::milliseconds upTo);

    bool flush(IStreamingAsrSession &into);

    [[nodiscard]] std::chrono::milliseconds sessionClock(std::uint64_t position) const noexcept;

    [[nodiscard]] std::uint64_t sentSamples() const noexcept
    {
        return sent_;
    }

    [[nodiscard]] std::uint64_t hushSamples() const noexcept
    {
        return hush_sent_;
    }

    [[nodiscard]] std::chrono::milliseconds lostAltogether() const noexcept;

private:

    bool offer(IStreamingAsrSession &into, std::span<const std::int16_t> samples,
               std::chrono::milliseconds at);

    RealtimeRing &ring_;
    unsigned sourceRate_;
    AsrWireFormat wire_;

    ResampleState held_;

    std::vector<std::int16_t> read_;
    std::vector<std::int16_t> converted_;

    std::vector<std::int16_t> hush_;

    std::chrono::milliseconds least_{0};
    std::vector<std::int16_t> waiting_;
    std::chrono::milliseconds waitingAt_{0};

    std::chrono::milliseconds reached_{0};

    std::chrono::milliseconds heard_{0};

    bool justHeard_{false};

    std::chrono::milliseconds base_{0};

    mutable std::uint64_t origin_{RealtimeRing::kNothingYet};

    std::uint64_t sent_{0};
    std::uint64_t hush_sent_{0};
    std::uint64_t lost_{0};
};

}
