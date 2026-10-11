// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/dial_link.hpp>

#include <softphone/core/call.hpp>

#include <algorithm>
#include <cctype>

namespace softphone::core {
namespace {

[[nodiscard]] char lowered(char character) noexcept
{
    return static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
}

[[nodiscard]] std::string_view trimmed(std::string_view text)
{
    const auto space = [](char character) { return static_cast<unsigned char>(character) <= ' '; };
    while (!text.empty() && space(text.front())) {
        text.remove_prefix(1);
    }
    while (!text.empty() && space(text.back())) {
        text.remove_suffix(1);
    }
    return text;
}

[[nodiscard]] int hexDigit(char character) noexcept
{
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    const auto small = lowered(character);
    if (small >= 'a' && small <= 'f') {
        return small - 'a' + 10;
    }
    return -1;
}

[[nodiscard]] std::string_view parameter(std::string_view parameters, std::string_view name)
{
    while (!parameters.empty()) {
        const auto end = parameters.find(';');
        const auto one = parameters.substr(0, end);
        const auto equals = one.find('=');
        if (equals != std::string_view::npos) {
            const auto key = one.substr(0, equals);
            const auto same = key.size() == name.size() &&
                              std::equal(key.begin(), key.end(), name.begin(),
                                         [](char a, char b) { return lowered(a) == lowered(b); });
            if (same) {
                return one.substr(equals + 1);
            }
        }
        if (end == std::string_view::npos) {
            break;
        }
        parameters.remove_prefix(end + 1);
    }
    return {};
}

}

std::string percentDecoded(std::string_view text)
{
    std::string out;
    out.reserve(text.size());

    for (std::size_t at = 0; at < text.size(); ++at) {
        if (text[at] == '%' && at + 2 < text.size()) {
            const auto high = hexDigit(text[at + 1]);
            const auto low = hexDigit(text[at + 2]);
            if (high >= 0 && low >= 0) {
                out.push_back(static_cast<char>(high * 16 + low));
                at += 2;
                continue;
            }
        }
        out.push_back(text[at]);
    }
    return out;
}

DialLink dialLink(std::string_view uri)
{
    const auto text = trimmed(uri);
    const auto colon = text.find(':');
    if (colon == std::string_view::npos || colon == 0) {
        return {};
    }

    const auto named = text.substr(0, colon);
    const auto knows = [&named](std::string_view scheme) {
        return named.size() == scheme.size() &&
               std::equal(named.begin(), named.end(), scheme.begin(),
                          [](char a, char b) { return lowered(a) == b; });
    };
    const auto found = std::find_if(kDialSchemes.begin(), kDialSchemes.end(), knows);
    if (found == kDialSchemes.end()) {
        return {};
    }

    DialLink link;
    link.ours = true;

    auto rest = text.substr(colon + 1);

    if (*found == "sip" || *found == "sips") {
        rest = rest.substr(0, rest.find('?'));
        if (rest.empty()) {
            return link;
        }
        link.target = std::string{named} + ':' + std::string{rest};
        return link;
    }

    if (*found == "callto" && rest.starts_with("//")) {
        rest.remove_prefix(2);
    }

    const auto semicolon = rest.find(';');
    const auto number = percentDecoded(rest.substr(0, semicolon));
    const auto parameters =
        semicolon == std::string_view::npos ? std::string_view{} : rest.substr(semicolon + 1);

    if (!number.empty() && number.front() != '+') {
        const auto context = percentDecoded(parameter(parameters, "phone-context"));
        if (context.starts_with("+")) {
            link.target = dialable(context + number);
            return link;
        }
    }

    link.target = dialable(number);
    return link;
}

}
