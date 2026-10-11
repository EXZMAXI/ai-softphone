// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ScrollBar {
    id: bar

    property bool always: false

    policy: bar.always && bar.size < 1.0 ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded

    readonly property color quiet: Qt.rgba(Material.foreground.r, Material.foreground.g,
                                           Material.foreground.b, 0.28)
    readonly property color awake: Qt.rgba(Material.foreground.r, Material.foreground.g,
                                           Material.foreground.b, 0.42)
    readonly property color held: Qt.rgba(Material.foreground.r, Material.foreground.g,
                                          Material.foreground.b, 0.58)

    contentItem: Rectangle {
        implicitWidth: 6
        implicitHeight: 6

        radius: Math.min(width, height) / 2
        color: bar.pressed ? bar.held : (bar.hovered ? bar.awake : bar.quiet)

        opacity: 0.0
    }

    background: Rectangle {
        implicitWidth: 6
        implicitHeight: 6
        color: "transparent"
        opacity: 0.0
    }
}
