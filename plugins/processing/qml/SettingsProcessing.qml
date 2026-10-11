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
        text: qsTr("Processing")
        font.pixelSize: 15
        font.bold: true
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        ListCheckBox {
            Layout.fillWidth: true

            text: qsTr("Process conversations automatically")
            checked: settings.processAutomatically
            onToggled: settings.setProcessAutomatically(checked)
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Off, nothing happens until you ask for it in the recordings window. On, the rules below run — which is what turns a conversation into a summary, a category and everything else without anybody pressing anything. A model in the cloud charges for each of those.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: qsTr("Language models")
            font.pixelSize: 15
            font.bold: true
        }

        Button {

            objectName: "theWayIn"
            visible: !models.editing
            text: qsTr("Add")
            onClicked: models.add()
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("What reads a transcript and writes about it. A model on your own machine keeps every conversation in the building and costs nothing to run.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    ColumnLayout {
        id: modelsColumn

        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: models.rows

            delegate: ColumnLayout {
                id: modelEntry

                required property var modelData

                Layout.fillWidth: true
                spacing: 0

                ItemDelegate {
                    readonly property var modelData: modelEntry.modelData

                    Layout.fillWidth: true
                    padding: 6
                    highlighted: models.editingId === modelData.id
                    onClicked: models.editingId === modelData.id
                               ? models.cancel()
                               : models.edit(modelData.id)

                    contentItem: RowLayout {
                        spacing: 8

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                Layout.fillWidth: true
                                text: modelData.name
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: modelData.model.length > 0
                                      ? modelData.model + " · " + modelData.endpoint
                                      : modelData.endpoint
                                elide: Text.ElideRight
                                font.pixelSize: 12
                                opacity: 0.6
                            }
                        }

                        Label {
                            visible: modelData.isDefault

                            text: qsTr("default")
                            font.pixelSize: 12
                            opacity: 0.6
                        }

                        Label {
                            visible: !modelData.enabled

                            text: qsTr("off", "a language model")
                            font.pixelSize: 12
                            opacity: 0.6
                        }
                    }
                }

                CardLoader {
                    settingsPage: page
                    shows: modelEntry
                    about: models
                    active: models.editing
                            && models.editingId === modelEntry.modelData.id
                    card: settingsPage.providerCard
                }
            }
        }
    }

    Button {
        Layout.fillWidth: true
        visible: !models.editing
                 && modelsColumn.implicitHeight > page.scroller.height - 140
        text: qsTr("Add")
        onClicked: models.add()
    }

    CardLoader {
        settingsPage: page
        about: models
        active: models.editing && models.editingId.length === 0
        card: settingsPage.providerCard
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 12
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: qsTr("Prompts")
            font.pixelSize: 15
            font.bold: true
        }

        Button {
            visible: !templates.editing
            text: qsTr("Add")
            onClicked: templates.add()
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("What the models are asked. Every one of these came with the program and every one of them is yours to change — and to put back.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    ColumnLayout {
        id: templatesColumn

        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: templates.rows

            delegate: ColumnLayout {
                id: templateEntry

                required property var modelData
                required property int index

                Layout.fillWidth: true
                spacing: 0

                ItemDelegate {
                    readonly property var modelData: templateEntry.modelData

                    Layout.fillWidth: true
                    padding: 6
                    highlighted: templates.editingId === modelData.id
                    onClicked: templates.editingId === modelData.id
                               ? templates.cancel()
                               : templates.edit(modelData.id)

                    contentItem: RowLayout {
                        spacing: 8

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                Layout.fillWidth: true
                                text: modelData.name
                                elide: Text.ElideRight
                                opacity: modelData.enabled ? 1.0 : 0.5
                            }

                            Label {
                                Layout.fillWidth: true
                                text: modelData.kindLabel + " · "
                                      + modelData.shapeLabel
                                elide: Text.ElideRight
                                font.pixelSize: 12
                                opacity: 0.6
                            }
                        }

                        Label {
                            visible: modelData.edited

                            text: qsTr("changed")
                            font.pixelSize: 12
                            opacity: 0.6
                        }

                        Label {
                            visible: !modelData.enabled

                            text: qsTr("off", "a prompt")
                            font.pixelSize: 12
                            opacity: 0.6
                        }

                        Button {
                            flat: true
                            text: "▲"
                            Accessible.name: qsTr("Move up")
                            enabled: templateEntry.index > 0

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
                            onClicked: templates.move(templateEntry.modelData.id, -1)
                        }

                        Button {
                            flat: true
                            text: "▼"
                            Accessible.name: qsTr("Move down")
                            enabled: templateEntry.index < templates.rows.length - 1

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
                            onClicked: templates.move(templateEntry.modelData.id, 1)
                        }
                    }
                }

                CardLoader {
                    settingsPage: page
                    shows: templateEntry
                    active: templates.editing
                            && templates.editingId === templateEntry.modelData.id
                    card: settingsPage.templateCard
                }
            }
        }
    }

    Button {
        Layout.fillWidth: true
        visible: !templates.editing
                 && templatesColumn.implicitHeight > page.scroller.height - 140
        text: qsTr("Add")
        onClicked: templates.add()
    }

    CardLoader {
        settingsPage: page
        active: templates.editing && templates.editingId.length === 0
        card: settingsPage.templateCard
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 12
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: qsTr("Rules")
            font.pixelSize: 15
            font.bold: true
        }

        Button {
            visible: !automations.editing
            text: qsTr("Add")
            onClicked: automations.add()
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("What runs by itself, in this order. Each one fires at most once per conversation.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    ColumnLayout {
        id: automationsColumn

        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: automations.rows

            delegate: ColumnLayout {
                id: automationEntry

                required property var modelData
                required property int index

                Layout.fillWidth: true
                spacing: 0

                ItemDelegate {
                    readonly property var modelData: automationEntry.modelData

                    Layout.fillWidth: true
                    padding: 6
                    highlighted: automations.editingId === modelData.id
                    onClicked: automations.editingId === modelData.id
                               ? automations.cancel()
                               : automations.edit(modelData.id)

                    contentItem: RowLayout {
                        spacing: 8

                        ListCheckBox {
                            implicitWidth: 30
                            checked: modelData.enabled
                            onToggled: automations.toggle(modelData.id,
                                                          checked)
                            Accessible.name: modelData.name
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                Layout.fillWidth: true
                                text: modelData.name
                                elide: Text.ElideRight
                                opacity: modelData.enabled ? 1.0 : 0.5
                            }

                            Label {
                                Layout.fillWidth: true
                                text: modelData.sentence
                                wrapMode: Text.WordWrap
                                font.pixelSize: 12
                                opacity: 0.6
                            }
                        }

                        ToolButton {
                            text: "▲"
                            enabled: automationEntry.index > 0
                            onClicked: automations.moveBy(modelData.id, -1)
                            Accessible.name: qsTr("Move up")
                        }

                        ToolButton {
                            text: "▼"
                            enabled: automationEntry.index
                                     < automations.count - 1
                            onClicked: automations.moveBy(modelData.id, 1)
                            Accessible.name: qsTr("Move down")
                        }
                    }
                }

                CardLoader {
                    settingsPage: page
                    shows: automationEntry
                    active: automations.editing
                            && automations.editingId
                               === automationEntry.modelData.id
                    card: settingsPage.automationCard
                }
            }
        }
    }

    Button {
        Layout.fillWidth: true
        visible: !automations.editing
                 && automationsColumn.implicitHeight
                    > page.scroller.height - 140
        text: qsTr("Add")
        onClicked: automations.add()
    }

    CardLoader {
        settingsPage: page
        active: automations.editing && automations.editingId.length === 0
        card: settingsPage.automationCard
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 12

        text: qsTr("Spending")
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        visible: settings.monthSoFar.length > 0
        text: settings.monthSoFar
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        color: settings.processingPaused
               ? Ink.bad(Material.theme)
               : Material.foreground
        opacity: settings.processingPaused ? 1.0 : 0.6
    }

    TextField {

        TextMenu {}

        Layout.fillWidth: true
        placeholderText: qsTr("Money limit, monthly")
        text: settings.monthlyCeiling
        onTextEdited: settings.setMonthlyCeiling(text)
    }

    TextField {

        TextMenu {}

        Layout.fillWidth: true
        placeholderText: qsTr("Token limit, monthly")
        text: settings.monthlyTokenCeiling
        onTextEdited: settings.setMonthlyTokenCeiling(text)
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Two limits, in the two things a month can be counted in. When either is reached the rules stop until the month turns. Asking for something yourself is never stopped.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 12
        text: qsTr("Defaults")
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Puts the prompts and the rules back as they came with the program. Your language models are left alone.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Button {
        Layout.fillWidth: true
        topInset: 0
        bottomInset: 0
        visible: !templates.editing && !automations.editing
        text: qsTr("Restore defaults")
        onClicked: page.restoreEverything.open()
    }

    Item { Layout.fillHeight: true }
}
