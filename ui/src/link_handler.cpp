// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "link_handler.hpp"

#include <softphone/core/dial_link.hpp>
#include <softphone/core/paths.hpp>

#include <algorithm>
#include <sstream>
#include <vector>

namespace softphone::ui {
namespace {

[[nodiscard]] std::string schemeKey(std::string_view scheme)
{
    return "x-scheme-handler/" + std::string{scheme};
}

[[nodiscard]] std::string_view trimmed(std::string_view text)
{
    const auto space = [](char character) {
        return character == ' ' || character == '\t' || character == '\r';
    };
    while (!text.empty() && space(text.front())) {
        text.remove_prefix(1);
    }
    while (!text.empty() && space(text.back())) {
        text.remove_suffix(1);
    }
    return text;
}

[[nodiscard]] std::string valueUnderDefaults(std::string_view carried, std::string_view key)
{
    std::istringstream lines{std::string{carried}};
    std::string line;
    bool inside = false;

    while (std::getline(lines, line)) {
        const auto cleaned = trimmed(line);
        if (cleaned.starts_with("[")) {
            inside = cleaned == "[Default Applications]";
            continue;
        }
        if (!inside) {
            continue;
        }
        const auto equals = cleaned.find('=');
        if (equals == std::string_view::npos) {
            continue;
        }
        if (trimmed(cleaned.substr(0, equals)) == key) {

            const auto value = trimmed(cleaned.substr(equals + 1));
            return std::string{value.substr(0, value.find(';'))};
        }
    }
    return {};
}

}

bool mimeAppsNameUs(std::string_view carried, std::string_view entryName)
{
    return std::all_of(core::kDialSchemes.begin(), core::kDialSchemes.end(),
                       [&carried, entryName](std::string_view scheme) {
                           return valueUnderDefaults(carried, schemeKey(scheme)) == entryName;
                       });
}

std::string mimeAppsHandlerOfTel(std::string_view carried)
{
    return valueUnderDefaults(carried, schemeKey("tel"));
}

std::string mimeAppsPointedAtUs(std::string_view carried, std::string_view entryName)
{
    std::vector<std::string> ours;
    ours.reserve(core::kDialSchemes.size());
    for (const auto scheme : core::kDialSchemes) {
        ours.push_back(schemeKey(scheme));
    }
    const auto isOurs = [&ours](std::string_view key) {
        return std::find(ours.begin(), ours.end(), key) != ours.end();
    };

    std::string out;
    std::istringstream lines{std::string{carried}};
    std::string line;
    bool inside = false;
    bool written = false;

    const auto writeOurs = [&out, &ours, entryName]() {
        for (const auto &key : ours) {
            out += key;
            out += '=';
            out += entryName;
            out += '\n';
        }
    };

    while (std::getline(lines, line)) {
        const auto cleaned = trimmed(line);

        if (cleaned.starts_with("[")) {

            if (inside && !written) {
                writeOurs();
                written = true;
            }
            inside = cleaned == "[Default Applications]";
            out += line;
            out += '\n';
            continue;
        }

        if (inside) {
            const auto equals = cleaned.find('=');
            if (equals != std::string_view::npos && isOurs(trimmed(cleaned.substr(0, equals)))) {

                continue;
            }
        }

        out += line;
        out += '\n';
    }

    if (inside && !written) {
        writeOurs();
        written = true;
    }

    if (!written) {

        if (!out.empty() && out.back() != '\n') {
            out += '\n';
        }
        out += "[Default Applications]\n";
        writeOurs();
    }

    return out;
}

std::string urlOpenCommand(const std::filesystem::path &binary)
{

    return '"' + core::asUtf8(binary) + "\" \"%1\"";
}

}
