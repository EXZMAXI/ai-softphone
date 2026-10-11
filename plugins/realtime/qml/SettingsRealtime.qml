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
import QtQuick.Layouts

import Softphone

ColumnLayout {

    required property var page

    Layout.fillWidth: true
    spacing: 12

    Label {
        Layout.fillWidth: true

        text: qsTr("Prompter")
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Speech recognition while a conversation is happening, and suggestions written to your own instructions. Both are charged by the minute.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        ListCheckBox {

            objectName: "theWayIn"

            Layout.fillWidth: true

            text: qsTr("Allow the prompter to be used")
            checked: settings.prompterOffered
            onToggled: settings.setPrompterOffered(checked)
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("The other party's voice is sent to a recogniser as it is spoken, which is no less than recording them. Where Settings → Recording asks for them to be told first, a prompter starts only after they have been.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 12

        text: qsTr("Text size")
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("The prompter is read while you are talking, often from further away than the rest of the phone. Choose a size you can take in without leaning towards the screen.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true

        text: qsTr("Transcript and suggestions: %n pixel(s)", "", settings.prompterColumnText)
        wrapMode: Text.WordWrap
    }

    Slider {
        Layout.fillWidth: true
        from: settings.leastPrompterColumnText
        to: settings.mostPrompterColumnText
        stepSize: 1
        snapMode: Slider.SnapAlways
        value: settings.prompterColumnText

        onPressedChanged: {
            if (!pressed) {
                settings.prompterColumnText = Math.round(value)
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        ListCheckBox {
            Layout.fillWidth: true

            text: qsTr("Repeat the newest line above the columns")
            checked: settings.prompterRepeatsTheLast
            onToggled: settings.prompterRepeatsTheLast = checked
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("The newest line stays in sight above the columns, large enough to read at a glance. Drag the divider under it to make it taller. Press any line to keep it there; press the newest one to follow again.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        visible: settings.prompterRepeatsTheLast

        text: qsTr("The repeated line: %n pixel(s)", "", settings.prompterRepeatText)
        wrapMode: Text.WordWrap
    }

    Slider {
        Layout.fillWidth: true
        visible: settings.prompterRepeatsTheLast
        from: settings.leastPrompterRepeatText
        to: settings.mostPrompterRepeatText
        stepSize: 1
        snapMode: Slider.SnapAlways
        value: settings.prompterRepeatText

        onPressedChanged: {
            if (!pressed) {
                settings.prompterRepeatText = Math.round(value)
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 12
        spacing: 8

        Label {
            Layout.fillWidth: true

            text: qsTr("Assistants")
            font.pixelSize: 15
            font.bold: true
        }

        Button {
            visible: !templates.editing
            text: qsTr("Add")
            onClicked: templates.addRealtime()
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Each one listens to a conversation as it happens and writes something into the prompter's window: the words as they are said, a translation of them, or a suggestion for what to say next. You choose which to run when you start a prompter.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    ColumnLayout {
        id: prompterTemplatesColumn

        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: templates.realtimeRows

            delegate: ColumnLayout {
                id: prompterTemplateEntry

                required property var modelData
                required property int index

                Layout.fillWidth: true
                spacing: 0

                ItemDelegate {
                    readonly property var modelData: prompterTemplateEntry.modelData

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
                            enabled: prompterTemplateEntry.index > 0

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
                            onClicked: templates.move(prompterTemplateEntry.modelData.id, -1)
                        }

                        Button {
                            flat: true
                            text: "▼"
                            Accessible.name: qsTr("Move down")
                            enabled: prompterTemplateEntry.index
                                     < templates.realtimeRows.length - 1

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
                            onClicked: templates.move(prompterTemplateEntry.modelData.id, 1)
                        }
                    }
                }

                CardLoader {
                    settingsPage: page
                    shows: prompterTemplateEntry
                    active: templates.editing
                            && templates.editingId === prompterTemplateEntry.modelData.id
                    card: settingsPage.templateCard
                }
            }
        }
    }

    Button {
        Layout.fillWidth: true
        visible: !templates.editing
                 && prompterTemplatesColumn.implicitHeight > page.scroller.height - 140
        text: qsTr("Add")
        onClicked: templates.addRealtime()
    }

    CardLoader {
        settingsPage: page
        active: templates.editing && templates.editingId.length === 0
        card: settingsPage.templateCard
    }

    Button {
        Layout.fillWidth: true
        Layout.topMargin: 12
        topInset: 0
        bottomInset: 0
        visible: !templates.editing
        text: qsTr("Restore defaults")
        onClicked: page.restoreEverything.open()
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Puts the prompts and the rules back as they came with the program. Your language models are left alone.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
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
        text: qsTr("Separate from what the rules may spend on finished conversations. A month of summaries must not be able to silence a prompter in the middle of a conversation.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    TextField {
        TextMenu {}

        Layout.fillWidth: true

        placeholderText: qsTr("Recognisers, a month")
        text: settings.realtimeRecogniserCeiling
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        onEditingFinished: settings.setRealtimeRecogniserCeiling(text)
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Reached, a running prompter stops at the end of the reply it is on — never mid-word. Empty is no ceiling.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    TextField {
        TextMenu {}

        Layout.fillWidth: true

        placeholderText: qsTr("Models, a month")
        text: settings.realtimeModelCeiling
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        onEditingFinished: settings.setRealtimeModelCeiling(text)
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Reached, the prompting stops and the subtitles carry on — which is worth having on its own. Empty is no ceiling.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 4
        text: qsTr("Recognisers are added under Transcription and models under Processing. A prompter is offered once a recogniser and a model have been chosen and tested — neither works until it has an address or a key.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }
}
