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
    id: rowButton

    required property string glyph

    required property string glyphs

    property int box: 40

    property int target: 40
    property int circle: 32

    property int pictureSize: 18

    property color tint: Material.foreground
    property real tintOpacity: 0.7

    implicitWidth: rowButton.box
    implicitHeight: rowButton.target
    padding: 0
    Layout.alignment: Qt.AlignVCenter

    display: AbstractButton.IconOnly
    icon.source: rowButton.glyphs === "" ? "" : rowButton.glyphs + "/" + rowButton.glyph + ".svg"
    icon.width: rowButton.pictureSize
    icon.height: rowButton.pictureSize
    icon.color: rowButton.tint

    opacity: rowButton.tintOpacity

    Accessible.name: rowButton.text

    Loader {
        active: rowButton.hovered && rowButton.text !== ""

        sourceComponent: Hint {
            owner: rowButton
            visible: true
            text: rowButton.text
        }
    }

    background: Rectangle {
        anchors.centerIn: parent
        width: rowButton.circle
        height: rowButton.circle
        radius: width / 2
        color: rowButton.down
               ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                         Material.foreground.b, 0.16)
               : (rowButton.hovered
                  ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                            Material.foreground.b, 0.10)
                  : "transparent")
    }
}
