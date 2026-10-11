// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "desktop_colours.hpp"

#include <QGuiApplication>
#include <QStyleHints>

#ifdef SOFTPHONE_HAS_PORTAL
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>
#include <QVariant>
#endif

namespace softphone::ui {

namespace {

#ifdef SOFTPHONE_HAS_PORTAL

constexpr auto kService = "org.freedesktop.portal.Desktop";
constexpr auto kPath = "/org/freedesktop/portal/desktop";
constexpr auto kSettings = "org.freedesktop.portal.Settings";
constexpr auto kAppearance = "org.freedesktop.appearance";
constexpr auto kColorScheme = "color-scheme";

Qt::ColorScheme fromPortalValue(uint value)
{
    switch (value) {
    case 1:
        return Qt::ColorScheme::Dark;
    case 2:
        return Qt::ColorScheme::Light;
    default:
        return Qt::ColorScheme::Unknown;
    }
}

Qt::ColorScheme readScheme(const QVariant &answer)
{
    auto inner = answer;
    while (inner.canConvert<QDBusVariant>()) {
        inner = inner.value<QDBusVariant>().variant();
    }
    bool understood = false;
    const auto value = inner.toUInt(&understood);
    return understood ? fromPortalValue(value) : Qt::ColorScheme::Unknown;
}

#endif

}

DesktopColours::DesktopColours(QObject *parent) : QObject{parent}
{
    if (auto *hints = QGuiApplication::styleHints(); hints != nullptr) {
        connect(hints, &QStyleHints::colorSchemeChanged, this, &DesktopColours::changed);
    }
    askThePortal();
    watchThePortal();
}

DesktopColours::~DesktopColours() = default;

Qt::ColorScheme DesktopColours::scheme() const
{

    if (fromThePortal_ != Qt::ColorScheme::Unknown) {
        return fromThePortal_;
    }
    const auto *hints = QGuiApplication::styleHints();
    return hints != nullptr ? hints->colorScheme() : Qt::ColorScheme::Unknown;
}

#ifdef SOFTPHONE_HAS_PORTAL

void DesktopColours::askThePortal()
{
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        return;
    }

    QDBusInterface settings{QString::fromLatin1(kService), QString::fromLatin1(kPath),
                            QString::fromLatin1(kSettings), bus};
    settings.setTimeout(300);
    if (!settings.isValid()) {
        return;
    }

    const QDBusReply<QVariant> answer =
        settings.call(QStringLiteral("Read"), QString::fromLatin1(kAppearance),
                      QString::fromLatin1(kColorScheme));
    if (!answer.isValid()) {
        return;
    }
    fromThePortal_ = readScheme(answer.value());
}

void DesktopColours::watchThePortal()
{
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        return;
    }
    bus.connect(QString::fromLatin1(kService), QString::fromLatin1(kPath),
                QString::fromLatin1(kSettings), QStringLiteral("SettingChanged"), this,
                SLOT(portalSaidSomething()));
}

void DesktopColours::portalSaidSomething()
{
    const auto was = fromThePortal_;
    askThePortal();
    if (fromThePortal_ != was) {
        Q_EMIT changed();
    }
}

#else

void DesktopColours::askThePortal() {}

void DesktopColours::watchThePortal() {}

void DesktopColours::portalSaidSomething() {}

#endif

}
