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

#include "tray_presentation.hpp"

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace softphone::ui {

class AccountsViewModel;
class CallViewModel;
class HistoryViewModel;

class TrayViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the shell")

    Q_PROPERTY(Status status READ status NOTIFY changed)

    Q_PROPERTY(QString icon READ icon NOTIFY changed)

    Q_PROPERTY(int lines READ lines NOTIFY changed)
    Q_PROPERTY(int connected READ connected NOTIFY changed)
    Q_PROPERTY(int failed READ failed NOTIFY changed)

    Q_PROPERTY(bool ringing READ ringing NOTIFY changed)
    Q_PROPERTY(bool inCall READ inCall NOTIFY changed)

    Q_PROPERTY(bool recording READ recording NOTIFY changed)
    Q_PROPERTY(int recordings READ recordings NOTIFY changed)
    Q_PROPERTY(bool muted READ muted NOTIFY changed)
    Q_PROPERTY(bool held READ held NOTIFY changed)

    Q_PROPERTY(QString callLabel READ callLabel NOTIFY changed)
    Q_PROPERTY(QString lineLabel READ lineLabel NOTIFY changed)

    Q_PROPERTY(bool canAnswer READ canAnswer NOTIFY changed)
    Q_PROPERTY(bool canReject READ canReject NOTIFY changed)
    Q_PROPERTY(bool canHangUp READ canHangUp NOTIFY changed)
    Q_PROPERTY(bool canMute READ canMute NOTIFY changed)

public:

    enum class Status {
        Offline,
        Degraded,
        Failing,
        Connected,
        Ringing,
    };
    Q_ENUM(Status)

    TrayViewModel(AccountsViewModel &accounts, CallViewModel &calls, HistoryViewModel &history,
                  QObject *parent = nullptr);

    [[nodiscard]] Status status() const noexcept;
    [[nodiscard]] QString icon() const;

    [[nodiscard]] int lines() const noexcept
    {
        return shown_.lines;
    }

    [[nodiscard]] int connected() const noexcept
    {
        return shown_.connected;
    }

    [[nodiscard]] int failed() const noexcept
    {
        return shown_.failed;
    }

    [[nodiscard]] bool ringing() const noexcept
    {
        return shown_.ringing;
    }

    [[nodiscard]] bool inCall() const noexcept
    {
        return shown_.inCall;
    }

    [[nodiscard]] bool recording() const noexcept
    {
        return shown_.recordings > 0;
    }

    [[nodiscard]] int recordings() const noexcept
    {
        return shown_.recordings;
    }

    [[nodiscard]] bool muted() const noexcept
    {
        return shown_.muted;
    }

    [[nodiscard]] bool held() const noexcept
    {
        return shown_.held;
    }

    [[nodiscard]] QString callLabel() const;
    [[nodiscard]] QString lineLabel() const;

    [[nodiscard]] bool canAnswer() const noexcept
    {
        return shown_.canAnswer;
    }

    [[nodiscard]] bool canReject() const noexcept
    {
        return shown_.canReject;
    }

    [[nodiscard]] bool canHangUp() const noexcept
    {
        return shown_.canHangUp;
    }

    [[nodiscard]] bool canMute() const noexcept
    {
        return shown_.canMute;
    }

    [[nodiscard]] const TrayPresentation &presentation() const noexcept
    {
        return shown_;
    }

Q_SIGNALS:

    void changed();

private:
    void refresh();

    AccountsViewModel &accounts_;
    CallViewModel &calls_;
    HistoryViewModel &history_;
    TrayPresentation shown_;
};

}
