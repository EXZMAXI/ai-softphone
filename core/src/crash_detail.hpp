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

#include <array>
#include <atomic>
#include <cstddef>
#include <string_view>

namespace softphone::core::crashDetail {

inline constexpr std::size_t kBreadcrumbLength = 240;

inline constexpr std::size_t kBreadcrumbCount = 128;

struct Breadcrumbs {
    struct Line {
        std::atomic<std::uint64_t> sequence{0};
        std::array<char, kBreadcrumbLength> text{};
    };

    std::array<Line, kBreadcrumbCount> lines{};
    std::atomic<std::uint64_t> next{1};
};

[[nodiscard]] Breadcrumbs &breadcrumbs() noexcept;

inline constexpr std::size_t kPathLength = 512;

struct Destination {
    std::array<char, kPathLength> directory{};
    std::size_t directoryLength{0};

    std::array<char, kPathLength> report{};
    std::array<char, kPathLength> dump{};
};

[[nodiscard]] Destination &destination() noexcept;

void nameTheFiles() noexcept;

std::size_t append(std::array<char, kPathLength> &into, std::size_t at,
                   std::string_view text) noexcept;

[[nodiscard]] std::string_view banner() noexcept;
void prepareBanner() noexcept;

void writeReport(int descriptor, std::string_view what) noexcept;

[[nodiscard]] std::string_view markerName() noexcept;

void rememberPreviousRun();

}
