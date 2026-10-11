// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/directory/plugin.hpp>
#include <softphone/plugins/directory/ui.hpp>

#include <softphone/shell/ui_loading.hpp>
#include <softphone/shell/windows.hpp>

#include <QCoreApplication>
#include <QString>
#include <QUrl>

namespace softphone::plugins::directory {
namespace {

constexpr auto kContext = "Directory";

const auto kComponents = QStringLiteral("qrc:/qt/qml/Softphone/Directory/qml/");

}

void contributeInterface(shell::UiRegistry &registry)
{
    const auto owner = QString::fromUtf8(kOwner.data(), static_cast<qsizetype>(kOwner.size()));

    registry.addWindow(
        QString::fromUtf8(shell::kPeopleWindow), 20, owner,
        [] {
            return QCoreApplication::translate(
                kContext, QT_TRANSLATE_NOOP("Directory", "Contacts and calls"));
        },
        QUrl{});

    registry.addSlotItem(
        QString::fromUtf8(shell::kPeopleTabs), QStringLiteral("directory.contacts"), 10, owner,
        QUrl{kComponents + QStringLiteral("ContactsTab.qml")},
        [] {
            return QCoreApplication::translate("ContactsTab",
                                               QT_TRANSLATE_NOOP("ContactsTab", "Contacts"));
        },
        QStringLiteral("book-contacts"));

    registry.addSlotItem(QStringLiteral("toolbar"), QStringLiteral("directory.contacts.act"), 10,
                         owner, QUrl{kComponents + QStringLiteral("ContactsAct.qml")});

    registry.addCatalogue(owner, QStringLiteral(":/i18n/directory"), QStringLiteral("directory_"));
}

}

SOFTPHONE_PLUGIN_UI("directory", ::softphone::plugins::directory::contributeInterface)
