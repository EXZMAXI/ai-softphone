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

#include <chrono>

namespace softphone::core {

inline constexpr unsigned kLoudestLevel = 255;

inline constexpr std::chrono::milliseconds kLevelMeterInterval{50};

struct AudioLevels {

    unsigned capture{0};

    unsigned playback{0};
};

class LevelMeter {
public:

    void observe(unsigned level) noexcept;

    [[nodiscard]] double reading() const noexcept
    {
        return reading_;
    }

    void reset() noexcept
    {
        reading_ = 0.0;
    }

private:

    static constexpr double kFallPerReading = 0.06;

    double reading_{0.0};
};

}
