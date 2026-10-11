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
#include <span>
#include <vector>

namespace softphone::core {

struct ResampleState {

    std::uint64_t position{0};

    std::int16_t previous{0};

    std::int32_t smoothed{0};

    bool started{false};
};

[[nodiscard]] std::size_t resampledLength(std::size_t count, unsigned from, unsigned to) noexcept;

void resample(ResampleState &held, std::span<const std::int16_t> samples, unsigned from,
              unsigned to, std::vector<std::int16_t> &out);

[[nodiscard]] std::uint8_t asMulaw(std::int16_t sample) noexcept;

void toMulaw(std::span<const std::int16_t> samples, std::vector<std::uint8_t> &out);

}
