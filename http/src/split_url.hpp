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

#include <string>
#include <string_view>

namespace softphone::http {

struct SplitUrl {
    bool ok{false};
    bool tls{false};
    std::string origin;
    std::string target;

    std::string host;
};

[[nodiscard]] inline SplitUrl split(std::string_view url)
{
    SplitUrl split;

    constexpr std::string_view kHttp{"http://"};
    constexpr std::string_view kHttps{"https://"};

    std::string_view rest;
    if (url.starts_with(kHttps)) {
        split.tls = true;
        rest = url.substr(kHttps.size());
    } else if (url.starts_with(kHttp)) {
        rest = url.substr(kHttp.size());
    } else {
        return split;
    }

    const auto slash = rest.find('/');
    if (slash == std::string_view::npos) {
        split.origin = std::string{url};
        split.target = "/";
    } else {
        split.origin = std::string{url.substr(0, url.size() - rest.size() + slash)};
        split.target = std::string{rest.substr(slash)};
    }

    auto authority = rest.substr(0, slash == std::string_view::npos ? rest.size() : slash);
    const auto afterBracket = authority.rfind(']');
    const auto colon = authority.rfind(':');
    if (colon != std::string_view::npos &&
        (afterBracket == std::string_view::npos || colon > afterBracket)) {
        authority = authority.substr(0, colon);
    }
    split.host = std::string{authority};

    split.ok = !split.origin.empty() && split.origin != kHttp && split.origin != kHttps;
    return split;
}

}
