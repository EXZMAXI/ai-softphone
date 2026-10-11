// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "export_names.hpp"

#include <QRegularExpression>

namespace softphone::ui {

QString conversationStem(const QDateTime &when, QString who)
{
    auto name = when.toString(QStringLiteral("yyyy-MM-dd HH-mm"));
    if (who.isEmpty()) {
        return name;
    }

    who.replace(QRegularExpression{QStringLiteral(R"([\\/:*?"<>|])")}, QStringLiteral("-"));

    constexpr qsizetype kRoomForNames = 120;
    if (who.size() > kRoomForNames) {
        who = who.left(kRoomForNames).trimmed() + QStringLiteral("…");
    }

    name += QStringLiteral(" ") + who;
    return name;
}

}
