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

import Softphone

ToolButton {
    id: key

    property bool offered: true

    property bool marked: false

    property string drawing: ""
    property color tint: Material.foreground

    property string hint: ""

    signal pressedHere

    Layout.alignment: Qt.AlignVCenter
    implicitWidth: 30
    implicitHeight: 30
    padding: 0

    visible: key.offered
    focusPolicy: Qt.NoFocus
    hoverEnabled: true

    icon.source: key.drawing === "" ? "" : settings.glyphSet + "/" + key.drawing + ".svg"

    icon.width: 18
    icon.height: 18
    icon.color: key.marked ? "white" : key.tint

    Accessible.name: key.hint

    onClicked: key.pressedHere()

    background: Rectangle {
        radius: height / 2
        color: key.marked
               ? key.tint
               : key.down
                 ? Qt.rgba(key.tint.r, key.tint.g, key.tint.b, 0.22)
                 : key.hovered
                   ? Qt.rgba(key.tint.r, key.tint.g, key.tint.b, 0.12)
                   : "transparent"
    }

    Hint {
        owner: key
        visible: key.hovered && key.hint !== ""
        text: key.hint
    }
}
