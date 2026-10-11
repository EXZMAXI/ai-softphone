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

MenuItem {
    id: entry

    horizontalPadding: 12
    verticalPadding: 6

    implicitHeight: 32

    height: entry.visible ? implicitHeight : 0

    property int standardKey: StandardKey.UnknownKey

    property string picture: ""

    property bool marked: false

    readonly property string keys: entry.standardKey === StandardKey.UnknownKey
                                   ? ""
                                   : desktop.shortcutText(entry.standardKey)

    contentItem: Item {
        implicitWidth: 18 + 8 + label.implicitWidth
                       + (keys.visible ? keys.implicitWidth + 24 : 0)
        implicitHeight: Math.max(label.implicitHeight, keys.implicitHeight)

        Glyph {
            id: drawing

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 18
            height: 18
            source: entry.picture
            color: entry.enabled ? entry.Material.foreground : entry.Material.hintTextColor
            opacity: entry.enabled ? 0.8 : 0.5
        }

        Rectangle {
            visible: entry.marked
            width: 7
            height: 7
            radius: width / 2
            color: Mark.bad

            x: drawing.x + drawing.width - 3
            y: drawing.y - 2
        }

        Label {
            id: label

            anchors.left: parent.left
            anchors.leftMargin: 18 + 8
            anchors.verticalCenter: parent.verticalCenter
            text: entry.text
            color: entry.enabled ? entry.Material.foreground : entry.Material.hintTextColor
        }

        Label {
            id: keys

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: entry.keys
            visible: entry.keys !== "" && Application.styleHints.showShortcutsInContextMenus
            color: entry.Material.hintTextColor
            font.pixelSize: 12
        }
    }
}
