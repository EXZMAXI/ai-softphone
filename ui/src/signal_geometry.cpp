// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "signal_geometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <vector>

namespace softphone::ui {

namespace {

constexpr double kOffTheEdge = 12.0;

}

double shaped(double level) noexcept
{
    return std::pow(std::clamp(level, 0.0, 1.0), 0.85);
}

double noiseFloor(std::span<const double> trail) noexcept
{
    if (trail.empty()) {
        return 0.0;
    }

    const auto from = trail.size() > kFloorWindow ? trail.size() - kFloorWindow : std::size_t{0};
    const auto recent = trail.subspan(from);

    std::array<double, kFloorWindow> sorted{};
    const auto count = std::min(recent.size(), sorted.size());
    std::copy_n(recent.begin(), count, sorted.begin());

    const auto at = sorted.begin() + static_cast<std::ptrdiff_t>(count / 10);
    std::nth_element(sorted.begin(), at, sorted.begin() + static_cast<std::ptrdiff_t>(count));
    return *at;
}

double aboveTheFloor(double level, double floor) noexcept
{

    if (level <= 0.0) {
        return 0.0;
    }

    constexpr double kMargin = 2.0 / 60.0;

    constexpr double kCeiling = 0.9;

    const double from = std::min(std::clamp(floor, 0.0, 1.0) + kMargin, kCeiling - 0.05);
    const double above = std::clamp((level - from) / (kCeiling - from), 0.0, 1.0);
    return kFloorShows + (1.0 - kFloorShows) * above;
}

Strip stripFor(double width, std::size_t columns, double slide) noexcept
{
    Strip strip;
    if (width <= 0.0 || columns < 2) {
        return strip;
    }

    const double spread = std::max(1.0, static_cast<double>(columns) - kOffTheEdge);
    strip.step = width / spread;
    strip.slide = std::clamp(slide, 0.0, 1.0) * strip.step;
    strip.barWidth = std::max(1.5, strip.step * 0.62);
    return strip;
}

double columnX(const Strip &strip, double width, std::size_t index, std::size_t count) noexcept
{
    if (count == 0) {
        return width;
    }
    const auto age = static_cast<double>(count - 1 - std::min(index, count - 1));
    return width - strip.slide - age * strip.step;
}

double columnAlpha(double x, double width) noexcept
{
    if (width <= 0.0) {
        return 1.0;
    }
    return 0.25 + 0.75 * std::min(1.0, x / width * 1.6);
}

namespace {

template<typename Number>
[[nodiscard]] double floorOf(std::span<const Number> bars)
{
    std::vector<double> carrying;
    carrying.reserve(bars.size());
    for (const auto one : bars) {
        if (one > Number{0}) {
            carrying.push_back(static_cast<double>(one));
        }
    }
    if (carrying.empty()) {
        return 0.0;
    }

    const auto at = carrying.begin() + static_cast<std::ptrdiff_t>(carrying.size() / 10);
    std::nth_element(carrying.begin(), at, carrying.end());
    return *at;
}

template<typename Number>
void raise(std::span<Number> bars)
{
    const double floor = floorOf(std::span<const Number>{bars});
    if (floor <= 0.0) {
        return;
    }
    for (auto &one : bars) {
        one = static_cast<Number>(aboveTheFloor(static_cast<double>(one), floor));
    }
}

}

void raiseOffTheNoiseFloor(std::span<double> bars) noexcept
{
    raise(bars);
}

void raiseOffTheNoiseFloor(std::span<float> bars) noexcept
{
    raise(bars);
}

}
