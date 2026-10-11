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

#include <softphone/core/recordings.hpp>

#include <chrono>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace softphone::core {

struct RetentionPolicy {

    int maxAgeDays{0};

    int maxSizeMegabytes{0};

    int dropStemsAfterDays{0};

    int freeSpaceFloorMegabytes{500};

    friend bool operator==(const RetentionPolicy &, const RetentionPolicy &) = default;
};

struct RetentionVerdict {

    std::vector<std::string> dropAudio;

    std::vector<std::string> dropStems;

    std::uint64_t bytes{0};
    std::uint64_t bytesAfter{0};

    bool pinnedOverTheCeiling{false};
};

[[nodiscard]] RetentionVerdict sweepRecordings(std::span<const Recording> recordings,
                                               const RetentionPolicy &policy,
                                               std::chrono::system_clock::time_point now);

inline constexpr int kMaxRetentionDays = 3650;
inline constexpr int kMaxLibraryMegabytes = 4'000'000;
inline constexpr int kMinFreeSpaceMegabytes = 100;
inline constexpr int kMaxFreeSpaceMegabytes = 100'000;

}
