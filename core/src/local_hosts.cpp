// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/local_hosts.hpp>

#include <algorithm>
#include <array>

namespace softphone::core::hosts {

namespace {

constexpr std::string_view kSite = "https://ai-softphone.com";

constexpr std::string_view kDocs = "https://docs.ai-softphone.com";

constexpr std::string_view kSourceLanguage = "en";

struct Local {
    std::string_view country;

    std::string_view site;

    std::string_view docs;

    bool docsByLanguage;

    std::string_view telemetry;
};

constexpr std::array kLocal = {
    Local{"RU", "https://ai-softphone.ru", "https://docs.ai-softphone.ru", false,
          "https://telemetry.ai-softphone.ru/api/token/v1/16aeb19b2fe864f478ed909252f3ac36"},
};

[[nodiscard]] const Local *rowFor(std::string_view country)
{
    if (country.empty()) {
        return nullptr;
    }
    const auto found = std::ranges::find(kLocal, country, &Local::country);
    return found == kLocal.end() ? nullptr : &*found;
}

}

std::string siteFor(std::string_view country)
{
    const auto *row = rowFor(country);
    return std::string{row != nullptr && !row->site.empty() ? row->site : kSite};
}

std::string docsFor(std::string_view country)
{
    const auto *row = rowFor(country);
    return std::string{row != nullptr && !row->docs.empty() ? row->docs : kDocs};
}

std::string docsPageFor(std::string_view country, std::string_view language)
{
    auto page = docsFor(country);

    if (language.empty() || language == kSourceLanguage) {
        return page;
    }

    const auto *row = rowFor(country);
    if (row != nullptr && !row->docs.empty() && !row->docsByLanguage) {
        return page;
    }

    page += '/';
    for (const char letter : language) {

        page += letter == '_' ? '-' : letter;
    }
    page += '/';
    return page;
}

std::vector<std::string> telemetryHostsFor(std::string_view country)
{
    const auto *row = rowFor(country);
    if (row == nullptr || row->telemetry.empty()) {
        return {};
    }
    return {std::string{row->telemetry}};
}

bool anythingFor(std::string_view country)
{
    return rowFor(country) != nullptr;
}

}
