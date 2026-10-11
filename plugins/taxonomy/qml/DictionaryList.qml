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

import Softphone

ColumnLayout {

    required property var settingsPage

    required property string kind
    required property string heading
    required property var rows
    required property string hint

    id: dictionary

    spacing: 12

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 12
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: dictionary.heading
            font.pixelSize: 15
            font.bold: true
        }

        Button {
            visible: !taxonomy.editing
            text: qsTr("Add")
            onClicked: taxonomy.add(dictionary.kind)
        }
    }

    Label {
        Layout.fillWidth: true
        text: dictionary.hint
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    ColumnLayout {
        id: dictionaryColumn

        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: dictionary.rows

            delegate: ColumnLayout {
                id: dictionaryEntry

                required property var modelData

                Layout.fillWidth: true
                spacing: 0

                ItemDelegate {
                    readonly property var modelData: dictionaryEntry.modelData

                    Layout.fillWidth: true
                    padding: 6
                    highlighted: taxonomy.editingId === modelData.id

                    onClicked: taxonomy.editingId === modelData.id
                               ? taxonomy.cancel()
                               : taxonomy.edit(modelData.id)

                    contentItem: RowLayout {
                        spacing: 8

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                Layout.fillWidth: true
                                text: modelData.title
                                elide: Text.ElideRight
                                opacity: modelData.retired ? 0.5 : 1.0
                            }

                            Label {
                                Layout.fillWidth: true
                                text: modelData.description.length > 0
                                      ? modelData.code + " · " + modelData.description
                                      : modelData.code
                                wrapMode: Text.WordWrap
                                maximumLineCount: 3
                                elide: Text.ElideRight
                                font.pixelSize: 12
                                opacity: 0.6
                            }
                        }

                        Label {
                            visible: modelData.severity === "high"

                            text: qsTr("high")
                            font.pixelSize: 12
                            color: Ink.bad(Material.theme)
                            opacity: 0.8
                        }

                        Label {
                            visible: modelData.retired

                            text: qsTr("deleted")
                            font.pixelSize: 12
                            opacity: 0.6
                        }
                    }
                }

                CardLoader {
                    settingsPage: dictionary.settingsPage
                    shows: dictionaryEntry
                    active: taxonomy.editing
                            && taxonomy.editingId === dictionaryEntry.modelData.id
                    card: settingsPage.taxonomyCard
                }
            }
        }
    }

    Button {
        Layout.fillWidth: true
        visible: !taxonomy.editing
                 && dictionaryColumn.implicitHeight > dictionary.settingsPage.scroller.height - 140
        text: qsTr("Add")
        onClicked: taxonomy.add(dictionary.kind)
    }

    CardLoader {
        settingsPage: dictionary.settingsPage
        active: taxonomy.editing && taxonomy.editingId.length === 0
                && taxonomy.editingKind === dictionary.kind
        card: settingsPage.taxonomyCard
    }
}
