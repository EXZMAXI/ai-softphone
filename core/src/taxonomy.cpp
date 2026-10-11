// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/taxonomy.hpp>

#include "shipped_content.hpp"

#include <algorithm>
#include <cctype>

namespace softphone::core {

std::string_view toString(TaxonomyKind kind) noexcept
{
    switch (kind) {
    case TaxonomyKind::Category:
        return "category";
    case TaxonomyKind::Tag:
        return "tag";
    case TaxonomyKind::RedFlag:
        return "red_flag";
    case TaxonomyKind::Instruction:
        return "instruction";
    }
    return "category";
}

std::optional<TaxonomyKind> parseTaxonomyKind(std::string_view text) noexcept
{
    if (text == "category") {
        return TaxonomyKind::Category;
    }
    if (text == "tag") {
        return TaxonomyKind::Tag;
    }
    if (text == "red_flag") {
        return TaxonomyKind::RedFlag;
    }
    if (text == "instruction") {
        return TaxonomyKind::Instruction;
    }
    return std::nullopt;
}

std::string_view toString(Severity severity) noexcept
{
    switch (severity) {
    case Severity::Low:
        return "low";
    case Severity::Medium:
        return "medium";
    case Severity::High:
        return "high";
    }
    return "medium";
}

std::optional<Severity> parseSeverity(std::string_view text) noexcept
{
    if (text == "low") {
        return Severity::Low;
    }
    if (text == "medium") {
        return Severity::Medium;
    }
    if (text == "high") {
        return Severity::High;
    }
    return std::nullopt;
}

std::string_view toString(TagSource source) noexcept
{
    switch (source) {
    case TagSource::Llm:
        return "llm";
    case TagSource::Manual:
        return "manual";
    }
    return "llm";
}

std::optional<TagSource> parseTagSource(std::string_view text) noexcept
{
    if (text == "llm") {
        return TagSource::Llm;
    }
    if (text == "manual") {
        return TagSource::Manual;
    }
    return std::nullopt;
}

bool validTaxonomyCode(std::string_view code) noexcept
{
    if (code.empty() || code.size() > 64) {
        return false;
    }
    if (code.front() == '-' || code.back() == '-') {
        return false;
    }
    return std::all_of(code.begin(), code.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
    });
}

std::string suggestTaxonomyCode(std::string_view title)
{
    std::string code;
    code.reserve(title.size());
    for (const char c : title) {
        const auto lowered = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if ((lowered >= 'a' && lowered <= 'z') || (lowered >= '0' && lowered <= '9')) {
            code.push_back(lowered);
        } else if (!code.empty() && code.back() != '-') {
            code.push_back('-');
        }
    }
    while (!code.empty() && code.back() == '-') {
        code.pop_back();
    }
    if (code.size() > 64) {
        code.resize(64);
        while (!code.empty() && code.back() == '-') {
            code.pop_back();
        }
    }
    return code;
}

std::string joinCodes(const std::vector<std::string> &codes)
{
    std::string text;
    for (const auto &code : codes) {
        if (!validTaxonomyCode(code)) {
            continue;
        }
        if (!text.empty()) {
            text += ',';
        }
        text += code;
    }
    return text;
}

std::vector<std::string> splitCodes(std::string_view text)
{
    std::vector<std::string> codes;
    std::size_t at = 0;
    while (at <= text.size()) {
        const auto comma = text.find(',', at);
        const auto piece =
            text.substr(at, comma == std::string_view::npos ? text.size() - at : comma - at);
        if (validTaxonomyCode(piece)) {
            codes.emplace_back(piece);
        }
        if (comma == std::string_view::npos) {
            break;
        }
        at = comma + 1;
    }
    return codes;
}

std::vector<TaxonomyEntry> starterTaxonomy(std::string_view language)
{
    const auto &rows = content::listOf(language, "taxonomy");

    std::vector<TaxonomyEntry> entries;
    entries.reserve(rows.size());

    int ordinal = 0;
    std::optional<TaxonomyKind> previous;

    for (const auto &row : rows) {
        if (!row.is_object()) {
            continue;
        }
        const auto kind = parseTaxonomyKind(row.value("kind", std::string{}));
        const auto code = row.value("code", std::string{});
        if (!kind.has_value() || !validTaxonomyCode(code)) {

            continue;
        }

        if (previous.has_value() && *kind != *previous) {
            ordinal = 0;
        }
        previous = kind;

        TaxonomyEntry entry;
        entry.kind = *kind;
        entry.code = code;
        entry.title = row.value("title", std::string{});
        entry.description = row.value("description", std::string{});
        entry.severity = parseSeverity(row.value("severity", std::string{}));
        entry.ordinal = ordinal++;
        entry.builtIn = true;
        entries.push_back(std::move(entry));
    }
    return entries;
}

}
