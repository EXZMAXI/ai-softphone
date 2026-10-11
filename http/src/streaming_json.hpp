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

#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace softphone::http::streaming {

template<typename T>
[[nodiscard]] T readOr(const nlohmann::json &from, std::string_view name, T fallback)
{
    if (!from.is_object()) {
        return fallback;
    }
    const auto at = from.find(name);
    if (at == from.end() || at->is_null()) {
        return fallback;
    }
    try {
        return at->get<T>();
    } catch (const nlohmann::json::exception &) {
        return fallback;
    }
}

template<typename... Names>
[[nodiscard]] std::string firstString(const nlohmann::json &from, Names... names)
{
    std::string found;
    (
        [&] {
            if (found.empty()) {
                found = readOr<std::string>(from, names, {});
            }
        }(),
        ...);
    return found;
}

[[nodiscard]] inline std::chrono::milliseconds fromSeconds(double seconds) noexcept
{
    return std::chrono::milliseconds{static_cast<std::int64_t>(seconds * 1000.0)};
}

[[nodiscard]] inline std::chrono::milliseconds fromMilliseconds(double value) noexcept
{
    return std::chrono::milliseconds{static_cast<std::int64_t>(value)};
}

[[nodiscard]] inline std::string asSocket(std::string host, std::string_view path)
{

    if (host.empty()) {
        return {};
    }

    if (host.rfind("http://", 0) == 0) {
        host = "ws://" + host.substr(7);
    } else if (host.rfind("https://", 0) == 0) {
        host = "wss://" + host.substr(8);
    }
    while (!host.empty() && host.back() == '/') {
        host.pop_back();
    }

    const auto afterScheme = host.find("://");
    const auto authority = afterScheme == std::string::npos ? 0 : afterScheme + 3;
    if (host.find('/', authority) != std::string::npos) {
        return host;
    }
    return host + std::string{path};
}

[[nodiscard]] inline bool outOfCredit(std::string_view body)
{
    std::string folded;
    folded.reserve(body.size());
    for (const char letter : body) {
        folded.push_back(letter >= 'A' && letter <= 'Z' ? static_cast<char>(letter - 'A' + 'a')
                                                        : letter);
    }

    constexpr std::string_view saying[]{"insufficient_quota",
                                        "insufficient quota",
                                        "no credits",
                                        "out of credits",
                                        "exceeded your current quota",
                                        "quota exceeded",
                                        "billing",
                                        "payment required",
                                        "add funds",
                                        "top up"};
    for (const auto words : saying) {
        if (folded.find(words) != std::string::npos) {
            return true;
        }
    }
    return false;
}

}
