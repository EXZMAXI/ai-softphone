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

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: qsTr("Buttons")
            font.pixelSize: 15
            font.bold: true
        }

        Button {
            visible: !buttons.editing
            text: qsTr("Add")
            onClicked: buttons.addButton()
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("A button dials its number with one press. A button that watches its line also shows a lamp: green when the line is free, amber while it rings, red during a call, and grey when the switch will not say.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        visible: buttons.buttons.length === 0 && !buttons.addingNew
        text: qsTr("No buttons yet. Add one, and a panel appears beside the dialler.")
        wrapMode: Text.WordWrap
        font.pixelSize: 13
        opacity: 0.7
    }

    ColumnLayout {
        id: keysColumn

        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: buttons.buttons

            delegate: ColumnLayout {
                id: keyEntry

                required property var modelData
                required property int index

                Layout.fillWidth: true
                spacing: 0

                ItemDelegate {
                    Layout.fillWidth: true
                    padding: 6
                    highlighted: buttons.selectedId === keyEntry.modelData.id
                                 && buttons.editing
                    onClicked: buttons.editing
                               && buttons.selectedId === keyEntry.modelData.id
                               ? buttons.cancel()
                               : buttons.select(keyEntry.modelData.id)

                    contentItem: RowLayout {
                        spacing: 8

                        Rectangle {
                            Layout.leftMargin: 6
                            Layout.alignment: Qt.AlignVCenter
                            visible: keyEntry.modelData.watched
                            implicitWidth: 10
                            implicitHeight: 10
                            radius: 5
                            color: page.lampColour(
                                       keyEntry.modelData.state)
                        }

                        Label {
                            Layout.leftMargin: keyEntry.modelData.watched ? 0 : 22
                            Layout.fillWidth: true
                            text: keyEntry.modelData.label
                            elide: Text.ElideRight
                        }

                        Label {
                            text: page.keyDetail(keyEntry.modelData)
                            font.pixelSize: 12
                            opacity: 0.6
                        }

                        Button {
                            flat: true
                            text: "▲"
                            Accessible.name: qsTr("Move up")
                            enabled: keyEntry.index > 0
                            leftPadding: 0
                            rightPadding: 0
                            topPadding: 0
                            bottomPadding: 0
                            topInset: 0
                            bottomInset: 0
                            implicitWidth: 30
                            implicitHeight: 30
                            onClicked: buttons.moveButton(
                                           keyEntry.modelData.id, -1)
                        }

                        Button {
                            flat: true
                            text: "▼"
                            Accessible.name: qsTr("Move down")
                            enabled: keyEntry.index < buttons.buttons.length - 1
                            leftPadding: 0
                            rightPadding: 0
                            topPadding: 0
                            bottomPadding: 0
                            topInset: 0
                            bottomInset: 0
                            implicitWidth: 30
                            implicitHeight: 30
                            onClicked: buttons.moveButton(
                                           keyEntry.modelData.id, 1)
                        }
                    }
                }

                CardLoader {
                    settingsPage: page
                    shows: keyEntry
                    active: buttons.editing && !buttons.addingNew
                            && buttons.selectedId === keyEntry.modelData.id
                    card: settingsPage.keyCard
                }
            }
        }
    }

    Button {
        Layout.fillWidth: true
        visible: !buttons.editing
                 && keysColumn.implicitHeight > page.scroller.height - 140
        text: qsTr("Add")
        onClicked: buttons.addButton()
    }

    CardLoader {
        settingsPage: page
        active: buttons.addingNew
        card: settingsPage.keyCard
    }

    Item { Layout.fillHeight: true }
}
