// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/diagnostics/plugin.hpp>
#include <softphone/plugins/diagnostics/ui.hpp>

#include <QCoreApplication>
#include <QString>
#include <QUrl>

namespace softphone::plugins::diagnostics {
namespace {

constexpr auto kContext = "Diagnostics";

const auto kComponents = QStringLiteral("qrc:/qt/qml/Softphone/Diagnostics/qml/");

}

void contributeInterface(shell::UiRegistry &registry)
{
    const auto owner = QString::fromUtf8(kOwner.data(), static_cast<qsizetype>(kOwner.size()));

    registry.addSettingsSection(
        "diagnostics", shell::settingsBand::kApplication, 50, owner,
        [] {
            return QCoreApplication::translate(kContext,
                                               QT_TRANSLATE_NOOP("Diagnostics", "Diagnostics"));
        },
        QUrl{kComponents + QStringLiteral("SettingsDiagnostics.qml")});

    registry.addApiResource(QString::fromUtf8(kMetricsResource), 60, owner, [] {
        return QCoreApplication::translate(kContext, QT_TRANSLATE_NOOP("Diagnostics", "Counters"));
    });

    registry.addWindow(
        "diagnostics", 90, owner,
        [] {
            return QCoreApplication::translate(kContext,
                                               QT_TRANSLATE_NOOP("Diagnostics", "Diagnostics"));
        },
        QUrl{kComponents + QStringLiteral("DiagnosticsBody.qml")}, QStringLiteral("pulse"));

    registry.addCatalogue(owner, QStringLiteral(":/i18n/diagnostics"),
                          QStringLiteral("diagnostics_"));
}

}
