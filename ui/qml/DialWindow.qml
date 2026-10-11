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
import QtQuick.Window

import Softphone

ApplicationWindow {
    id: dialWindow

    required property var phone

    title: dialWindow.phone.title

    width: 340
    height: 132
    minimumWidth: 300
    minimumHeight: 132
    maximumHeight: 132

    flags: Qt.Dialog

    Material.theme: dialWindow.phone.darkWanted ? Material.Dark : Material.Light
    Material.accent: dialWindow.phone.Material.accent

    function ask() {
        if (!dialWindow.visible) {

            const on = dialWindow.phone && dialWindow.phone.screenForOurWindows
                       ? dialWindow.phone.screenForOurWindows() : dialWindow.screen
            if (on !== null && on !== undefined) {
                dialWindow.x = on.virtualX + Math.round((on.width - dialWindow.width) / 2)
                dialWindow.y = on.virtualY + Math.round((on.height - dialWindow.height) / 3)
            }
            dialWindow.show()
        }
        dialWindow.raise()
        dialWindow.requestActivate()
        field.forceActiveFocus()
    }

    onClosing: (event) => {
        if (!dialWindow.phone.quitting) {
            event.accepted = false
            dialWindow.hide()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            DialField {
                id: field

                implicitHeight: 40
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter

                cleaner: calls
                placeholderText: qsTr("Number")
                onDialEdited: dialled => calls.dialText = dialled
                onAccepted: dialWindow.placeIt()

                Binding {
                    target: field
                    property: "text"
                    value: calls.dialText
                    restoreMode: Binding.RestoreNone
                }
            }

            Button {
                id: dialKey

                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: field.implicitHeight
                Layout.preferredHeight: field.implicitHeight

                leftPadding: 0
                rightPadding: 0
                topPadding: 0
                bottomPadding: 0
                topInset: 0
                bottomInset: 0
                leftInset: 0
                rightInset: 0

                enabled: calls.canDial && accounts.connected

                Accessible.name: qsTr("Call")

                background: Rectangle {
                    radius: height / 2

                    color: dialKey.enabled
                           ? (dialKey.down ? Qt.darker(Mark.good, 1.35) : Mark.good)
                           : Qt.rgba(Material.foreground.r, Material.foreground.g,
                                     Material.foreground.b, 0.12)
                }

                contentItem: Item {
                    Glyph {
                        anchors.centerIn: parent

                        width: 2 * Math.round(dialKey.height * 0.29)
                        height: width
                        source: settings.glyphSet + "/call.svg"
                        color: dialKey.enabled
                               ? "white"
                               : Qt.rgba(Material.foreground.r, Material.foreground.g,
                                         Material.foreground.b, 0.4)
                    }
                }

                onClicked: dialWindow.placeIt()
            }
        }

        Label {
            Layout.fillWidth: true

            text: qsTr("The call is answered from the banner and controlled from the tray.")
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: 0.6
        }
    }

    function placeIt() {
        if (!calls.canDial) {
            return
        }
        calls.dial()
        dialWindow.hide()
    }
}
