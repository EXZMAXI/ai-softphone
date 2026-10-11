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

#include <softphone/core/call_controller.hpp>

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include <functional>

namespace softphone::ui {

class DemoCalls : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `demo`.")

public:

    DemoCalls(core::CallController &calls, std::string accountId,
              std::function<void(std::function<void()>)> post, QObject *parent = nullptr);

    Q_INVOKABLE void arrives(const QString &callId, const QString &number);

    Q_INVOKABLE void connected(const QString &callId);

    Q_INVOKABLE void held(const QString &callId);

    Q_INVOKABLE void heldByThem(const QString &callId);

    Q_INVOKABLE void activated(const QString &callId);

    Q_INVOKABLE void quiet();

private:
    core::CallController &calls_;
    std::string accountId_;
    std::function<void(std::function<void()>)> post_;
};

}
