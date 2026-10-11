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

#include <softphone/core/updates.hpp>

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QTimer>

#include <QtQml/qqmlregistration.h>
#include <chrono>

namespace softphone::ui {

class UpdateViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by the shell, which gives it to QML as a context property")

    Q_PROPERTY(State state READ state NOTIFY changed)

    Q_PROPERTY(QString offered READ offered NOTIFY changed)

    Q_PROPERTY(bool selfReplacing READ selfReplacing NOTIFY changed)

    Q_PROPERTY(bool hasDownload READ hasDownload NOTIFY changed)
    Q_PROPERTY(bool hasNotes READ hasNotes NOTIFY changed)

    Q_PROPERTY(double progress READ progress NOTIFY changed)

    Q_PROPERTY(QString complaint READ complaint NOTIFY changed)

    Q_PROPERTY(QString lastChecked READ lastChecked NOTIFY changed)

    Q_PROPERTY(bool networkReachable READ networkReachable NOTIFY networkReachableChanged)

    Q_PROPERTY(bool announcing READ announcing NOTIFY changed)

public:

    enum class State {
        Unconfigured,
        Idle,
        Checking,
        UpToDate,
        Available,
        Downloading,
        Ready,
        Failed,
    };
    Q_ENUM(State)

    explicit UpdateViewModel(core::UpdateService *service, QObject *parent = nullptr);
    ~UpdateViewModel() override;

    [[nodiscard]] State state() const noexcept
    {
        return state_;
    }

    [[nodiscard]] QString offered() const
    {
        return offered_;
    }

    [[nodiscard]] bool selfReplacing() const noexcept
    {
        return selfReplacing_;
    }

    [[nodiscard]] bool hasDownload() const
    {
        return !downloadUrl_.isEmpty();
    }

    [[nodiscard]] bool hasNotes() const
    {
        return !notes_.isEmpty();
    }

    [[nodiscard]] double progress() const noexcept;

    [[nodiscard]] QString lastChecked() const
    {
        return lastChecked_;
    }

    [[nodiscard]] QString complaint() const
    {
        return complaint_;
    }

    [[nodiscard]] bool announcing() const noexcept
    {
        return announcing_;
    }

    [[nodiscard]] bool networkReachable() const;

    void inACall(bool busy);

    void capturing(bool busy);

    void waitAfterACall(std::chrono::milliseconds pause)
    {
        afterACall_ = pause;
    }

    void automatic(bool wanted);

    Q_INVOKABLE void check();

    Q_INVOKABLE void fetch();

    Q_INVOKABLE void install();

    Q_INVOKABLE void openDownload();
    Q_INVOKABLE void openNotes();

    Q_INVOKABLE void downloadAndInstall();

    Q_INVOKABLE void notNow();

    Q_PROPERTY(bool heldBack READ heldBack NOTIFY changed)

    [[nodiscard]] bool heldBack() const noexcept
    {
        return heldBack_;
    }

Q_SIGNALS:
    void changed();

    void networkReachableChanged();

    void restartWanted(const QString &image);

private:
    void adopt(const core::UpdateStatus &status);

    void considerAnnouncing(std::chrono::milliseconds after);

    void busyChanged(bool nowBusy);

    void installUnlessBusy();

    core::UpdateService *service_{nullptr};

    State state_{State::Unconfigured};
    QString offered_;
    QString notes_;
    QString downloadUrl_;
    QString complaint_;
    QString lastChecked_;
    bool selfReplacing_{false};
    quint64 received_{0};
    quint64 total_{0};

    QTimer daily_;
    QTimer first_;

    QTimer settle_;

    bool announcing_{false};
    bool inACall_{false};
    bool capturing_{false};

    std::chrono::milliseconds afterACall_;

    bool installWhenReady_{false};

    bool heldBack_{false};

    QDateTime toldAt_;
};

}
