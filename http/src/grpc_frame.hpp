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

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::http::grpcwire {

constexpr std::size_t kHeader = 5;

inline void frame(std::string &into, std::string_view message)
{
    into.push_back('\0');
    const auto length = static_cast<std::uint32_t>(message.size());
    into.push_back(static_cast<char>((length >> 24) & 0xFFU));
    into.push_back(static_cast<char>((length >> 16) & 0xFFU));
    into.push_back(static_cast<char>((length >> 8) & 0xFFU));
    into.push_back(static_cast<char>(length & 0xFFU));
    into.append(message);
}

[[nodiscard]] inline std::string framed(std::string_view message)
{
    std::string out;
    out.reserve(message.size() + kHeader);
    frame(out, message);
    return out;
}

struct Taken {

    std::vector<std::string> messages;

    bool compressed{false};
};

[[nodiscard]] inline Taken take(std::string &buffer)
{
    Taken took;
    std::size_t at = 0;
    for (;;) {
        if (buffer.size() - at < kHeader) {
            break;
        }
        if (static_cast<std::uint8_t>(buffer[at]) != 0) {
            took.compressed = true;
            break;
        }
        const auto length =
            (static_cast<std::uint32_t>(static_cast<std::uint8_t>(buffer[at + 1])) << 24) |
            (static_cast<std::uint32_t>(static_cast<std::uint8_t>(buffer[at + 2])) << 16) |
            (static_cast<std::uint32_t>(static_cast<std::uint8_t>(buffer[at + 3])) << 8) |
            static_cast<std::uint32_t>(static_cast<std::uint8_t>(buffer[at + 4]));
        if (buffer.size() - at - kHeader < length) {
            break;
        }
        took.messages.emplace_back(buffer.substr(at + kHeader, length));
        at += kHeader + length;
    }
    buffer.erase(0, at);
    return took;
}

}
