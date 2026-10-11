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

#include <QList>
#include <QRect>
#include <QSize>

namespace softphone::ui {

inline constexpr double kVisibleFraction = 0.25;
inline constexpr int kVisibleWidth = 120;
inline constexpr int kVisibleHeight = 60;

inline constexpr int kTitleBarReserve = 48;

[[nodiscard]] QRect placeWindow(const QRect &saved, const QList<QRect> &screens,
                                const QSize &minimum);

[[nodiscard]] QRect placeNotice(const QRect &screen, const QSize &notice, bool rightToLeft);

struct PanelPlace {
    int x{0};
    int y{0};

    bool right{true};

    bool stuck{true};
};

[[nodiscard]] PanelPlace placePanel(const QRect &phone, int panelWidth, const QRect &screen,
                                    int takenRight, int takenLeft);

[[nodiscard]] int standAgainst(const QRect &phone, int panelWidth, const QRect &screen, int taken,
                               bool right);

}
