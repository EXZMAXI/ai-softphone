// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/mediastorage/plugin.hpp>
#include <softphone/plugins/mediastorage/ui.hpp>

#include <softphone/shell/ui_loading.hpp>

#include <QCoreApplication>
#include <QString>
#include <QUrl>

namespace softphone::plugins::mediastorage {
namespace {

constexpr auto kContext = "MediaStorage";

const auto kComponents = QStringLiteral("qrc:/qt/qml/Softphone/MediaStorage/qml/");

}

void contributeInterface(shell::UiRegistry &registry)
{
    const auto owner = QString::fromUtf8(kOwner.data(), static_cast<qsizetype>(kOwner.size()));

    registry.addWindow(
        "recordings", 10, owner,
        [] {
            return QCoreApplication::translate(kContext,
                                               QT_TRANSLATE_NOOP("MediaStorage", "Recordings"));
        },
        QUrl{kComponents + QStringLiteral("RecordingsBody.qml")}, QStringLiteral("library"));

    registry.addSlotItem(QStringLiteral("toolbar"), QStringLiteral("mediastorage.recordings"), 30,
                         owner, QUrl{kComponents + QStringLiteral("RecordingsAct.qml")});

    registry.addCatalogue(owner, QStringLiteral(":/i18n/mediastorage"),
                          QStringLiteral("mediastorage_"));
}

}

SOFTPHONE_PLUGIN_UI("mediastorage", ::softphone::plugins::mediastorage::contributeInterface)
