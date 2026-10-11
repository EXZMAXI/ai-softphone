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
#include <cstdint>
#include <span>
#include <string>

namespace softphone::http::streaming {

[[nodiscard]] inline std::string asBase64(std::span<const std::byte> bytes)
{
    static constexpr std::string_view kAlphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string out;
    out.reserve((bytes.size() + 2) / 3 * 4);

    std::size_t at = 0;
    while (at + 3 <= bytes.size()) {
        const auto three = (static_cast<std::uint32_t>(bytes[at]) << 16) |
                           (static_cast<std::uint32_t>(bytes[at + 1]) << 8) |
                           static_cast<std::uint32_t>(bytes[at + 2]);
        out.push_back(kAlphabet[(three >> 18) & 0x3F]);
        out.push_back(kAlphabet[(three >> 12) & 0x3F]);
        out.push_back(kAlphabet[(three >> 6) & 0x3F]);
        out.push_back(kAlphabet[three & 0x3F]);
        at += 3;
    }

    if (const auto left = bytes.size() - at; left == 1) {
        const auto one = static_cast<std::uint32_t>(bytes[at]) << 16;
        out.push_back(kAlphabet[(one >> 18) & 0x3F]);
        out.push_back(kAlphabet[(one >> 12) & 0x3F]);
        out.push_back('=');
        out.push_back('=');
    } else if (left == 2) {
        const auto two = (static_cast<std::uint32_t>(bytes[at]) << 16) |
                         (static_cast<std::uint32_t>(bytes[at + 1]) << 8);
        out.push_back(kAlphabet[(two >> 18) & 0x3F]);
        out.push_back(kAlphabet[(two >> 12) & 0x3F]);
        out.push_back(kAlphabet[(two >> 6) & 0x3F]);
        out.push_back('=');
    }

    return out;
}

}
