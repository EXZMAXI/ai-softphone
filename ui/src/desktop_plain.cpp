// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "desktop_platform.hpp"

#include <softphone/core/logging.hpp>

#include <QMargins>
#include <QPoint>
#include <QWindow>

#include <optional>

namespace softphone::ui::platform {

bool controlsTaskbarEntry()
{
    return false;
}

bool edgesCanBeReserved()
{

    return false;
}

void releaseEdge(QWindow & )
{

}

bool reserveEdge(QWindow & , ScreenEdge , int )
{

    return false;
}

void setTaskbarEntry(QWindow & , bool )
{
    core::Logger::instance().log(core::LogLevel::Debug, "ui",
                                 "this build cannot take a window out of the task bar");
}

bool pointerHeld()
{

    return false;
}

void bringToFront(QWindow & )
{

}

void allowAnotherProcessToRaise()
{

}

std::optional<QPoint> whereOnScreen(const QWindow & )
{
    return std::nullopt;
}

void giveBackTheRoom()
{

}

void askForFrameOffset(QWindow & )
{

}

std::optional<QPoint> frameOffset(const QWindow &window)
{

    const QMargins frame = window.frameMargins();
    return QPoint{frame.left(), frame.top()};
}

}
