// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/realtime/plugin.hpp>
#include <softphone/plugins/realtime/ui.hpp>

#include <softphone/shell/ui_loading.hpp>

#include <QCoreApplication>
#include <QString>
#include <QUrl>

namespace softphone::plugins::realtime {
namespace {

constexpr auto kContext = "Realtime";

const auto kComponents = QStringLiteral("qrc:/qt/qml/Softphone/Realtime/qml/");

}

void contributeInterface(shell::UiRegistry &registry)
{
    const auto owner = QString::fromUtf8(kOwner.data(), static_cast<qsizetype>(kOwner.size()));

    registry.addSettingsSection(
        "realtime", shell::settingsBand::kProcessing, 30, owner,
        [] {

            return QCoreApplication::translate(kContext, QT_TRANSLATE_NOOP("Realtime", "Prompter"));
        },
        QUrl{kComponents + QStringLiteral("SettingsRealtime.qml")});

    registry.addWindow(
        "prompter", 40, owner,
        [] {

            return QCoreApplication::translate(kContext, QT_TRANSLATE_NOOP("Realtime", "Prompter"));
        },
        QUrl{kComponents +
             QStringLiteral("PrompterWindow.qml")},

        QStringLiteral("sparkle"));

    registry.addSlotItem(QStringLiteral("toolbar"), QStringLiteral("realtime.prompter"), 50, owner,
                         QUrl{kComponents + QStringLiteral("PrompterAct.qml")});

    registry.addCatalogue(owner, QStringLiteral(":/i18n/realtime"), QStringLiteral("realtime_"));
}

}

SOFTPHONE_PLUGIN_UI("realtime", ::softphone::plugins::realtime::contributeInterface)
