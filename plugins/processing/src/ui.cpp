// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/mediastorage/ui.hpp>
#include <softphone/plugins/processing/plugin.hpp>
#include <softphone/plugins/processing/ui.hpp>

#include <softphone/shell/ui_loading.hpp>

#include <QCoreApplication>
#include <QString>
#include <QUrl>

namespace softphone::plugins::processing {
namespace {

constexpr auto kContext = "Processing";

const auto kComponents = QStringLiteral("qrc:/qt/qml/Softphone/Processing/qml/");

}

void contributeInterface(shell::UiRegistry &registry)
{
    const auto owner = QString::fromUtf8(kOwner.data(), static_cast<qsizetype>(kOwner.size()));

    registry.addSettingsSection(
        "processing", shell::settingsBand::kProcessing, 20, owner,
        [] {
            return QCoreApplication::translate(kContext,
                                               QT_TRANSLATE_NOOP("Processing", "Processing"));
        },
        QUrl{kComponents + QStringLiteral("SettingsProcessing.qml")});

    registry.addSlotItem(QString::fromUtf8(mediastorage::kReadableSlot), "processing.answers", 20,
                         owner, QUrl{kComponents + QStringLiteral("AnswerPanel.qml")});

    registry.addCatalogue(owner, QStringLiteral(":/i18n/processing"),
                          QStringLiteral("processing_"));
}

}

SOFTPHONE_PLUGIN_UI("processing", ::softphone::plugins::processing::contributeInterface)
