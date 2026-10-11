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
#include <softphone/plugins/transcription/plugin.hpp>
#include <softphone/plugins/transcription/ui.hpp>

#include <softphone/shell/ui_loading.hpp>

#include <QCoreApplication>
#include <QString>
#include <QUrl>

namespace softphone::plugins::transcription {
namespace {

constexpr auto kContext = "Transcription";

const auto kComponents = QStringLiteral("qrc:/qt/qml/Softphone/Transcription/qml/");

}

void contributeInterface(shell::UiRegistry &registry)
{
    const auto owner = QString::fromUtf8(kOwner.data(), static_cast<qsizetype>(kOwner.size()));

    registry.addSettingsSection(
        "transcription", shell::settingsBand::kProcessing, 10, owner,
        [] {
            return QCoreApplication::translate(kContext,
                                               QT_TRANSLATE_NOOP("Transcription", "Transcription"));
        },
        QUrl{kComponents + QStringLiteral("SettingsTranscription.qml")});

    registry.addSlotItem(QString::fromUtf8(mediastorage::kReadableSlot), "transcription.readings",
                         10, owner, QUrl{kComponents + QStringLiteral("ReadingPanel.qml")});

    registry.addCatalogue(owner, QStringLiteral(":/i18n/transcription"),
                          QStringLiteral("transcription_"));
}

}

SOFTPHONE_PLUGIN_UI("transcription", ::softphone::plugins::transcription::contributeInterface)
