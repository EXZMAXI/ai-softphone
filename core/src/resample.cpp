// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/resample.hpp>

#include <algorithm>
#include <cstdlib>

namespace softphone::core {
namespace {

constexpr std::uint64_t kOne = std::uint64_t{1} << 32;

}

std::size_t resampledLength(std::size_t count, unsigned from, unsigned to) noexcept
{
    if (from == 0 || to == 0 || from == to) {
        return count;
    }
    return static_cast<std::size_t>(static_cast<std::uint64_t>(count) * to / from);
}

void resample(ResampleState &held, std::span<const std::int16_t> samples, unsigned from,
              unsigned to, std::vector<std::int16_t> &out)
{
    if (samples.empty()) {
        return;
    }

    if (from == to || from == 0 || to == 0) {
        out.insert(out.end(), samples.begin(), samples.end());
        return;
    }

    const bool decimating = to < from;

    const auto step = static_cast<std::uint64_t>(
        (static_cast<double>(from) / static_cast<double>(to)) * static_cast<double>(kOne));

    if (!held.started) {
        held.started = true;
        held.previous = samples.front();
        held.smoothed = samples.front();
        held.position = 0;
    }

    std::vector<std::int16_t> smoothed;
    std::span<const std::int16_t> source = samples;
    if (decimating) {
        smoothed.reserve(samples.size());
        for (const auto sample : samples) {
            held.smoothed = (held.smoothed + sample) / 2;
            smoothed.push_back(static_cast<std::int16_t>(held.smoothed));
        }
        source = smoothed;
    }

    while (held.position < static_cast<std::uint64_t>(source.size()) * kOne) {
        const auto whole = static_cast<std::size_t>(held.position / kOne);
        const auto part = held.position % kOne;

        const std::int32_t before =
            whole == 0 ? held.previous : static_cast<std::int32_t>(source[whole - 1]);
        const auto after = static_cast<std::int32_t>(source[whole]);

        const auto mixed =
            before + static_cast<std::int32_t>((static_cast<std::int64_t>(after - before) *
                                                static_cast<std::int64_t>(part)) >>
                                               32);
        out.push_back(static_cast<std::int16_t>(std::clamp(mixed, -32768, 32767)));

        held.position += step;
    }

    held.previous = source.back();
    held.position -= static_cast<std::uint64_t>(source.size()) * kOne;
}

std::uint8_t asMulaw(std::int16_t sample) noexcept
{

    constexpr std::int32_t kBias = 0x84;
    constexpr std::int32_t kClip = 32635;

    auto value = static_cast<std::int32_t>(sample);
    const auto sign = static_cast<std::uint8_t>(value < 0 ? 0x80 : 0x00);
    if (value < 0) {
        value = -value;
    }
    value = std::min(value, kClip);
    value += kBias;

    std::uint8_t exponent = 7;
    for (std::int32_t mask = 0x4000; (value & mask) == 0 && exponent > 0; mask >>= 1) {
        --exponent;
    }

    const auto mantissa = static_cast<std::uint8_t>((value >> (exponent + 3)) & 0x0F);
    return static_cast<std::uint8_t>(~(sign | static_cast<std::uint8_t>(exponent << 4) | mantissa));
}

void toMulaw(std::span<const std::int16_t> samples, std::vector<std::uint8_t> &out)
{
    out.reserve(out.size() + samples.size());
    for (const auto sample : samples) {
        out.push_back(asMulaw(sample));
    }
}

}
