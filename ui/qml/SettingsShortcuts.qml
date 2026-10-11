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
import QtQuick.Controls.Material.impl
import QtQuick.Layouts

import Qt.labs.platform as Platform

import Softphone

ColumnLayout {

    required property var page
    Layout.fillWidth: true
    spacing: 12

    Label {
        Layout.fillWidth: true
        text: qsTr("Keyboard shortcuts")
        font.pixelSize: 15
        font.bold: true
    }

    Repeater {
        model: settings.hotkeys

        ColumnLayout {
            id: hotkeyRow

            required property var modelData

            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: page.actionLabel(hotkeyRow.modelData.action)
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Button {
                    id: capture

                    property bool capturing: false

                    property int refused: SettingsViewModel.KeyProblem.None

                    Layout.minimumWidth: 150
                    focusPolicy: Qt.StrongFocus

                    text: capturing
                        ? qsTr("Press a key…")
                        : (hotkeyRow.modelData.sequence.length > 0
                           ? settings.shownSequence(hotkeyRow.modelData.sequence)
                           : qsTr("Not set"))
                    highlighted: capturing

                    onCapturingChanged: settings.capturingKeys = capture.capturing

                    Component.onDestruction: if (capture.capturing) {
                        settings.capturingKeys = false
                    }

                    onClicked: {
                        capture.refused = SettingsViewModel.KeyProblem.None
                        if (capture.capturing) {
                            capture.capturing = false
                            settings.bindHotkey(hotkeyRow.modelData.action, "")
                            return
                        }
                        capture.capturing = true
                        forceActiveFocus()
                    }

                    onActiveFocusChanged: if (!activeFocus) capturing = false

                    Keys.onPressed: (event) => {
                        if (!capture.capturing) {
                            return
                        }

                        if (event.key === Qt.Key_Escape) {
                            capture.capturing = false
                            settings.bindHotkey(
                                hotkeyRow.modelData.action, "")
                            event.accepted = true
                            return
                        }

                        event.accepted = true

                        const chord = settings.sequenceFor(event.key,
                                                           event.modifiers)
                        if (chord.length === 0) {
                            return
                        }

                        const problem = settings.checkSequence(chord)
                        if (problem !== SettingsViewModel.KeyProblem.None) {
                            capture.refused = problem
                            return
                        }

                        capture.refused = SettingsViewModel.KeyProblem.None
                        settings.bindHotkey(hotkeyRow.modelData.action, chord)
                        capture.capturing = false
                    }
                }

                CheckBox {
                    text: qsTr("Everywhere")
                    checked: hotkeyRow.modelData.global

                    enabled: hotkeyRow.modelData.sequence.length > 0
                    onToggled: settings.setHotkeyGlobal(
                                   hotkeyRow.modelData.action, checked)
                }

                Item { Layout.fillWidth: true }
            }

            Label {
                Layout.fillWidth: true
                visible: capture.refused !== SettingsViewModel.KeyProblem.None
                text: page.keyProblemText(capture.refused)
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Ink.bad(Material.theme)
            }

            Label {
                Layout.fillWidth: true
                visible: hotkeyRow.modelData.clashing
                text: qsTr("Another shortcut is on this combination.")
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Ink.warn(Material.theme)
            }

            Label {
                Layout.fillWidth: true
                visible: hotkeyRow.modelData.global
                         && hotkeyRow.modelData.grab
                            !== SettingsViewModel.Grab.Working
                text: page.grabText(hotkeyRow.modelData.grab)
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Ink.warn(Material.theme)
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Click a shortcut and press the combination; clicking it again — or pressing Escape — clears it. “Everywhere” takes the combination from the rest of the desktop.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Button {
        Layout.fillWidth: true
        flat: true
        text: qsTr("Restore defaults")
        onClicked: settings.resetHotkeys()
    }

    Item { Layout.fillHeight: true }
}
