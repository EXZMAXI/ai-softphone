// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "shipped_content.hpp"

#include <map>
#include <mutex>
#include <string>

namespace softphone::core::content {

namespace {

constexpr std::string_view kFallback = "en";

const nlohmann::json &parsed(std::string_view language)
{

    static std::mutex guard;
    static std::map<std::string, nlohmann::json, std::less<>> documents;

    const std::lock_guard held{guard};
    if (const auto found = documents.find(language); found != documents.end()) {
        return found->second;
    }

    const auto bytes = shippedBytes(language);
    auto document = bytes.empty() ? nlohmann::json{} : nlohmann::json::parse(bytes, nullptr, false);
    if (document.is_discarded() || !document.is_object()) {
        document = nlohmann::json::object();
    }
    return documents.emplace(std::string{language}, std::move(document)).first->second;
}

}

const nlohmann::json &shipped(std::string_view language)
{
    if (const auto &document = parsed(language); !document.empty()) {
        return document;
    }
    return parsed(kFallback);
}

const nlohmann::json &listOf(std::string_view language, std::string_view name)
{
    static const nlohmann::json kEmpty = nlohmann::json::array();

    const auto &document = shipped(language);
    const auto found = document.find(name);
    if (found == document.end() || !found->is_array()) {
        return kEmpty;
    }
    return *found;
}

}
