// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "desktop.hpp"

#include <QKeySequence>

#include "desktop_platform.hpp"
#include "window_placement.hpp"

#include <softphone/core/logging.hpp>

#include <QGuiApplication>
#include <QPlatformSurfaceEvent>
#include <QPointer>
#include <QScreen>
#include <QTimer>
#include <QWindow>

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <vector>

namespace softphone::ui {
namespace {

[[nodiscard]] platform::ScreenEdge edgeCalled(const QString &edge)
{
    return edge == QLatin1String("bottom") ? platform::ScreenEdge::Bottom
                                           : platform::ScreenEdge::Top;
}

}

Desktop::Desktop(QObject *parent) : QObject{parent}
{
    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this,
            [this](QWindow *) { emit focusWindowChanged(); });

    connect(qGuiApp, &QGuiApplication::aboutToQuit, this, [this] { releaseTheEdge(); });
}

Desktop::~Desktop()
{

    releaseTheEdge();
}

void Desktop::releaseTheEdge()
{

    if (reservedThickness_ <= 0) {
        return;
    }
    reservedThickness_ = 0;

    core::Logger::instance().log(core::LogLevel::Info, "ui",
                                 "giving the desktop back the room the strip reserved");

    platform::giveBackTheRoom();
    reserving_.clear();
}

QWindow *Desktop::focusWindow() const
{
    return QGuiApplication::focusWindow();
}

bool Desktop::controlsTaskbarEntry() const
{
    return platform::controlsTaskbarEntry();
}

QRect Desktop::noticePlace(int width, int height) const
{
    const auto *screen = QGuiApplication::primaryScreen();
    if (screen == nullptr) {
        return {};
    }

    return placeNotice(screen->availableGeometry(), QSize{width, height},
                       QGuiApplication::layoutDirection() == Qt::RightToLeft);
}

QVariantMap Desktop::panelPlace(QObject *phone, int phoneX, int phoneY, int panelWidth,
                                int takenRight, int takenLeft) const
{
    const auto *window = qobject_cast<QWindow *>(phone);
    if (window == nullptr) {
        return {};
    }

    const auto *screen = window->screen();
    const QRect area = screen == nullptr ? QRect{} : screen->availableGeometry();
    const QRect where{phoneX, phoneY, window->width(), window->height()};

    const auto place = placePanel(where, panelWidth, area, takenRight, takenLeft);

    QVariantMap answer;
    answer.insert(QStringLiteral("x"), place.x);
    answer.insert(QStringLiteral("y"), place.y);
    answer.insert(QStringLiteral("right"), place.right);
    answer.insert(QStringLiteral("stuck"), place.stuck);
    return answer;
}

int Desktop::panelStand(QObject *phone, int phoneX, int phoneY, int panelWidth, int taken,
                        bool right) const
{
    const auto *window = qobject_cast<QWindow *>(phone);
    if (window == nullptr) {
        return phoneX;
    }

    const auto *screen = window->screen();
    const QRect area = screen == nullptr ? QRect{} : screen->availableGeometry();
    const QRect where{phoneX, phoneY, window->width(), window->height()};

    return standAgainst(where, panelWidth, area, taken, right);
}

bool Desktop::pointerHeld() const
{
    return platform::pointerHeld();
}

QString Desktop::shortcutText(int standardKey) const
{
    const QKeySequence keys{static_cast<QKeySequence::StandardKey>(standardKey)};

    return keys.toString(QKeySequence::NativeText);
}

void Desktop::placeNow(QObject *window, int x, int y)
{
    auto *asWindow = qobject_cast<QWindow *>(window);
    if (asWindow == nullptr) {
        return;
    }

    if (checking_ != nullptr) {
        checking_->stop();
    }

    asWindow->setPosition(QPoint{x, y});
}

void Desktop::keepPlace(QObject *window, int x, int y)
{
    auto *asWindow = qobject_cast<QWindow *>(window);
    if (asWindow == nullptr) {
        return;
    }

    const QPoint wanted{x, y};

    asWindow->setPosition(asWindow->isVisible() ? wanted : wanted + learned_);

    constexpr int kAnswerMs = 120;

    if (checking_ == nullptr) {
        checking_ = new QTimer{this};
        checking_->setSingleShot(true);
    }

    checking_->disconnect();
    connect(checking_, &QTimer::timeout, this, [this, asWindow, wanted]() {

        const auto actual = platform::whereOnScreen(*asWindow);
        if (!actual.has_value() || *actual == wanted) {
            return;
        }

        const auto correction = wanted - *actual;

        constexpr int kFrameLimit = 64;
        if (std::abs(correction.x()) <= kFrameLimit && std::abs(correction.y()) <= kFrameLimit) {
            learned_ += correction;
        }

        core::Logger::instance().log(core::LogLevel::Debug, "ui",
                                     "the desktop placed a window " +
                                         std::to_string(correction.y()) +
                                         " pixels from where it was asked; correcting, and "
                                         "remembering for next time");

        asWindow->setPosition(wanted + learned_);
    });

    checking_->start(kAnswerMs);
}

void Desktop::learnFrame(QObject *window)
{
    auto *asWindow = qobject_cast<QWindow *>(window);
    if (asWindow == nullptr || frameProbe_ != nullptr) {
        return;
    }

    frameProbe_ = std::make_unique<QWindow>();
    frameProbe_->setFlags(asWindow->flags());
    frameProbe_->setGeometry(QRect{-4000, -4000, 100, 100});
    frameProbe_->create();

    platform::askForFrameOffset(*frameProbe_);
}

QPoint Desktop::frameOffset() const
{
    if (frameProbe_ == nullptr) {
        return QPoint{0, 0};
    }

    return platform::frameOffset(*frameProbe_).value_or(QPoint{0, 0});
}

void Desktop::bringToFront(QObject *window)
{
    auto *asWindow = qobject_cast<QWindow *>(window);
    if (asWindow == nullptr) {
        return;
    }

    platform::bringToFront(*asWindow);
}

bool Desktop::edgesReservable()
{
    return platform::edgesCanBeReserved();
}

namespace {

constexpr int kFirstLookMs = 1200;
constexpr int kNextLookMs = 1500;
constexpr int kLooks = 3;

[[nodiscard]] bool roomWasGiven(const QWindow &window, const QString &edge, int thickness)
{
    const auto *screen = window.screen();
    if (screen == nullptr || thickness <= 0) {
        return false;
    }

    constexpr int kSlack = 2;
    const auto left = screen->availableGeometry();
    const auto top = window.y();

    return edge == QLatin1String("bottom") ? left.bottom() <= top + kSlack
                                           : left.top() + kSlack >= top + thickness;
}

}

void Desktop::lookForTheRoom(int attempt)
{
    if (roomWasAsked_ || reserving_.isNull()) {
        return;
    }

    if (roomWasGiven(*reserving_, reservedEdge_, reservedThickness_)) {

        roomWasAsked_ = true;
        return;
    }

    if (attempt < kLooks) {
        QTimer::singleShot(kNextLookMs, this, [this, attempt]() { lookForTheRoom(attempt + 1); });
        return;
    }

    roomWasAsked_ = true;
    roomIsKept_ = false;
    platform::releaseEdge(*reserving_);

    core::Logger::instance().log(
        core::LogLevel::Info, "ui",
        "this session keeps no room for a strip along an edge, so the phone stops standing above "
        "other windows");
}

bool Desktop::eventFilter(QObject *watched, QEvent *event)
{

    if (event != nullptr && event->type() == QEvent::PlatformSurface &&
        watched == reserving_.data()) {
        const auto *surface = static_cast<QPlatformSurfaceEvent *>(event);
        if (surface->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            releaseTheEdge();
        }
    }
    return QObject::eventFilter(watched, event);
}

bool Desktop::reserveEdge(QObject *window, const QString &edge, int thickness)
{
    auto *asWindow = qobject_cast<QWindow *>(window);
    if (asWindow == nullptr) {
        return false;
    }

    if (!roomIsKept_) {
        platform::releaseEdge(*asWindow);
        return false;
    }

    if (thickness > 0) {
        if (reserving_ != asWindow) {
            if (!reserving_.isNull()) {
                reserving_->removeEventFilter(this);
            }
            asWindow->installEventFilter(this);
        }
        reserving_ = asWindow;
        reservedEdge_ = edge;
        reservedThickness_ = thickness;
    } else {
        if (!reserving_.isNull()) {
            reserving_->removeEventFilter(this);
        }
        reserving_.clear();
        reservedThickness_ = 0;
    }

    const auto asked = platform::reserveEdge(*asWindow, edgeCalled(edge), thickness);

    if (asked && thickness > 0 && !roomWasAsked_) {
        QTimer::singleShot(kFirstLookMs, this, [this]() { lookForTheRoom(1); });
    }

    return asked;
}

void Desktop::setTaskbarEntry(QObject *window, bool shown)
{
    auto *asWindow = qobject_cast<QWindow *>(window);
    if (asWindow == nullptr) {
        return;
    }

    platform::setTaskbarEntry(*asWindow, shown);
}

}
