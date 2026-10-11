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

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace softphone::ui {

class StartupState : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool ready READ ready NOTIFY changed)

    Q_PROPERTY(QString problem READ problem NOTIFY changed)

public:
    using QObject::QObject;

    [[nodiscard]] bool ready() const noexcept
    {
        return ready_;
    }

    [[nodiscard]] QString problem() const
    {
        return problem_;
    }

    void succeeded()
    {
        if (ready_ && problem_.isEmpty()) {
            return;
        }
        ready_ = true;
        problem_.clear();
        emit changed();
    }

    void nothingToWaitFor()
    {
        succeeded();
    }

    void starting()
    {
        if (!ready_ && problem_.isEmpty()) {
            return;
        }
        ready_ = false;
        problem_.clear();
        emit changed();
    }

    void failed(const QString &why)
    {
        if (!ready_ && problem_ == why) {
            return;
        }
        ready_ = false;
        problem_ = why;
        emit changed();
    }

signals:

    void changed();

private:
    bool ready_{false};
    QString problem_;
};

}
