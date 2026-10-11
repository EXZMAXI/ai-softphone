// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/level_meter.hpp>

#include <algorithm>

namespace softphone::core {

void LevelMeter::observe(unsigned level) noexcept
{
    const auto scaled =
        static_cast<double>(std::min(level, kLoudestLevel)) / static_cast<double>(kLoudestLevel);

    reading_ = std::max(scaled, reading_ - kFallPerReading);
}

}
