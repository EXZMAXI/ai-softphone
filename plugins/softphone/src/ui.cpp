// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/softphone/plugin.hpp>
#include <softphone/plugins/softphone/ui.hpp>

#include <softphone/shell/windows.hpp>

#include <QCoreApplication>
#include <QString>
#include <QUrl>

namespace softphone::plugins::softphone {
namespace {

constexpr auto kContext = "Telephone";

const auto kComponents = QStringLiteral("qrc:/qt/qml/Softphone/Telephone/qml/");

}

void contributeInterface(shell::UiRegistry &registry)
{
    const auto owner = QString::fromUtf8(kOwner.data(), static_cast<qsizetype>(kOwner.size()));

    registry.addSettingsSection(
        "accounts", shell::settingsBand::kTelephone, 10, owner,
        [] {
            return QCoreApplication::translate(kContext,
                                               QT_TRANSLATE_NOOP("Telephone", "Accounts"));
        },
        QUrl{kComponents + QStringLiteral("SettingsAccounts.qml")});

    registry.addSettingsSection(
        "audio", shell::settingsBand::kTelephone, 20, owner,
        [] {
            return QCoreApplication::translate(kContext, QT_TRANSLATE_NOOP("Telephone", "Devices"));
        },
        QUrl{kComponents + QStringLiteral("SettingsAudio.qml")});

    registry.addSettingsSection(
        "calls", shell::settingsBand::kTelephone, 30, owner,
        [] {
            return QCoreApplication::translate(kContext, QT_TRANSLATE_NOOP("Telephone", "Calls"));
        },
        QUrl{kComponents + QStringLiteral("SettingsCalls.qml")});

    registry.addSlotItem(QString::fromUtf8(shell::kBarSlot), QStringLiteral("softphone.bar"), 10,
                         owner, QUrl{kComponents + QStringLiteral("BarPage.qml")});

    registry.addSlotItem(QStringLiteral("primary"), QStringLiteral("softphone.dialler"), 10, owner,
                         QUrl{kComponents + QStringLiteral("Dialler.qml")});

    registry.addWindow(
        QString::fromUtf8(shell::kPeopleWindow), 20, owner,
        [] {
            return QCoreApplication::translate(
                kContext, QT_TRANSLATE_NOOP("Telephone", "Contacts and calls"));
        },
        QUrl{});

    registry.addSlotItem(
        QString::fromUtf8(shell::kPeopleTabs), QStringLiteral("softphone.history"), 20, owner,
        QUrl{kComponents + QStringLiteral("HistoryTab.qml")},
        [] {
            return QCoreApplication::translate("HistoryTab",
                                               QT_TRANSLATE_NOOP("HistoryTab", "History"));
        },
        QStringLiteral("history"));

    registry.addSlotItem(QStringLiteral("toolbar"), QStringLiteral("softphone.history.act"), 20,
                         owner, QUrl{kComponents + QStringLiteral("HistoryAct.qml")});

    registry.addCatalogue(owner, QStringLiteral(":/i18n/telephone"), QStringLiteral("telephone_"));
}

}
