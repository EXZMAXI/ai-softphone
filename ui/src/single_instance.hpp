// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#pragma once

#include <QLocalServer>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include <chrono>

namespace softphone::ui {

[[nodiscard]] QString defaultInstanceName();

class SingleInstance : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `instance`.")

public:
    explicit SingleInstance(QString name, QObject *parent = nullptr);

    [[nodiscard]] bool claim(const QString &wanted = {});

    void broughtALink(const QString &target);

    void askedForTheWindow();

    void release();

Q_SIGNALS:

    void showRequested();

    void dialRequested(const QString &target);

    void quitRequested();

private:
    QString name_;
    QLocalServer server_;
};

enum class QuitOutcome {

    NobodyThere,

    Left,

    StillRunning,
};

[[nodiscard]] QuitOutcome askTheRunningPhoneToQuit(const QString &name,
                                                   std::chrono::milliseconds patience);

}
