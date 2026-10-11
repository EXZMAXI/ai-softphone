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

    required property string picture

    required property string glyphs

    property bool shown: true

    property string hint: text

    visible: shown

    implicitWidth: 40
    implicitHeight: 40
    padding: 0
    Layout.alignment: Qt.AlignVCenter

    display: AbstractButton.IconOnly
    icon.source: act.glyphs === "" ? "" : act.glyphs + "/" + act.picture + ".svg"
    icon.width: 18
    icon.height: 18
    icon.color: Material.foreground

    opacity: act.enabled ? 0.7 : 0.3

    Accessible.name: act.hint

    Loader {
        active: act.hovered && act.hint !== ""

        sourceComponent: Hint {
            owner: act
            visible: true
            text: act.hint
        }
    }

    background: Rectangle {
        anchors.centerIn: parent
        width: 32
        height: 32
        radius: width / 2
        color: act.down
               ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                         Material.foreground.b, 0.16)
               : (act.hovered
                  ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                            Material.foreground.b, 0.10)
                  : "transparent")
    }
}
