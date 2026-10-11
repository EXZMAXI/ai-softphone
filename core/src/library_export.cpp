// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/library_export.hpp>

#include <softphone/core/time_text.hpp>

#include <array>
#include <cstdio>

namespace softphone::core {
namespace {

[[nodiscard]] std::string stampOf(std::chrono::system_clock::time_point when)
{
    const auto parts = localParts(when);
    std::array<char, 32> text{};
    std::snprintf(text.data(), text.size(), "%04d-%02d-%02d %02d:%02d:%02d", parts.tm_year + 1900,
                  parts.tm_mon + 1, parts.tm_mday, parts.tm_hour, parts.tm_min, parts.tm_sec);
    return std::string{text.data()};
}

void addRow(std::string &out, const std::vector<std::string> &fields)
{
    for (std::size_t at = 0; at < fields.size(); ++at) {
        if (at > 0) {
            out += ',';
        }
        out += csvField(fields.at(at));
    }

    out += "\r\n";
}

}

std::string csvField(std::string_view value)
{
    const auto awkward = value.find_first_of(",\"\r\n") != std::string_view::npos;

    const auto misread =
        !value.empty() && (value.front() == ' ' || value.front() == '+' || value.front() == '=' ||
                           value.front() == '-' || value.front() == '@');
    if (!awkward && !misread) {
        return std::string{value};
    }

    std::string quoted;
    quoted.reserve(value.size() + 2);
    quoted += '"';
    for (const char c : value) {
        if (c == '"') {
            quoted += '"';
        }
        quoted += c;
    }
    quoted += '"';
    return quoted;
}

std::string renderLibraryCsv(std::span<const Recording> rows, const LibraryColumns &columns)
{
    std::string out;

    addRow(out, {columns.when, columns.direction, columns.party, columns.number, columns.account,
                 columns.seconds, columns.category, columns.tags, columns.redFlags,
                 columns.language, columns.summary});

    for (const auto &row : rows) {
        std::string tags;
        for (const auto &code : row.tags) {
            if (!tags.empty()) {

                tags += ';';
            }
            tags += code;
        }

        addRow(out,
               {
                   stampOf(row.startedAt),
                   row.direction == CallDirection::Incoming ? columns.incoming : columns.outgoing,
                   row.remoteName,
                   row.remoteNumber,
                   row.accountId,
                   std::to_string(
                       std::chrono::duration_cast<std::chrono::seconds>(row.duration).count()),
                   row.categoryCode,
                   tags,
                   std::to_string(row.redFlagCount),
                   row.contentLanguage,
                   row.superSummary,
               });
    }

    return out;
}

}
