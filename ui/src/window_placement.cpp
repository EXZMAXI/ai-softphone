// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "window_placement.hpp"

#include <algorithm>

namespace softphone::ui {
namespace {

[[nodiscard]] bool showsEnough(const QRect &window, const QRect &screen)
{
    const auto visible = window.intersected(screen);
    if (visible.isEmpty()) {
        return false;
    }

    const auto area = static_cast<double>(visible.width()) * visible.height();
    const auto whole = static_cast<double>(window.width()) * window.height();

    return area >= whole * kVisibleFraction ||
           (visible.width() >= kVisibleWidth && visible.height() >= kVisibleHeight);
}

}

QRect placeWindow(const QRect &saved, const QList<QRect> &screens, const QSize &minimum)
{
    if (screens.isEmpty()) {
        return saved;
    }

    const QRect *best = &screens.first();
    int most = -1;
    for (const auto &screen : screens) {
        const auto visible = saved.intersected(screen);
        const auto area = visible.width() * visible.height();
        if (area > most) {
            most = area;
            best = &screen;
        }
    }

    const auto &primary = screens.first();
    const auto onScreen = showsEnough(saved, *best) ? *best : primary;

    QRect placed = saved;

    placed.setWidth(std::max(std::min(saved.width(), onScreen.width()), minimum.width()));
    placed.setHeight(
        std::max(std::min(saved.height(), onScreen.height() - kTitleBarReserve), minimum.height()));

    if (!showsEnough(saved, *best)) {

        placed.moveLeft(onScreen.left() + (onScreen.width() - placed.width()) / 2);
        placed.moveTop(onScreen.top() + (onScreen.height() - placed.height()) / 2);
        return placed;
    }

    if (placed.right() > onScreen.right()) {
        placed.moveRight(onScreen.right());
    }
    if (placed.bottom() > onScreen.bottom()) {
        placed.moveBottom(onScreen.bottom());
    }
    if (placed.left() < onScreen.left()) {
        placed.moveLeft(onScreen.left());
    }

    if (placed.top() < onScreen.top() + kTitleBarReserve) {
        placed.moveTop(onScreen.top() + kTitleBarReserve);
    }

    return placed;
}

QRect placeNotice(const QRect &screen, const QSize &notice, bool rightToLeft)
{

    const QSize size{std::min(notice.width(), screen.width()),
                     std::min(notice.height(), screen.height())};

    const int y = screen.bottom() + 1 - size.height();
    const int x = rightToLeft ? screen.left() : screen.right() + 1 - size.width();

    return QRect{QPoint{std::max(screen.left(), x), std::max(screen.top(), y)}, size};
}

namespace {

[[nodiscard]] bool wholeOn(int x, int panelWidth, const QRect &screen)
{
    return x >= screen.x() && x + panelWidth <= screen.x() + screen.width();
}

[[nodiscard]] int inTheRow(const QRect &phone, int panelWidth, int taken, bool right)
{
    return right ? phone.x() + phone.width() + taken : phone.x() - taken - panelWidth;
}

[[nodiscard]] int atThePhone(const QRect &phone, int panelWidth, bool right)
{
    return right ? phone.x() + phone.width() : phone.x() - panelWidth;
}

}

int standAgainst(const QRect &phone, int panelWidth, const QRect &screen, int taken, bool right)
{
    const auto row = inTheRow(phone, panelWidth, taken, right);

    if (screen.isEmpty() || wholeOn(row, panelWidth, screen)) {
        return row;
    }

    const auto flush = atThePhone(phone, panelWidth, right);
    if (wholeOn(flush, panelWidth, screen)) {
        return flush;
    }

    return right ? std::max(screen.x(), screen.x() + screen.width() - panelWidth) : screen.x();
}

PanelPlace placePanel(const QRect &phone, int panelWidth, const QRect &screen, int takenRight,
                      int takenLeft)
{
    PanelPlace place;
    place.y = phone.y();

    if (screen.isEmpty()) {
        place.x = inTheRow(phone, panelWidth, takenRight, true);
        place.right = true;
        return place;
    }

    if (wholeOn(inTheRow(phone, panelWidth, takenRight, true), panelWidth, screen)) {
        place.x = inTheRow(phone, panelWidth, takenRight, true);
        place.right = true;
        return place;
    }

    if (wholeOn(inTheRow(phone, panelWidth, takenLeft, false), panelWidth, screen)) {
        place.x = inTheRow(phone, panelWidth, takenLeft, false);
        place.right = false;
        return place;
    }

    if (wholeOn(atThePhone(phone, panelWidth, true), panelWidth, screen)) {
        place.x = atThePhone(phone, panelWidth, true);
        place.right = true;
        return place;
    }

    if (wholeOn(atThePhone(phone, panelWidth, false), panelWidth, screen)) {
        place.x = atThePhone(phone, panelWidth, false);
        place.right = false;
        return place;
    }

    const auto phoneIsOnTheLeft = phone.x() + phone.width() / 2 < screen.x() + screen.width() / 2;
    place.x = standAgainst(phone, panelWidth, screen, 0, phoneIsOnTheLeft);
    place.right = phoneIsOnTheLeft;
    place.stuck = false;
    return place;
}

}
