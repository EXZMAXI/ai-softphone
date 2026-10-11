// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/providers.hpp>

#include <nlohmann/json.hpp>

namespace softphone::core {

std::string_view toString(ProviderKind kind) noexcept
{
    switch (kind) {
    case ProviderKind::Asr:
        return "asr";
    case ProviderKind::Llm:
        return "llm";
    }
    return "asr";
}

std::optional<ProviderKind> parseProviderKind(std::string_view text) noexcept
{
    if (text == "asr") {
        return ProviderKind::Asr;
    }
    if (text == "llm") {
        return ProviderKind::Llm;
    }
    return std::nullopt;
}

std::string encodeParams(const std::vector<std::pair<std::string, std::string>> &params)
{
    if (params.empty()) {
        return {};
    }
    nlohmann::ordered_json object = nlohmann::ordered_json::object();
    for (const auto &[name, value] : params) {
        object[name] = value;
    }
    return object.dump();
}

std::vector<std::pair<std::string, std::string>> decodeParams(std::string_view text)
{
    std::vector<std::pair<std::string, std::string>> params;
    if (text.empty()) {
        return params;
    }
    const auto parsed = nlohmann::ordered_json::parse(text, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return params;
    }
    for (const auto &entry : parsed.items()) {
        if (!entry.value().is_string()) {
            return {};
        }
        params.emplace_back(entry.key(), entry.value().get<std::string>());
    }
    return params;
}

}
