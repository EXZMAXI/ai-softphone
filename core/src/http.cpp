// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/http.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>

namespace softphone::core {
namespace {

[[nodiscard]] bool sameName(std::string_view left, std::string_view right) noexcept
{
    if (left.size() != right.size()) {
        return false;
    }
    return std::equal(left.begin(), left.end(), right.begin(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) ==
               std::tolower(static_cast<unsigned char>(b));
    });
}

[[nodiscard]] std::size_t wideBreakAt(std::string_view text, std::size_t at) noexcept
{
    const auto byte = [&](std::size_t where) { return static_cast<unsigned char>(text[where]); };
    if (at + 1 < text.size() && byte(at) == 0xC2 && byte(at + 1) == 0x85) {
        return 2;
    }
    if (at + 2 < text.size() && byte(at) == 0xE2 && byte(at + 1) == 0x80 &&
        (byte(at + 2) == 0xA8 || byte(at + 2) == 0xA9)) {
        return 3;
    }
    return 0;
}

[[nodiscard]] bool nameCharacter(unsigned char byte) noexcept
{
    if (std::isalnum(byte) != 0) {
        return true;
    }
    constexpr std::string_view kRest = "!#$%&'*+-.^_`|~";
    return kRest.find(static_cast<char>(byte)) != std::string_view::npos;
}

void percent(std::string &into, unsigned char byte)
{
    constexpr std::string_view kDigits = "0123456789ABCDEF";
    into.push_back('%');
    into.push_back(kDigits[byte >> 4U]);
    into.push_back(kDigits[byte & 0x0FU]);
}

}

std::string asOneLine(std::string_view text)
{
    std::string kept;
    kept.reserve(text.size());

    for (std::size_t at = 0; at < text.size();) {
        if (const auto wide = wideBreakAt(text, at); wide > 0) {
            at += wide;
            continue;
        }
        const auto byte = static_cast<unsigned char>(text[at]);
        if (byte == '\n' || byte == '\r' || byte == '\v' || byte == '\f') {
            ++at;
            continue;
        }
        kept.push_back(text[at]);
        ++at;
    }

    const auto first = kept.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = kept.find_last_not_of(" \t");
    return kept.substr(first, last - first + 1);
}

std::string headerSafeValue(std::string_view value)
{
    std::string safe;
    safe.reserve(value.size());
    for (const char letter : value) {
        const auto byte = static_cast<unsigned char>(letter);

        if (byte == '\t' || (byte >= 0x20 && byte != 0x7F)) {
            safe.push_back(letter);
            continue;
        }
        safe.push_back(' ');
    }
    return safe;
}

std::string headerSafeName(std::string_view name)
{
    std::string safe;
    safe.reserve(name.size());
    for (const char letter : name) {
        const auto byte = static_cast<unsigned char>(letter);
        if (nameCharacter(byte)) {
            safe.push_back(letter);
            continue;
        }
        percent(safe, byte);
    }
    return safe;
}

std::string_view toString(HttpVerb verb) noexcept
{
    switch (verb) {
    case HttpVerb::Get:
        return "GET";
    case HttpVerb::Post:
        return "POST";
    case HttpVerb::Put:
        return "PUT";
    case HttpVerb::Remove:
        return "DELETE";
    }
    return "POST";
}

std::string HttpResponse::header(std::string_view name) const
{
    for (const auto &[key, value] : headers) {
        if (sameName(key, name)) {
            return value;
        }
    }
    return {};
}

}
