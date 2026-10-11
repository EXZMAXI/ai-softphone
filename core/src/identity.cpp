// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/identity.hpp>

#include <algorithm>
#include <cctype>
#include <string_view>

namespace softphone::core {
namespace {

[[nodiscard]] std::string lowered(std::string_view text)
{
    std::string result{text};
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

[[nodiscard]] std::string_view trimmed(std::string_view text)
{
    const auto first = text.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t");
    return text.substr(first, last - first + 1);
}

[[nodiscard]] std::string_view addressPart(std::string_view value)
{
    if (const auto close = value.find('>'); close != std::string_view::npos) {
        return value.substr(0, close + 1);
    }

    if (const auto semicolon = value.find(';'); semicolon != std::string_view::npos) {
        return trimmed(value.substr(0, semicolon));
    }
    return value;
}

[[nodiscard]] std::string parameterPart(std::string_view value)
{
    const auto close = value.find('>');
    const auto from = close != std::string_view::npos ? close + 1 : value.find(';');
    if (from == std::string_view::npos || from >= value.size()) {
        return {};
    }
    return lowered(value.substr(from));
}

[[nodiscard]] bool hasParameter(std::string_view parameters, std::string_view name,
                                std::string_view value)
{
    std::string needle{";"};
    needle += name;
    needle += '=';
    needle += value;
    return parameters.find(needle) != std::string_view::npos;
}

[[nodiscard]] bool privacyRequested(const std::vector<std::string> &privacy)
{
    for (const auto &value : privacy) {
        const auto text = lowered(value);
        for (const auto *token : {"id", "header", "full"}) {
            const auto at = text.find(token);
            if (at == std::string::npos) {
                continue;
            }

            const auto before = at == 0 ? ' ' : text[at - 1];
            const auto after = at + std::string_view{token}.size() >= text.size()
                                   ? ' '
                                   : text[at + std::string_view{token}.size()];
            const auto separates = [](char c) {
                return c == ' ' || c == ',' || c == ';' || c == '\t';
            };
            if (separates(before) && separates(after)) {
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] std::optional<SipAddress> bestOf(const std::vector<std::string> &values)
{
    std::optional<SipAddress> fallback;

    for (const auto &value : values) {
        const auto address = parseSipAddress(addressPart(trimmed(value)));

        if (address.uri.find(':') == std::string::npos) {
            continue;
        }
        if (address.uri.starts_with("sip:") || address.uri.starts_with("sips:")) {
            return address;
        }
        if (!fallback.has_value()) {
            fallback = address;
        }
    }

    return fallback;
}

[[nodiscard]] std::string_view farEndParty(CallDirection direction)
{
    return direction == CallDirection::Outgoing ? "called" : "calling";
}

}

std::optional<SipAddress> identityToShow(const AssertedIdentity &asserted, CallDirection direction)
{
    if (privacyRequested(asserted.privacy)) {
        return std::nullopt;
    }

    if (auto identity = bestOf(asserted.pAssertedIdentity); identity.has_value()) {
        return identity;
    }

    std::vector<std::string> usable;
    for (const auto &value : asserted.remotePartyId) {
        const auto parameters = parameterPart(trimmed(value));

        if (hasParameter(parameters, "privacy", "full") ||
            hasParameter(parameters, "privacy", "uri")) {
            continue;
        }

        const auto wrongParty =
            parameters.find(";party=") != std::string::npos &&
            parameters.find(std::string{";party="} + std::string{farEndParty(direction)}) ==
                std::string::npos;
        if (wrongParty) {
            continue;
        }

        usable.push_back(value);
    }

    auto identity = bestOf(usable);
    if (!identity.has_value()) {
        return identity;
    }

    for (const auto &value : usable) {
        if (hasParameter(parameterPart(trimmed(value)), "privacy", "name")) {
            const auto address = parseSipAddress(addressPart(trimmed(value)));
            if (address.uri == identity->uri) {
                identity->displayName.clear();
            }
        }
    }

    return identity;
}

}
