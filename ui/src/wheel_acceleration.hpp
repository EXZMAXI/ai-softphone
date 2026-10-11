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
#include <QPointF>
#include <QPointer>
#include <QTimer>
#include <QtGlobal>

#include <deque>

class QPointingDevice;
class QWheelEvent;
class QWindow;

namespace softphone::ui {

class WheelAcceleration {
public:

    static constexpr qint64 kWindow = 200;

    static constexpr double kSlope = 2.0;

    static constexpr double kMost = 5.0;

    [[nodiscard]] double factorFor(qint64 atMs, int delta);

private:
    struct Part {
        qint64 at;
        int size;
    };

    std::deque<Part> lately_;
    int direction_{0};
};

class WheelAccelerator final : public QObject {
    Q_OBJECT

public:

    static constexpr int kSettleMs = 40;

    explicit WheelAccelerator(QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:

    void send(int delta, qint64 atMs);

    void settle();

    WheelAcceleration acceleration_;

    QPointer<QWindow> window_;
    QPointF position_;
    QPointF globalPosition_;
    Qt::MouseButtons buttons_;
    Qt::KeyboardModifiers modifiers_;
    bool inverted_{false};
    const QPointingDevice *device_{nullptr};
    qint64 lastAt_{0};

    int pending_{0};

    QTimer settling_;

    bool resending_{false};
};

}
