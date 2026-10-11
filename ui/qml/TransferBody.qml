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

ColumnLayout {
    id: body

    required property var page

    property bool arranged: false

    property bool keysShown: false

    signal finished()

    function begin() {
        transferTarget.text = ""
        body.arranged = false
        body.keysShown = false
    }

    function takeTheKeyboard() {
        transferTarget.forceActiveFocus()
    }

    spacing: 12

    focus: true

    Keys.onEscapePressed: (event) => {
        body.finished()
        event.accepted = true
    }

    DialField {
        id: transferTarget

        implicitHeight: body.page.fieldHeight

        Layout.fillWidth: true
        cleaner: calls

        rightPadding: transferFieldButton.width + 4
        placeholderText: qsTr("Number or address")
        onAccepted: {
            if (text.trim().length === 0) {
                return
            }
            body.arranged = true
            calls.transferNow(text)
            body.finished()
        }

        ToolButton {
            id: transferFieldButton

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            flat: true
            focusPolicy: Qt.NoFocus
            text: transferTarget.text.length > 0
                  ? body.page.clearGlyph
                  : (body.keysShown ? body.page.collapseGlyph : body.page.expandGlyph)
            onClicked: {
                if (transferTarget.text.length > 0) {
                    transferTarget.clear()
                } else {
                    body.keysShown = !body.keysShown
                }
            }
        }
    }

    GridLayout {
        Layout.fillWidth: true
        visible: body.keysShown
        columns: 3
        columnSpacing: 6
        rowSpacing: 6

        Repeater {
            model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "*", "0", "#"]

            Button {
                required property string modelData

                Layout.fillWidth: true
                Layout.preferredHeight: 40
                flat: true

                focusPolicy: Qt.NoFocus
                font.pixelSize: 18
                text: modelData
                onClicked: transferTarget.text = transferTarget.text + modelData
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 12

        DialogButton {
            enabled: transferTarget.text.trim().length > 0
            highlighted: true
            glyph: "transfer"
            text: qsTr("Transfer now")
            onClicked: {
                body.arranged = true
                calls.transferNow(transferTarget.text)
                body.finished()
            }
        }

        DialogButton {
            enabled: transferTarget.text.trim().length > 0

            glyph: "call"
            text: qsTr("Ask first")
            onClicked: {

                body.arranged = true
                calls.askFirst(transferTarget.text)
                body.finished()
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: transferChoices.count > 0
        text: qsTr("Or hand it to a call you already have")
        opacity: 0.7
        font.pixelSize: 12
        wrapMode: Text.WordWrap
    }

    ListView {
        id: transferChoices

        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(count, 3) * 44
        visible: count > 0
        clip: true
        interactive: contentHeight > height

        model: calls.transferTargets

        delegate: ItemDelegate {
            required property var modelData

            width: transferChoices.width
            height: 44
            text: modelData.number.length > 0
                  ? qsTr("%1 (%2)").arg(modelData.who).arg(modelData.number)
                  : modelData.who
            onClicked: {
                body.arranged = true
                calls.transferToCall(modelData.callId)
                body.finished()
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 12

        DialogButton {

            Layout.fillWidth: false
            flat: true
            Material.foreground: Material.Red
            glyph: "call-end"
            text: qsTr("Hang up")
            onClicked: {

                body.arranged = true
                calls.hangUp()
                body.finished()
            }
        }

        Item {
            Layout.fillWidth: true
        }

        DialogButton {
            Layout.fillWidth: false
            flat: true
            glyph: "dismiss"
            text: qsTr("Cancel")
            onClicked: body.finished()
        }
    }

    component DialogButton: Button {
        id: dialogButton

        required property string glyph

        topInset: 0
        bottomInset: 0
        topPadding: body.page.cardButtonPad
        bottomPadding: body.page.cardButtonPad

        Layout.fillWidth: true

        Layout.preferredHeight: body.page.cardButtonHeight
        focusPolicy: Qt.NoFocus

        display: AbstractButton.IconOnly
        icon.source: body.page.glyph(dialogButton.glyph)
        icon.width: body.page.cardGlyphSize
        icon.height: body.page.cardGlyphSize

        Accessible.name: text

        Hint {
            owner: dialogButton
            visible: dialogButton.hovered
            text: dialogButton.text
        }
    }
}
