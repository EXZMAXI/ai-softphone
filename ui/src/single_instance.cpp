// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "single_instance.hpp"

#include "desktop_platform.hpp"

#include <softphone/core/logging.hpp>

#include <QByteArray>
#include <QCryptographicHash>
#include <QDeadlineTimer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QThread>

#include <chrono>
#include <utility>

namespace softphone::ui {
namespace {

constexpr int kAnswerMs = 500;

}

QString defaultInstanceName()
{

    const auto home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const auto digest =
        QCryptographicHash::hash(home.toUtf8(), QCryptographicHash::Sha256).toHex().left(16);

    return QStringLiteral("ai-softphone-%1").arg(QString::fromLatin1(digest));
}

SingleInstance::SingleInstance(QString name, QObject *parent)
    : QObject{parent}, name_{std::move(name)}
{
    connect(&server_, &QLocalServer::newConnection, this, [this]() {
        while (auto *connection = server_.nextPendingConnection()) {

            const bool arrived = connection->waitForReadyRead(kAnswerMs);
            const auto message = arrived ? connection->readAll() : QByteArray{};
            connection->deleteLater();

            if (message.startsWith("dial ")) {
                broughtALink(QString::fromUtf8(message.mid(5)));
                return;
            }

            if (message.startsWith("quit")) {
                core::Logger::instance().log(core::LogLevel::Info, "ui",
                                             "another copy asked this one to leave");
                Q_EMIT quitRequested();
                return;
            }

            core::Logger::instance().log(core::LogLevel::Info, "ui",
                                         "another copy was started; showing this one");
            Q_EMIT showRequested();
        }
    });
}

bool SingleInstance::claim(const QString &wanted)
{
    QLocalSocket socket;
    socket.connectToServer(name_);

    if (socket.waitForConnected(kAnswerMs)) {

        platform::allowAnotherProcessToRaise();

        socket.write(wanted.isEmpty() ? QByteArray{"show"} : QByteArray{"dial "} + wanted.toUtf8());
        socket.flush();
        static_cast<void>(socket.waitForBytesWritten(kAnswerMs));
        socket.disconnectFromServer();
        return false;
    }

    static_cast<void>(QLocalServer::removeServer(name_));

    server_.setSocketOptions(QLocalServer::UserAccessOption);

    if (!server_.listen(name_)) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot listen for other copies of the phone: " +
                                         server_.errorString().toStdString() + "; starting anyway");
    }

    return true;
}

void SingleInstance::askedForTheWindow()
{
    core::Logger::instance().log(core::LogLevel::Info, "ui",
                                 "the desktop asked this phone to show itself");
    Q_EMIT showRequested();
}

void SingleInstance::broughtALink(const QString &target)
{
    if (target.isEmpty()) {
        return;
    }
    core::Logger::instance().log(core::LogLevel::Info, "ui",
                                 "a link asked this phone to dial " + target.toStdString());
    Q_EMIT dialRequested(target);
}

void SingleInstance::release()
{
    if (!server_.isListening()) {
        return;
    }

    server_.close();

    static_cast<void>(QLocalServer::removeServer(name_));
}

QuitOutcome askTheRunningPhoneToQuit(const QString &name, std::chrono::milliseconds patience)
{

    const auto tell = [&name]() {
        QLocalSocket socket;
        socket.connectToServer(name);

        if (!socket.waitForConnected(kAnswerMs)) {
            return false;
        }

        socket.write("quit");
        socket.flush();
        static_cast<void>(socket.waitForBytesWritten(kAnswerMs));
        socket.disconnectFromServer();
        return true;
    };

    if (!tell()) {
        return QuitOutcome::NobodyThere;
    }

    constexpr auto kPoll = std::chrono::milliseconds{250};

    QDeadlineTimer deadline{static_cast<qint64>(patience.count())};
    while (!deadline.hasExpired()) {
        QThread::msleep(static_cast<unsigned long>(kPoll.count()));

        if (!tell()) {
            return QuitOutcome::Left;
        }
    }

    return QuitOutcome::StillRunning;
}

}
