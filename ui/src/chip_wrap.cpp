// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "chip_wrap.hpp"

#include <algorithm>
#include <cstddef>

namespace softphone::ui {
namespace {

[[nodiscard]] std::vector<std::vector<std::size_t>> cutInto(const std::vector<double> &widths,
                                                            double room, double spacing)
{
    std::vector<std::vector<std::size_t>> rows;
    double used = 0.0;

    for (std::size_t index = 0; index < widths.size(); ++index) {
        const auto width = widths[index];
        if (width <= 0.0) {
            continue;
        }

        const auto wanted = rows.empty() || rows.back().empty() ? width : used + spacing + width;
        if (!rows.empty() && !rows.back().empty() && wanted > room) {
            rows.emplace_back();
            used = width;
        } else {
            if (rows.empty()) {
                rows.emplace_back();
            }
            used = wanted;
        }

        rows.back().push_back(index);
    }

    return rows;
}

}

ChipWrap wrapChips(const std::vector<double> &widths, double available, double spacing,
                   double rowHeight, double rowSpacing, Lines lines)
{
    ChipWrap laid;
    laid.places.resize(widths.size());

    const auto room = std::max(0.0, available);

    auto rows = cutInto(widths, room, spacing);

    if (lines == Lines::Evened && rows.size() > 1) {

        auto widest = 0.0;
        for (const auto width : widths) {
            widest = std::max(widest, width);
        }

        if (room > widest) {
            auto low = widest;
            auto high = room;

            while (high - low > 0.5) {
                const auto middle = low + (high - low) / 2.0;
                if (cutInto(widths, middle, spacing).size() <= rows.size()) {
                    high = middle;
                } else {
                    low = middle;
                }
            }

            if (auto evened = cutInto(widths, high, spacing); evened.size() == rows.size()) {
                rows = std::move(evened);
            }
        }
    }

    for (std::size_t row = 0; row < rows.size(); ++row) {
        double span = 0.0;
        for (const auto index : rows[row]) {
            span += widths[index];
        }
        span += spacing * static_cast<double>(rows[row].size() - 1);

        auto at = std::max(0.0, (room - span) / 2.0);
        const auto top = static_cast<double>(row) * (rowHeight + rowSpacing);

        for (const auto index : rows[row]) {
            laid.places[index] = ChipPlace{at, top};
            at += widths[index] + spacing;
        }
    }

    laid.rows = static_cast<int>(rows.size());
    return laid;
}

QVariantMap wrapAsVariant(const QVariantList &widths, double available, double spacing,
                          double rowHeight, double rowSpacing, Lines lines)
{
    std::vector<double> measured;
    measured.reserve(static_cast<std::size_t>(widths.size()));
    for (const auto &value : widths) {
        measured.push_back(value.toDouble());
    }

    const auto laid = wrapChips(measured, available, spacing, rowHeight, rowSpacing, lines);

    QVariantList places;
    places.reserve(static_cast<qsizetype>(laid.places.size()));
    for (const auto &place : laid.places) {
        QVariantMap one;
        one.insert(QStringLiteral("x"), place.x);
        one.insert(QStringLiteral("y"), place.y);
        places.append(one);
    }

    QVariantMap answer;
    answer.insert(QStringLiteral("places"), places);
    answer.insert(QStringLiteral("rows"), laid.rows);
    return answer;
}

}
