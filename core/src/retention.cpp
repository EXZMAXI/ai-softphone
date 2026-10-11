// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/retention.hpp>

#include <algorithm>

namespace softphone::core {
namespace {

[[nodiscard]] bool olderThan(const Recording &recording, std::chrono::system_clock::time_point now,
                             int days)
{
    if (days <= 0) {
        return false;
    }
    return recording.startedAt + std::chrono::hours{24} * days < now;
}

[[nodiscard]] std::uint64_t onTheDisk(const Recording &recording) noexcept
{
    std::uint64_t total = recording.bytes;
    for (const RecordingStem &stem : recording.stems) {
        if (stem.droppedAt.time_since_epoch().count() == 0 && !stem.filePath.empty()) {
            total += stem.bytes;
        }
    }
    return total;
}

}

RetentionVerdict sweepRecordings(std::span<const Recording> recordings,
                                 const RetentionPolicy &policy,
                                 std::chrono::system_clock::time_point now)
{
    RetentionVerdict verdict;

    std::vector<const Recording *> living;
    for (const Recording &recording : recordings) {

        if (!recording.hasAudio()) {
            continue;
        }
        living.push_back(&recording);
        verdict.bytes += onTheDisk(recording);
    }
    std::sort(living.begin(), living.end(), [](const Recording *one, const Recording *other) {
        return one->startedAt < other->startedAt;
    });

    std::vector<bool> going(living.size(), false);
    std::vector<bool> stemsGoing(living.size(), false);

    for (std::size_t at = 0; at < living.size(); ++at) {
        const Recording &recording = *living[at];
        if (recording.pinned) {
            continue;
        }
        if (olderThan(recording, now, policy.maxAgeDays)) {
            going[at] = true;
        }
    }

    for (std::size_t at = 0; at < living.size(); ++at) {
        const Recording &recording = *living[at];
        if (going[at] || recording.pinned || recording.stems.empty()) {
            continue;
        }
        if (!olderThan(recording, now, policy.dropStemsAfterDays)) {
            continue;
        }

        const bool anyLeft = std::any_of(
            recording.stems.begin(), recording.stems.end(), [](const RecordingStem &stem) {
                return stem.droppedAt.time_since_epoch().count() == 0 && !stem.filePath.empty();
            });
        stemsGoing[at] = anyLeft;
    }

    std::uint64_t left = 0;
    for (std::size_t at = 0; at < living.size(); ++at) {
        if (going[at]) {
            continue;
        }
        left += stemsGoing[at] ? living[at]->bytes : onTheDisk(*living[at]);
    }

    const auto ceiling =
        static_cast<std::uint64_t>(std::max(0, policy.maxSizeMegabytes)) * 1024 * 1024;
    if (ceiling > 0) {
        for (std::size_t at = 0; at < living.size() && left > ceiling; ++at) {
            if (going[at] || living[at]->pinned) {
                continue;
            }
            going[at] = true;
            left -= std::min(left, stemsGoing[at] ? living[at]->bytes : onTheDisk(*living[at]));
        }

        verdict.pinnedOverTheCeiling = left > ceiling;
    }

    for (std::size_t at = 0; at < living.size(); ++at) {
        if (going[at]) {
            verdict.dropAudio.push_back(living[at]->id);
            continue;
        }

        if (stemsGoing[at]) {
            verdict.dropStems.push_back(living[at]->id);
        }
    }

    verdict.bytesAfter = left;
    return verdict;
}

}
