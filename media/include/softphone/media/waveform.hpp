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

#include <cstdint>
#include <filesystem>
#include <vector>

namespace softphone::media {

struct Waveform {

    std::vector<std::uint8_t> nearSide;
    std::vector<std::uint8_t> farSide;

    [[nodiscard]] bool empty() const noexcept
    {
        return nearSide.empty();
    }
};

inline constexpr unsigned kWaveformBucketMs = 40;

inline constexpr unsigned kWaveformBucketsMin = 2000;

inline constexpr unsigned kWaveformBucketsMax = 90000;

[[nodiscard]] unsigned waveformBucketsFor(std::uint64_t durationMs);

[[nodiscard]] bool waveformFor(const std::filesystem::path &file, Waveform &out);

[[nodiscard]] Waveform waveformOf(const std::vector<std::int16_t> &interleaved, unsigned channels,
                                  unsigned sampleRate);

[[nodiscard]] std::filesystem::path waveformCacheFor(const std::filesystem::path &file);

}
