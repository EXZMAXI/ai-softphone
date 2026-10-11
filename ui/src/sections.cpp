// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "sections.hpp"

#include "api_resources.hpp"

#include <softphone/core/webhook_events.hpp>

#include <QCoreApplication>
#include <QString>
#include <QUrl>

namespace softphone::ui {
namespace {

constexpr auto kContext = "Settings";

const auto kComponents = QStringLiteral("qrc:/qt/qml/Softphone/qml/Settings");

[[nodiscard]] QString translated(const char *text)
{
    return QCoreApplication::translate(kContext, text);
}

}

void registerSettingsGroups(shell::UiRegistry &registry)
{

    registry.addSettingsGroup(QString::fromUtf8(shell::settingsBand::kOverview), 5);
    registry.addSettingsGroup(QString::fromUtf8(shell::settingsBand::kTelephone), 10);
    registry.addSettingsGroup(QString::fromUtf8(shell::settingsBand::kProcessing), 20);
    registry.addSettingsGroup(QString::fromUtf8(shell::settingsBand::kApplication), 30);
    registry.addSettingsGroup(QString::fromUtf8(shell::settingsBand::kOther), 40);
}

void registerMonolithSections(shell::UiRegistry &registry)
{

    constexpr auto kOwner = "application";

    registry.addSettingsSection(
        "dashboard", shell::settingsBand::kOverview, 10, kOwner,

        [] { return translated(QT_TRANSLATE_NOOP("Settings", "Overview")); },
        kComponents + QStringLiteral("Dashboard.qml"));

    registry.addSettingsSection(
        "appearance", shell::settingsBand::kApplication, 10, kOwner,
        [] { return translated(QT_TRANSLATE_NOOP("Settings", "Appearance")); },
        kComponents + QStringLiteral("Appearance.qml"));

    registry.addSettingsSection(
        "startup", shell::settingsBand::kApplication, 20, kOwner,
        [] { return translated(QT_TRANSLATE_NOOP("Settings", "Startup")); },
        kComponents + QStringLiteral("Startup.qml"));
    registry.addSettingsSection(
        "shortcuts", shell::settingsBand::kApplication, 30, kOwner,
        [] { return translated(QT_TRANSLATE_NOOP("Settings", "Shortcuts")); },
        kComponents + QStringLiteral("Shortcuts.qml"));

    registry.addSettingsSection(
        "plugins", shell::settingsBand::kOther, 10, kOwner,
        [] { return translated(QT_TRANSLATE_NOOP("Settings", "Modules")); },
        kComponents + QStringLiteral("Plugins.qml"));

    registry.addSettingsSection(
        "about", shell::settingsBand::kOther, 20, kOwner,
        [] { return translated(QT_TRANSLATE_NOOP("Settings", "About")); },
        kComponents + QStringLiteral("About.qml"));
}

void registerMonolithApiResources(shell::UiRegistry &registry)
{
    constexpr auto kOwner = "application";

    registry.addApiResource(QString::fromUtf8(resource::kContacts), 10, kOwner,
                            [] { return translated(QT_TRANSLATE_NOOP("Settings", "Contacts")); });
    registry.addApiResource(QString::fromUtf8(resource::kHistory), 20, kOwner, [] {
        return translated(QT_TRANSLATE_NOOP("Settings", "Call history"));
    });
    registry.addApiResource(QString::fromUtf8(resource::kCalls), 30, kOwner, [] {
        return translated(QT_TRANSLATE_NOOP("Settings", "Calls, and control of them"));
    });
    registry.addApiResource(QString::fromUtf8(resource::kAccounts), 40, kOwner,
                            [] { return translated(QT_TRANSLATE_NOOP("Settings", "Accounts")); });
    registry.addApiResource(QString::fromUtf8(resource::kSettings), 50, kOwner,
                            [] { return translated(QT_TRANSLATE_NOOP("Settings", "Settings")); });
}

void registerMonolithOutboundEvents(shell::UiRegistry &registry)
{
    constexpr auto kOwner = "application";

    registry.addOutboundEvent(QString::fromUtf8(core::webhookEvent::kStarted), 10, kOwner, [] {
        return translated(QT_TRANSLATE_NOOP("Settings", "A new call"));
    });
    registry.addOutboundEvent(QString::fromUtf8(core::webhookEvent::kEnded), 20, kOwner, [] {
        return translated(QT_TRANSLATE_NOOP("Settings", "A call ending"));
    });
    registry.addOutboundEvent(QString::fromUtf8(core::webhookEvent::kStateChanged), 30, kOwner, [] {
        return translated(QT_TRANSLATE_NOOP("Settings", "A call changing state"));
    });
}

void registerMonolithSlotItems(shell::UiRegistry &registry)
{

    static_cast<void>(registry);
}

}
