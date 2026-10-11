// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "row_marks.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QLocale>

#include <algorithm>
#include <ranges>

namespace softphone::ui {

QString markOn(const QString &name, const QString &mark)
{
    if (mark.isEmpty()) {
        return name;
    }

    return QCoreApplication::translate("RowMarks", "%1 · %2").arg(name, mark);
}

QString providerNameOf(const std::string &providerId, const std::string &providerType,
                       const std::vector<std::pair<QString, QString>> &names)
{
    const auto wanted = QString::fromStdString(providerId);
    if (!wanted.isEmpty()) {
        for (const auto &[id, name] : names) {
            if (id == wanted && !name.isEmpty()) {
                return name;
            }
        }
    }

    return QString::fromStdString(providerType);
}

QString whenMark(std::chrono::system_clock::time_point at)
{
    const auto since = std::chrono::duration_cast<std::chrono::milliseconds>(at.time_since_epoch());
    if (since.count() <= 0) {
        return {};
    }

    const auto when = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(since.count()));
    return when.date() == QDate::currentDate()
               ? QLocale{}.toString(when.time(), QLocale::ShortFormat)
               : QLocale{}.toString(when, QLocale::ShortFormat);
}

std::vector<QString> toldApart(std::vector<QString> base,
                               const std::vector<std::vector<QString>> &candidates)
{
    for (const auto &column : candidates) {
        if (column.size() != base.size()) {
            continue;
        }

        std::vector<std::vector<std::size_t>> groups;
        for (std::size_t at = 0; at < base.size(); ++at) {
            const auto found =
                std::ranges::find_if(groups, [&base, at](const std::vector<std::size_t> &group) {
                    return base[group.front()] == base[at];
                });
            if (found == groups.end()) {
                groups.push_back({at});
            } else {
                found->push_back(at);
            }
        }

        for (const auto &group : groups) {
            if (group.size() < 2) {
                continue;
            }

            const bool differs = std::ranges::any_of(group, [&column, &group](std::size_t at) {
                return column[at] != column[group.front()];
            });
            if (!differs) {
                continue;
            }
            for (const std::size_t at : group) {
                base[at] = markOn(base[at], column[at]);
            }
        }
    }
    return base;
}

}
