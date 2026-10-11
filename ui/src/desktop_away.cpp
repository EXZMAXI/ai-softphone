// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "desktop_platform.hpp"

#include <softphone/core/logging.hpp>

#ifdef SOFTPHONE_HAS_PORTAL
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QObject>
#include <QString>
#endif

#include <string>
#include <utility>

namespace softphone::ui::platform {

#ifdef SOFTPHONE_HAS_PORTAL

namespace {

class Listener : public QObject {
    Q_OBJECT

public:
    explicit Listener(Leaving told) : told_{std::move(told)} {}

public Q_SLOTS:

    void sleeping(bool about)
    {

        if (about) {
            say(Away::Sleeping);
        }
    }

    void locked(bool active)
    {
        say(active ? Away::Locked : Away::Unlocked);
    }

private:
    void say(Away what)
    {
        if (told_) {
            told_(what);
        }
    }

    Leaving told_;
};

Listener *listening = nullptr;

}

bool watchForLeaving(Leaving told)
{
    stopWatchingForLeaving();
    auto *const owner = QCoreApplication::instance();
    if (owner == nullptr) {

        return false;
    }
    listening = new Listener{std::move(told)};
    listening->setParent(owner);

    const auto sleep = QDBusConnection::systemBus().connect(
        QStringLiteral("org.freedesktop.login1"), QStringLiteral("/org/freedesktop/login1"),
        QStringLiteral("org.freedesktop.login1.Manager"), QStringLiteral("PrepareForSleep"),
        listening, SLOT(sleeping(bool)));

    bool lock = false;
    for (const auto &service :
         {QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("org.gnome.ScreenSaver"),
          QStringLiteral("org.cinnamon.ScreenSaver")}) {
        auto path = service;
        path.replace(QLatin1Char('.'), QLatin1Char('/'));
        lock = QDBusConnection::sessionBus().connect(service, QStringLiteral("/") + path, service,
                                                     QStringLiteral("ActiveChanged"), listening,
                                                     SLOT(locked(bool))) ||
               lock;
    }

    if (!sleep && !lock) {
        stopWatchingForLeaving();
        core::Logger::instance().log(core::LogLevel::Info, "capture",
                                     "this session says nothing about locking or sleeping; a "
                                     "recording nobody is there for ends by going quiet instead");
        return false;
    }

    core::Logger::instance().log(core::LogLevel::Info, "capture",
                                 std::string{"watching this session: sleep "} +
                                     (sleep ? "yes" : "no") + ", lock " + (lock ? "yes" : "no"));
    return true;
}

void stopWatchingForLeaving()
{
    delete listening;
    listening = nullptr;
}

#else

bool watchForLeaving(Leaving )
{

    return false;
}

void stopWatchingForLeaving() {}

#endif

}

#ifdef SOFTPHONE_HAS_PORTAL
#include "desktop_away.moc"
#endif
