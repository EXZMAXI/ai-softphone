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
            text: qsTr("Accounts")
            font.pixelSize: 15
            font.bold: true
        }

        Button {

            objectName: "theWayIn"
            visible: !accounts.editing
            text: qsTr("Add")
            onClicked: accounts.addAccount()
        }
    }

    Label {
        Layout.fillWidth: true
        visible: accounts.count === 0
        text: qsTr("No accounts yet. Add the one your provider or administrator gave you.")
        wrapMode: Text.WordWrap
        opacity: 0.6
    }

    ColumnLayout {
        id: accountsColumn

        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: accounts.accounts

            delegate: ColumnLayout {
                id: accountEntry

                required property var modelData
                required property int index

                Layout.fillWidth: true
                spacing: 0

                ItemDelegate {
                    id: accountRow

                    readonly property var modelData: accountEntry.modelData
                    readonly property int index: accountEntry.index

                    Layout.fillWidth: true
                padding: 6
                highlighted: accounts.selectedId === accountRow.modelData.id

                onClicked: accounts.editing
                           && accounts.selectedId === accountRow.modelData.id
                           ? accounts.cancelEdit()
                           : accounts.select(accountRow.modelData.id)

                contentItem: RowLayout {
                    spacing: 8

                    CheckBox {
                        padding: 0
                        implicitWidth: implicitIndicatorWidth

                        Layout.leftMargin: 6
                        checked: accountRow.modelData.enabled
                        Accessible.name: qsTr("Use this account")
                        onToggled: accounts.setAccountEnabled(
                                       accountRow.modelData.id, checked)
                    }

                    Rectangle {
                        implicitWidth: 10
                        implicitHeight: 10
                        radius: 5
                        Layout.alignment: Qt.AlignVCenter
                        color: page.stateColor(accountRow.modelData)
                    }

                    ColumnLayout {
                        Layout.fillWidth: true

                        Layout.minimumWidth: 0
                        spacing: 0

                        Label {
                            Layout.fillWidth: true
                            elide: Text.ElideMiddle
                            text: accountRow.modelData.label
                        }

                        Label {
                            id: noteLabel

                            Layout.fillWidth: true
                            elide: Text.ElideMiddle
                            visible: text.length > 0
                            font.pixelSize: 11
                            opacity: 0.6
                            text: page.rowNote(accountRow.modelData)

                            HoverHandler {
                                id: rowNoteHover
                            }

                            Hint {
                                owner: noteLabel
                                text: noteLabel.text
                                visible: rowNoteHover.hovered
                                         && noteLabel.truncated
                            }
                        }
                    }

                    Button {
                        flat: true

                        opacity: accountRow.modelData.enabled ? 1.0 : 0.0
                        enabled: accountRow.modelData.enabled

                        focusPolicy: Qt.NoFocus
                        Accessible.ignored: !accountRow.modelData.enabled
                        text: page.stateIs(
                                  accountRow.modelData,
                                  AccountsViewModel.Registered)
                              ? qsTr("Disconnect") : qsTr("Connect")
                        onClicked: page.stateIs(
                                       accountRow.modelData,
                                       AccountsViewModel.Registered)
                                   ? accounts.disconnectAccount(
                                         accountRow.modelData.id)
                                   : accounts.connectAccount(
                                         accountRow.modelData.id)
                    }

                    Button {
                        flat: true
                        text: "▲"
                        Accessible.name: qsTr("Move up")
                        enabled: accountRow.index > 0

                        leftPadding: 0
                        rightPadding: 0
                        topPadding: 0
                        bottomPadding: 0
                        topInset: 0
                        bottomInset: 0

                        Layout.alignment: Qt.AlignVCenter
                        Layout.minimumWidth: 24
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        onClicked: accounts.moveAccount(
                                       accountRow.modelData.id, -1)
                    }

                    Button {
                        flat: true
                        text: "▼"
                        Accessible.name: qsTr("Move down")
                        enabled: accountRow.index < accounts.count - 1

                        leftPadding: 0
                        rightPadding: 0
                        topPadding: 0
                        bottomPadding: 0
                        topInset: 0
                        bottomInset: 0

                        Layout.alignment: Qt.AlignVCenter
                        Layout.minimumWidth: 24
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        onClicked: accounts.moveAccount(
                                       accountRow.modelData.id, 1)
                    }
                }
                }

                CardLoader {
                    settingsPage: page
                    shows: accountEntry
                    active: accounts.editing && !accounts.addingNew
                            && accounts.selectedId === accountEntry.modelData.id
                    card: settingsPage.accountCard
                }
            }
        }
    }

    Button {
        Layout.fillWidth: true
        visible: !accounts.editing
                 && accountsColumn.implicitHeight > page.scroller.height - 140
        text: qsTr("Add")
        onClicked: accounts.addAccount()
    }

    CardLoader {
        settingsPage: page
        active: accounts.addingNew
        card: settingsPage.accountCard
    }

    Label {
        Layout.fillWidth: true
        visible: text.length > 0
        text: page.registrationSummary
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        color: Ink.bad(Material.theme)
    }

    Item { Layout.fillHeight: true }
}
