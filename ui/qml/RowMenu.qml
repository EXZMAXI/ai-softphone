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
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Controls.Material

Item {
    id: rowMenu

    property Item acts: null

    property string number: ""

    property string numberToCopy: rowMenu.number

    property var extra: []

    anchors.fill: parent

    z: 1

    property var mayAnswer: null

    TapHandler {
        id: pressing

        acceptedButtons: Qt.RightButton

        onPressedChanged: {
            if (pressed
                && Application.styleHints.contextMenuTrigger === Qt.ContextMenuTrigger.Press) {
                rowMenu.offer(point.position)
            }
        }

        onTapped: {
            if (Application.styleHints.contextMenuTrigger === Qt.ContextMenuTrigger.Release) {
                rowMenu.offer(point.position)
            }
        }
    }

    function offer(where) {
        if (rowMenu.mayAnswer && !rowMenu.mayAnswer(where)) {
            return
        }
        rowMenu.show()
    }

    function show() {
        const rows = []
        for (const one of (rowMenu.acts ? rowMenu.acts.offered() : [])) {
            rows.push({
                label: one.text,
                offered: one.enabled,
                button: one,
                act: null,

                picture: one.icon.source
            })
        }
        for (const one of rowMenu.extra) {
            rows.push({
                label: one.label,
                offered: one.enabled === undefined ? true : one.enabled,
                button: null,
                act: one.act,
                picture: one.picture === undefined ? "" : one.picture
            })
        }
        if (rowMenu.numberToCopy !== "") {

            rows.push({
                label: qsTr("Copy the number"),
                offered: true,
                button: null,
                act: null,
                picture: settings.glyphSet + "/copy.svg"
            })
        }
        if (rows.length === 0) {
            return
        }

        entries.model = rows
        menu.present()
    }

    RightMenu {
        id: menu

        host: rowMenu

        Repeater {
            id: entries

            model: []

            TextMenuItem {
                required property var modelData

                text: modelData.label
                enabled: modelData.offered

                picture: modelData.picture

                onTriggered: {
                    if (modelData.button) {
                        modelData.button.clicked()
                    } else if (modelData.act) {
                        modelData.act()
                    } else {
                        settings.copyToClipboard(rowMenu.numberToCopy)
                    }
                }
            }
        }
    }
}
