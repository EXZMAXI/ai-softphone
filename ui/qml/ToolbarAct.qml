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
import QtQuick.Layouts

ToolButton {
    id: act

    property bool offered: true

    display: AbstractButton.TextBesideIcon

    icon.width: 18
    icon.height: 18

    spacing: 6

    property bool marked: false

    implicitHeight: 34
    Layout.alignment: Qt.AlignVCenter

    topPadding: 0
    bottomPadding: 0

    leftPadding: 10
    rightPadding: 10

    font.pixelSize: 13
    hoverEnabled: true

    Rectangle {
        id: dot

        visible: act.marked
        width: 7
        height: 7
        radius: width / 2
        color: Mark.bad

        anchors.left: parent.left
        anchors.leftMargin: act.leftPadding + act.icon.width - 3
        anchors.top: parent.top
        anchors.topMargin: (act.height - act.icon.height) / 2 - 2
    }

    background: Rectangle {
        radius: height / 2
        color: act.down
               ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                         Material.foreground.b, 0.16)
               : act.hovered
                 ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                           Material.foreground.b, 0.08)
                 : "transparent"
    }
}
