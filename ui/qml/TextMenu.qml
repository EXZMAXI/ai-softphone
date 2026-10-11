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
    id: attached

    property Item field: parent

    property var extra: []

    readonly property bool editable: attached.field
                                     && attached.field.readOnly !== undefined
                                     && !attached.field.readOnly

    readonly property bool hasSelection: attached.field
                                         && attached.field.selectedText !== undefined
                                         && attached.field.selectedText.length > 0
    readonly property bool hasText: attached.field && attached.field.text !== undefined
                                    && attached.field.text.length > 0

    anchors.fill: parent

    z: 1

    TapHandler {
        acceptedButtons: Qt.RightButton

        onPressedChanged: {
            if (pressed
                && Application.styleHints.contextMenuTrigger === Qt.ContextMenuTrigger.Press) {
                attached.show()
            }
        }

        onTapped: {
            if (Application.styleHints.contextMenuTrigger === Qt.ContextMenuTrigger.Release) {
                attached.show()
            }
        }
    }

    function show() {
        if (attached.field && attached.field.forceActiveFocus) {
            attached.field.forceActiveFocus()
        }
        menu.present()
    }

    RightMenu {
        id: menu

        host: attached

        TextMenuItem {

            text: qsTr("Undo")
            picture: settings.glyphSet + "/undo.svg"
            standardKey: StandardKey.Undo
            visible: attached.editable
            enabled: attached.field && attached.field.canUndo
            onTriggered: attached.field.undo()
        }

        TextMenuItem {

            text: qsTr("Redo")
            picture: settings.glyphSet + "/redo.svg"
            standardKey: StandardKey.Redo
            visible: attached.editable
            enabled: attached.field && attached.field.canRedo
            onTriggered: attached.field.redo()
        }

        MenuSeparator {
            visible: attached.editable
            height: visible ? implicitHeight : 0
        }

        TextMenuItem {

            text: qsTr("Cut")
            picture: settings.glyphSet + "/cut.svg"
            standardKey: StandardKey.Cut
            visible: attached.editable
            enabled: attached.hasSelection
            onTriggered: attached.field.cut()
        }

        TextMenuItem {

            text: qsTr("Copy")
            picture: settings.glyphSet + "/copy.svg"
            standardKey: StandardKey.Copy
            enabled: attached.hasSelection
            onTriggered: attached.field.copy()
        }

        TextMenuItem {

            text: qsTr("Paste")
            picture: settings.glyphSet + "/paste.svg"
            standardKey: StandardKey.Paste
            visible: attached.editable
            enabled: attached.field && attached.field.canPaste
            onTriggered: attached.field.paste()
        }

        MenuSeparator {}

        TextMenuItem {

            text: qsTr("Select all")
            picture: settings.glyphSet + "/select-all.svg"
            standardKey: StandardKey.SelectAll
            enabled: attached.hasText
            onTriggered: attached.field.selectAll()
        }

        MenuSeparator {
            visible: attached.extra.length > 0
            height: visible ? implicitHeight : 0
        }

        Repeater {
            model: attached.extra

            TextMenuItem {
                required property var modelData

                text: modelData.label
                picture: modelData.picture
                onTriggered: modelData.act()
            }
        }
    }
}
