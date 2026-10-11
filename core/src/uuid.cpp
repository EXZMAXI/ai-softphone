// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/uuid.hpp>

#include <array>
#include <random>

namespace softphone::core {
namespace {

[[nodiscard]] std::mt19937_64 &engine()
{
    thread_local std::mt19937_64 generator{std::random_device{}()};
    return generator;
}

constexpr std::array<char, 16> kHex{'0', '1', '2', '3', '4', '5', '6', '7',
                                    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

[[nodiscard]] bool isHex(char character) noexcept
{
    return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f');
}

}

std::string makeUuid()
{
    std::uniform_int_distribution<std::uint64_t> bits;
    auto high = bits(engine());
    auto low = bits(engine());

    high = (high & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    low = (low & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    std::string text;
    text.reserve(36);

    const auto append = [&text](std::uint64_t value, int nibbles) {
        for (int index = nibbles - 1; index >= 0; --index) {
            text.push_back(kHex.at((value >> (index * 4)) & 0xF));
        }
    };

    append(high >> 32, 8);
    text.push_back('-');
    append((high >> 16) & 0xFFFF, 4);
    text.push_back('-');
    append(high & 0xFFFF, 4);
    text.push_back('-');
    append(low >> 48, 4);
    text.push_back('-');
    append(low & 0xFFFFFFFFFFFFULL, 12);

    return text;
}

bool isUuid(std::string_view text) noexcept
{
    if (text.size() != 36) {
        return false;
    }

    for (std::size_t index = 0; index < text.size(); ++index) {
        const bool hyphen = index == 8 || index == 13 || index == 18 || index == 23;
        if (hyphen) {
            if (text[index] != '-') {
                return false;
            }
        } else if (!isHex(text[index])) {
            return false;
        }
    }
    return true;
}

}
