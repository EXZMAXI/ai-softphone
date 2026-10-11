// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/paths.hpp>

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <string>
#include <string_view>

namespace softphone::core {

std::string asUtf8(const std::filesystem::path &file)
{

    const auto utf8 = file.u8string();
    return std::string{reinterpret_cast<const char *>(utf8.data()), utf8.size()};
}

std::filesystem::path fromUtf8(std::string_view text)
{
    const std::u8string utf8{reinterpret_cast<const char8_t *>(text.data()), text.size()};
    return std::filesystem::path{utf8};
}

std::string readablePath(const std::filesystem::path &file)
{
    const auto shown = asUtf8(file);

#if defined(_WIN32)
    return shown;
#else
    const auto *home = std::getenv("HOME");
    if (home == nullptr || *home == '\0') {
        return shown;
    }

    auto prefix = asUtf8(std::filesystem::path{home});

    while (prefix.size() > 1 && prefix.back() == '/') {
        prefix.pop_back();
    }

    if (shown == prefix) {
        return "~";
    }

    if (shown.size() > prefix.size() && shown.compare(0, prefix.size(), prefix) == 0 &&
        shown[prefix.size()] == '/') {
        return "~" + shown.substr(prefix.size());
    }

    return shown;
#endif
}

ChosenLocations locationsIn(std::string_view document)
{
    ChosenLocations chosen;

    const auto parsed = nlohmann::json::parse(document, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return chosen;
    }

    const auto read = [&parsed](const char *name) -> std::filesystem::path {
        const auto found = parsed.find(name);
        if (found == parsed.end() || !found->is_string()) {
            return {};
        }
        const auto text = found->get<std::string>();
        return text.empty() ? std::filesystem::path{} : fromUtf8(text);
    };

    chosen.profile = read("profile");
    chosen.recordings = read("recordings");
    return chosen;
}

}
