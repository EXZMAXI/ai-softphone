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

#include <cstddef>
#include <span>

namespace softphone::ui {

[[nodiscard]] double shaped(double level) noexcept;

inline constexpr std::size_t kFloorWindow = 60;

[[nodiscard]] double noiseFloor(std::span<const double> trail) noexcept;

[[nodiscard]] double aboveTheFloor(double level, double floor) noexcept;

inline constexpr double kFloorShows = 0.06;

void raiseOffTheNoiseFloor(std::span<double> bars) noexcept;

void raiseOffTheNoiseFloor(std::span<float> bars) noexcept;

struct Strip {

    double step{0.0};

    double slide{0.0};

    double barWidth{0.0};
};

[[nodiscard]] Strip stripFor(double width, std::size_t columns, double slide) noexcept;

[[nodiscard]] double columnX(const Strip &strip, double width, std::size_t index,
                             std::size_t count) noexcept;

[[nodiscard]] double columnAlpha(double x, double width) noexcept;

}
