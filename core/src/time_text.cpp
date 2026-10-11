// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/time_text.hpp>

#include <array>
#include <ctime>
#include <type_traits>

namespace softphone::core {
namespace {

[[nodiscard]] std::tm broken(std::chrono::seconds sinceEpoch) noexcept
{
    using namespace std::chrono;

    const auto whole = floor<days>(sinceEpoch);
    const sys_days midnight{whole};
    const year_month_day date{midnight};

    const hh_mm_ss timeOfDay{sinceEpoch - whole};

    std::tm parts{};
    parts.tm_year = int{date.year()} - 1900;
    parts.tm_mon = static_cast<int>(static_cast<unsigned>(date.month())) - 1;
    parts.tm_mday = static_cast<int>(static_cast<unsigned>(date.day()));
    parts.tm_hour = static_cast<int>(timeOfDay.hours().count());
    parts.tm_min = static_cast<int>(timeOfDay.minutes().count());
    parts.tm_sec = static_cast<int>(timeOfDay.seconds().count());
    parts.tm_wday = static_cast<int>(weekday{midnight}.c_encoding());
    parts.tm_yday = static_cast<int>((midnight - sys_days{date.year() / January / 1}).count());

    parts.tm_isdst = -1;

    return parts;
}

#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
#define SOFTPHONE_HAS_TZDB 1
#else
#define SOFTPHONE_HAS_TZDB 0
#endif

#if SOFTPHONE_HAS_TZDB

[[nodiscard]] const std::chrono::time_zone *localZone() noexcept
{
    static const auto *zone = []() noexcept -> const std::chrono::time_zone * {
        try {
            return std::chrono::current_zone();
        } catch (...) {
            return nullptr;
        }
    }();

    return zone;
}

#endif

}

std::tm utcParts(std::chrono::system_clock::time_point when) noexcept
{
    return broken(std::chrono::floor<std::chrono::seconds>(when.time_since_epoch()));
}

std::tm localParts(std::chrono::system_clock::time_point when) noexcept
{
#if SOFTPHONE_HAS_TZDB
    const auto *zone = localZone();
    if (zone == nullptr) {
        return utcParts(when);
    }

    try {
        const auto local = zone->to_local(std::chrono::floor<std::chrono::seconds>(when));
        return broken(local.time_since_epoch());
    } catch (...) {

        return utcParts(when);
    }
#else

    const auto seconds = std::chrono::floor<std::chrono::seconds>(when.time_since_epoch()).count();
    const auto asTimeT = static_cast<std::time_t>(seconds);

    if (static_cast<std::remove_const_t<decltype(seconds)>>(asTimeT) != seconds) {
        return utcParts(when);
    }

    std::tm parts{};
#ifdef _WIN32
    if (::localtime_s(&parts, &asTimeT) != 0) {
        return utcParts(when);
    }
#else
    if (::localtime_r(&asTimeT, &parts) == nullptr) {
        return utcParts(when);
    }
#endif
    return parts;
#endif
}

std::string isoUtc(std::chrono::system_clock::time_point when)
{
    const std::tm parts = utcParts(when);

    std::array<char, 32> text{};
    const auto written = std::strftime(text.data(), text.size(), "%Y-%m-%dT%H:%M:%SZ", &parts);
    return std::string{text.data(), written};
}

std::string unixMilliseconds(std::chrono::system_clock::time_point when)
{
    if (when == std::chrono::system_clock::time_point{}) {
        return {};
    }
    return std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(when.time_since_epoch()).count());
}

}
