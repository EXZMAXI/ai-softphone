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

#include <QEvent>
#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QPointer>
#include <QString>
#include <QVariantMap>

#include <memory>

#include <QWindow>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

#include <QRect>
#include <QtQml/qqmlregistration.h>

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

namespace softphone::ui {

class Desktop : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `desktop`.")

    Q_PROPERTY(bool controlsTaskbarEntry READ controlsTaskbarEntry CONSTANT)

    Q_PROPERTY(bool edgesReservable READ edgesReservable CONSTANT)

    Q_PROPERTY(QWindow *focusWindow READ focusWindow NOTIFY focusWindowChanged)

public:
    explicit Desktop(QObject *parent = nullptr);
    ~Desktop() override;

    [[nodiscard]] bool controlsTaskbarEntry() const;

    [[nodiscard]] QWindow *focusWindow() const;

    Q_INVOKABLE void setTaskbarEntry(QObject *window, bool shown);

    Q_INVOKABLE bool reserveEdge(QObject *window, const QString &edge, int thickness);

    [[nodiscard]] static bool edgesReservable();

    bool eventFilter(QObject *watched, QEvent *event) override;

    void releaseTheEdge();

    Q_INVOKABLE void bringToFront(QObject *window);

    Q_INVOKABLE void keepPlace(QObject *window, int x, int y);

    Q_INVOKABLE void learnFrame(QObject *window);

    [[nodiscard]] Q_INVOKABLE QPoint frameOffset() const;

    Q_INVOKABLE void placeNow(QObject *window, int x, int y);

    [[nodiscard]] Q_INVOKABLE bool pointerHeld() const;

    [[nodiscard]] Q_INVOKABLE QString shortcutText(int standardKey) const;

    Q_INVOKABLE [[nodiscard]] QRect noticePlace(int width, int height) const;

    Q_INVOKABLE [[nodiscard]] QVariantMap panelPlace(QObject *phone, int phoneX, int phoneY,
                                                     int panelWidth, int takenRight,
                                                     int takenLeft) const;

    Q_INVOKABLE [[nodiscard]] int panelStand(QObject *phone, int phoneX, int phoneY, int panelWidth,
                                             int taken, bool right) const;

private:

    QPoint learned_{0, 0};

    std::unique_ptr<QWindow> frameProbe_;

    bool roomIsKept_{true};

    bool roomWasAsked_{false};

    QPointer<QWindow> reserving_;
    QString reservedEdge_{QStringLiteral("top")};
    int reservedThickness_{0};

    void lookForTheRoom(int attempt);

    QTimer *checking_{nullptr};

signals:

    void focusWindowChanged();
};

}
