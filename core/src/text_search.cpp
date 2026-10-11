// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/text_search.hpp>

#include <array>
#include <cstdint>

namespace softphone::core {
namespace {

struct Decoded {
    char32_t code{0};
    std::size_t length{1};
};

[[nodiscard]] Decoded decode(std::string_view text, std::size_t at) noexcept
{
    const auto byte = static_cast<unsigned char>(text[at]);
    if (byte < 0x80) {
        return {byte, 1};
    }

    const auto continues = [&text](std::size_t which) {
        return which < text.size() && (static_cast<unsigned char>(text[which]) & 0xC0) == 0x80;
    };
    const auto tail = [&text](std::size_t which) {
        return static_cast<char32_t>(static_cast<unsigned char>(text[which]) & 0x3F);
    };

    if ((byte & 0xE0) == 0xC0 && continues(at + 1)) {
        return {(static_cast<char32_t>(byte & 0x1F) << 6) | tail(at + 1), 2};
    }
    if ((byte & 0xF0) == 0xE0 && continues(at + 1) && continues(at + 2)) {
        return {(static_cast<char32_t>(byte & 0x0F) << 12) | (tail(at + 1) << 6) | tail(at + 2), 3};
    }
    if ((byte & 0xF8) == 0xF0 && continues(at + 1) && continues(at + 2) && continues(at + 3)) {
        return {(static_cast<char32_t>(byte & 0x07) << 18) | (tail(at + 1) << 12) |
                    (tail(at + 2) << 6) | tail(at + 3),
                4};
    }
    return {0, 1};
}

void append(std::string &into, char32_t code)
{
    if (code < 0x80) {
        into.push_back(static_cast<char>(code));
        return;
    }
    if (code < 0x800) {
        into.push_back(static_cast<char>(0xC0 | (code >> 6)));
        into.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        return;
    }
    if (code < 0x10000) {
        into.push_back(static_cast<char>(0xE0 | (code >> 12)));
        into.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
        into.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        return;
    }
    into.push_back(static_cast<char>(0xF0 | (code >> 18)));
    into.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
    into.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
    into.push_back(static_cast<char>(0x80 | (code & 0x3F)));
}

[[nodiscard]] char32_t foldOne(char32_t code) noexcept
{
    if (code >= U'A' && code <= U'Z') {
        return code + 32;
    }

    if (code >= 0x00C0 && code <= 0x00DE && code != 0x00D7) {
        return code + 32;
    }

    if (code >= 0x0100 && code <= 0x0137) {
        return (code % 2 == 0) ? code + 1 : code;
    }
    if (code >= 0x0139 && code <= 0x0148) {
        return (code % 2 != 0) ? code + 1 : code;
    }
    if (code >= 0x014A && code <= 0x0177) {
        return (code % 2 == 0) ? code + 1 : code;
    }

    if (code == 0x0178) {
        return 0x00FF;
    }
    if (code >= 0x0179 && code <= 0x017E) {
        return (code % 2 != 0) ? code + 1 : code;
    }

    if (code >= 0x0391 && code <= 0x03A9 && code != 0x03A2) {
        return code + 32;
    }

    if (code >= 0x0400 && code <= 0x040F) {
        return code + 80;
    }

    if (code >= 0x0410 && code <= 0x042F) {
        return code + 32;
    }

    return code;
}

}

std::string casefolded(std::string_view text)
{
    std::string answer;
    answer.reserve(text.size());

    for (std::size_t at = 0; at < text.size();) {
        const auto one = decode(text, at);
        if (one.code == 0 && one.length == 1) {

            answer.push_back(text[at]);
            at += 1;
            continue;
        }
        append(answer, foldOne(one.code));
        at += one.length;
    }

    return answer;
}

std::string tidiedLine(std::string_view text)
{
    const auto blank = [](char letter) { return static_cast<unsigned char>(letter) <= ' '; };

    std::size_t from = 0;
    while (from < text.size() && blank(text[from])) {
        ++from;
    }
    auto to = text.size();
    while (to > from && blank(text[to - 1])) {
        --to;
    }
    return std::string{text.substr(from, to - from)};
}

}
