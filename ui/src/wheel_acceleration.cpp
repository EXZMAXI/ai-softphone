// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "wheel_acceleration.hpp"

#include <QCoreApplication>
#include <QInputDevice>
#include <QPointingDevice>
#include <QWheelEvent>
#include <QWindow>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace softphone::ui {
namespace {

constexpr int kNotch = 120;

}

double WheelAcceleration::factorFor(qint64 atMs, int delta)
{
    const int direction = delta > 0 ? 1 : (delta < 0 ? -1 : 0);
    if (direction == 0) {
        return 1.0;
    }
    if (direction != direction_) {
        lately_.clear();
        direction_ = direction;
    }

    while (!lately_.empty() && atMs - lately_.front().at >= kWindow) {
        lately_.pop_front();
    }
    lately_.push_back(Part{atMs, std::abs(delta)});

    int turned = 0;
    for (const auto &part : lately_) {
        turned += part.size;
    }

    const double notches = static_cast<double>(turned) / kNotch;
    return std::clamp(1.0 + (notches - 1.0) * kSlope, 1.0, kMost);
}

WheelAccelerator::WheelAccelerator(QObject *parent) : QObject{parent}
{
    settling_.setSingleShot(true);
    settling_.setInterval(kSettleMs);
    connect(&settling_, &QTimer::timeout, this, &WheelAccelerator::settle);
}

bool WheelAccelerator::eventFilter(QObject *watched, QEvent *event)
{
#if defined(Q_OS_MACOS)

    static_cast<void>(watched);
    static_cast<void>(event);
    return false;
#else
    if (resending_ || event->type() != QEvent::Wheel || !watched->isWindowType()) {
        return false;
    }

    const auto *wheel = static_cast<QWheelEvent *>(event);
    const auto delta = wheel->angleDelta().y();
    const auto *device = wheel->pointingDevice();

    if (delta == 0 || wheel->angleDelta().x() != 0 || wheel->phase() != Qt::NoScrollPhase ||
        (device != nullptr && device->type() == QInputDevice::DeviceType::TouchPad)) {
        return false;
    }

    auto *window = static_cast<QWindow *>(watched);
    const auto atMs = static_cast<qint64>(wheel->timestamp());

    if (pending_ != 0 && ((pending_ > 0) != (delta > 0) || window_ != window)) {
        settle();
    }

    window_ = window;
    position_ = wheel->position();
    globalPosition_ = wheel->globalPosition();
    buttons_ = wheel->buttons();
    modifiers_ = wheel->modifiers();
    inverted_ = wheel->inverted();
    device_ = device;
    lastAt_ = atMs;

    pending_ += delta;
    while (std::abs(pending_) >= kNotch) {
        const int notch = pending_ > 0 ? kNotch : -kNotch;
        pending_ -= notch;
        send(notch, atMs);
    }

    if (pending_ != 0) {
        settling_.start();
    } else {
        settling_.stop();
    }

    event->accept();
    return true;
#endif
}

void WheelAccelerator::send(int delta, qint64 atMs)
{
    if (window_.isNull()) {
        return;
    }

    const auto factor = acceleration_.factorFor(atMs, delta);
    QWheelEvent turned(position_, globalPosition_, QPoint{},
                       QPoint{0, static_cast<int>(std::lround(delta * factor))}, buttons_,
                       modifiers_, Qt::NoScrollPhase, inverted_, Qt::MouseEventNotSynthesized,
                       device_ != nullptr ? device_ : QPointingDevice::primaryPointingDevice());
    turned.setTimestamp(static_cast<quint64>(atMs));

    resending_ = true;
    QCoreApplication::sendEvent(window_.data(), &turned);
    resending_ = false;
}

void WheelAccelerator::settle()
{
    settling_.stop();
    if (pending_ == 0) {
        return;
    }
    const int left = pending_;
    pending_ = 0;
    send(left, lastAt_ + kSettleMs);
}

}
