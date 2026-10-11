// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/container/version.hpp>

#include <algorithm>
#include <charconv>
#include <vector>

namespace softphone::container {
namespace {

[[nodiscard]] std::vector<std::string_view> split(std::string_view text, char separator)
{
    std::vector<std::string_view> parts;
    std::size_t start = 0;
    while (true) {
        const auto found = text.find(separator, start);
        if (found == std::string_view::npos) {
            parts.push_back(text.substr(start));
            return parts;
        }
        parts.push_back(text.substr(start, found - start));
        start = found + 1;
    }
}

[[nodiscard]] std::optional<int> wholeNumber(std::string_view text)
{
    if (text.empty()) {
        return std::nullopt;
    }

    if (!std::ranges::all_of(text, [](char c) { return c >= '0' && c <= '9'; })) {
        return std::nullopt;
    }

    int value = 0;
    const auto *const end = text.data() + text.size();
    const auto result = std::from_chars(text.data(), end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] std::string_view trimmed(std::string_view text)
{
    const auto space = [](char c) { return c == ' ' || c == '\t'; };
    while (!text.empty() && space(text.front())) {
        text.remove_prefix(1);
    }
    while (!text.empty() && space(text.back())) {
        text.remove_suffix(1);
    }
    return text;
}

}

std::optional<Version> parseVersion(std::string_view text)
{
    text = trimmed(text);
    const auto parts = split(text, '.');
    if (parts.empty() || parts.size() > 3) {
        return std::nullopt;
    }

    Version version;
    int *const fields[] = {&version.major, &version.minor, &version.patch};
    for (std::size_t index = 0; index < parts.size(); ++index) {
        const auto number = wholeNumber(parts[index]);
        if (!number.has_value()) {
            return std::nullopt;
        }
        *fields[index] = *number;
    }
    return version;
}

std::string toString(const Version &version)
{
    return std::to_string(version.major) + '.' + std::to_string(version.minor) + '.' +
           std::to_string(version.patch);
}

bool VersionRange::allows(const Version &version) const
{
    if (atLeast.has_value() && version < *atLeast) {
        return false;
    }
    if (below.has_value() && !(version < *below)) {
        return false;
    }
    return true;
}

std::optional<VersionRange> parseVersionRange(std::string_view text)
{
    VersionRange range;
    for (const auto clause : split(text, ' ')) {
        const auto piece = trimmed(clause);
        if (piece.empty()) {
            continue;
        }

        if (piece.starts_with(">=")) {
            if (range.atLeast.has_value()) {
                return std::nullopt;
            }
            const auto version = parseVersion(piece.substr(2));
            if (!version.has_value()) {
                return std::nullopt;
            }
            range.atLeast = version;
            continue;
        }
        if (piece.starts_with('<')) {
            if (range.below.has_value()) {
                return std::nullopt;
            }
            const auto version = parseVersion(piece.substr(1));
            if (!version.has_value()) {
                return std::nullopt;
            }
            range.below = version;
            continue;
        }
        return std::nullopt;
    }

    if (range.atLeast.has_value() && range.below.has_value() && !(*range.atLeast < *range.below)) {
        return std::nullopt;
    }
    return range;
}

std::string toString(const VersionRange &range)
{
    std::string text;
    if (range.atLeast.has_value()) {
        text += ">=" + toString(*range.atLeast);
    }
    if (range.below.has_value()) {
        if (!text.empty()) {
            text += ' ';
        }
        text += '<' + toString(*range.below);
    }
    return text;
}

}
