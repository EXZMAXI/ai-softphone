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

#include <QPoint>

#include <functional>
#include <optional>
#include <string_view>

class QWindow;

namespace softphone::ui::platform {

enum class Away {

    Locked,

    Unlocked,

    Sleeping,
};

using Leaving = std::function<void(Away what)>;

[[nodiscard]] bool watchForLeaving(Leaving told);

void stopWatchingForLeaving();

[[nodiscard]] bool controlsTaskbarEntry();

void setTaskbarEntry(QWindow &window, bool shown);

enum class ScreenEdge { Top, Bottom };

bool reserveEdge(QWindow &window, ScreenEdge edge, int thickness);

void releaseEdge(QWindow &window);

[[nodiscard]] bool edgesCanBeReserved();

void giveBackTheRoom();

[[nodiscard]] bool pointerHeld();

void bringToFront(QWindow &window);

void allowAnotherProcessToRaise();

[[nodiscard]] std::optional<QPoint> whereOnScreen(const QWindow &window);

void askForFrameOffset(QWindow &window);

[[nodiscard]] std::optional<QPoint> frameOffset(const QWindow &window);

}
