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

import QtQuick.Dialogs as Dialogs
import Qt.labs.platform as Platform

import Softphone

Item {
    id: prompterBody

    objectName: "prompterBody"

    property string glyphs: ""
    property bool onScreen: false
    property bool kept: false
    property bool settling: false

    property bool focused: false

    implicitWidth: 520
    property int leastWidth: 360

    readonly property string drawing: "sparkle"

    readonly property string windowTitle: qsTr("Prompter")

    property int bandHeight: 0

    readonly property int bandRoom: {
        const least = settings.prompterRepeatText + 28
        const most = Math.max(least, Math.round(prompterBody.height * 0.6))
        const wanted = prompterBody.bandHeight > 0
                     ? prompterBody.bandHeight
                     : Math.round(prompterBody.height * 0.125)
        return Math.min(most, Math.max(least, wanted))
    }

    property string pinned: ""

    readonly property string repeating: {
        if (prompterBody.pinned.length > 0) {
            return prompterBody.pinned
        }
        const rows = prompter.suggesting ? prompter.answers : prompter.said
        return rows.length > 0 ? (rows[rows.length - 1].text ?? "") : ""
    }

    function hold(text, letGo) {
        prompterBody.pinned = letGo ? "" : text
    }

    readonly property string sectionNow: ""
    function showSection(which) {}

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label {
            Layout.fillWidth: true
            visible: text.length > 0
            text: prompter.running ? prompter.heading
                                   : (prompter.waiting ? prompter.waitingLine
                                                       : prompter.unavailable)
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: 0.6
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 0

            LabelledPicker {
                id: templatePicker

                readonly property var choices: prompter.templates

                Layout.fillWidth: true
                Layout.rightMargin: 8

                enabled: prompter.configured && !prompter.running

                placeholderText: qsTr("Assistant")
                model: choices.map((one) => one.id)
                displayText: {
                    const found = choices.find((one) => one.id === model[currentIndex]);
                    return found ? found.label : "";
                }

                delegate: ItemDelegate {
                    required property var modelData
                    required property int index

                    width: templatePicker.width
                    text: {
                        const found = templatePicker.choices.find((one) => one.id === modelData);
                        return found ? found.label : modelData;
                    }
                    highlighted: templatePicker.highlightedIndex === index
                }

                currentIndex: choices.findIndex((one) => one.id === prompter.chosen)
                onActivated: (which) => prompter.choose(templatePicker.choices[which].id)
            }

            PanelAct {
                glyphs: prompterBody.glyphs

                objectName: "prompterStart"

                readonly property bool begun: prompter.begun

                picture: begun ? "record-stop" : "resume"
                enabled: begun || (prompter.available && prompter.chosen.length > 0)

                text: begun ? qsTr("Stop") : qsTr("Start")

                hint: prompter.waiting

                      ? qsTr("Do not start after all")
                      : prompter.running

                        ? qsTr("Stop listening. What was said stays on the screen.")

                        : qsTr("Start listening to this call")
                onClicked: begun ? prompter.stop() : prompter.start()
            }

            PanelAct {
                glyphs: prompterBody.glyphs

                picture: prompter.suggesting ? "sparkle" : "split"

                enabled: prompter.running
                text: prompter.suggesting

                      ? qsTr("Prompt me")

                      : qsTr("End the reply")

                hint: prompter.suggesting

                      ? qsTr("End the reply here and suggest what to say")

                      : qsTr("End the reply here, so the next one starts clean")
                onClicked: prompter.askNow()
            }

            PanelAct {
                glyphs: prompterBody.glyphs

                picture: "delete"
                enabled: prompter.anything

                text: qsTr("Clear")

                hint: qsTr("Forget what is on the screen")

                onClicked: clearing.open()
            }

            PanelAct {
                glyphs: prompterBody.glyphs

                picture: "save"
                enabled: prompter.anything

                text: qsTr("Export…")
                hint: qsTr("Write both columns to a file, with their times")
                onClicked: prompterBody.offerToSave()
            }

            PanelAct {
                id: rehearseAct

                glyphs: prompterBody.glyphs

                picture: "library"
                shown: !prompter.rehearsing
                enabled: !prompter.begun

                text: qsTr("Rehearse…")
                hint: qsTr("Try an assistant out on a recording instead of a call")
                onClicked: prompterBody.offerToRehearse(rehearseAct)
            }
        }

        Label {
            Layout.fillWidth: true
            visible: prompter.trouble.length > 0
            text: prompter.trouble
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            color: Ink.warn(Material.theme)
        }

        Label {
            Layout.fillWidth: true
            visible: prompter.troubleSaid.length > 0
            text: prompter.troubleSaid
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: 0.6
        }

        RehearsalBar {
            Layout.fillWidth: true

            visible: prompter.rehearsing
            glyphs: prompterBody.glyphs
        }

        Label {
            Layout.fillWidth: true

            visible: prompter.rehearsing && prompter.rehearsalNote.length > 0
            text: prompter.rehearsalNote
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: 0.6
        }

        Frame {
            id: band

            Layout.fillWidth: true
            Layout.preferredHeight: prompterBody.bandRoom
            Layout.minimumHeight: 0

            visible: settings.prompterRepeatsTheLast
            padding: 8

            Item {
                anchors.fill: parent

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    Flickable {
                        id: bandView

                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        clip: true
                        contentWidth: width
                        contentHeight: repeated.implicitHeight
                        boundsBehavior: Flickable.StopAtBounds

                        ScrollBar.vertical: ThinScrollBar {
                            always: true
                        }

                        Label {
                            id: repeated

                            width: bandView.width - 12
                            y: Math.max(0, (bandView.height - repeated.implicitHeight) / 2)
                            text: prompterBody.repeating
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                            font.pixelSize: settings.prompterRepeatText

                            onTextChanged: if (prompterBody.pinned.length === 0) {
                                bandView.contentY = 0
                            }

                            RowMenu {
                                extra: [{

                                    label: qsTr("Copy this"),
                                    picture: settings.glyphSet + "/copy.svg",
                                    act: () => settings.copyToClipboard(
                                             prompterBody.repeating)
                                }]
                            }
                        }
                    }

                    PanelAct {
                        glyphs: prompterBody.glyphs

                        picture: "pin"
                        shown: prompterBody.pinned.length > 0

                        text: qsTr("Follow the newest")
                        hint: qsTr("Stop holding this one")
                        onClicked: prompterBody.pinned = ""
                    }
                }

                Label {
                    anchors.centerIn: parent
                    width: parent.width - 24
                    visible: prompterBody.repeating.length === 0

                    text: qsTr("The newest line appears here")
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    font.pixelSize: 12
                    opacity: 0.6
                }
            }
        }

        SplitGrip {
            Layout.fillWidth: true

            visible: band.visible

            sizeNow: band.height
            hairline: Qt.rgba(Material.foreground.r, Material.foreground.g,
                              Material.foreground.b, 0.2)

            onMoved: (size) => { prompterBody.bandHeight = size }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            columns: prompterBody.width >= 640 && prompter.suggesting ? 2 : 1
            columnSpacing: 12
            rowSpacing: 8

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                spacing: 4

                Label {
                    Layout.fillWidth: true

                    text: qsTr("Transcript")
                    font.pixelSize: 12
                    font.bold: true
                    opacity: 0.6
                }

                Tail {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: prompter.said
                    textPixels: settings.prompterColumnText
                    pickable: settings.prompterRepeatsTheLast

                    empty: qsTr("Nothing yet")

                    onPicked: (text, theLastOne) =>
                        prompterBody.hold(text, theLastOne && !prompter.suggesting)
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                spacing: 4

                visible: prompter.suggesting

                Label {
                    Layout.fillWidth: true

                    text: qsTr("Suggestions")
                    font.pixelSize: 12
                    font.bold: true
                    opacity: 0.6
                }

                Tail {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: prompter.answers
                    answersColumn: true
                    textPixels: settings.prompterColumnText
                    pickable: settings.prompterRepeatsTheLast

                    empty: qsTr("No suggestions yet")

                    onPicked: (text, theLastOne) =>
                        prompterBody.hold(text, theLastOne && prompter.suggesting)
                }
            }
        }
    }

    ConfirmDialog {
        id: clearing

        title: qsTr("Clear the prompter?")
        actionText: qsTr("Clear")

        onAccepted: {
            prompter.clearEverything()
            prompterBody.pinned = ""
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Both columns go, and so does the conversation the next suggestion would have been built from. What has already been filed against a recording is kept. Clearing cannot be undone.")
        }
    }

    function offerToSave() {
        if (exporting.currentFolder.toString().length === 0) {
            exporting.currentFolder =
                Platform.StandardPaths.writableLocation(Platform.StandardPaths.DocumentsLocation)
        }
        exporting.selectedFile = exporting.currentFolder + "/" + prompter.exportName()
        exporting.open()
    }

    RehearsalChooser {
        id: rehearsals

        host: rehearseAct
        glyphs: prompterBody.glyphs

        rows: prompter.rehearsalChoices

        empty: qsTr("No conversations in the library yet")

        fileEntry: qsTr("A file on this computer…")

        onChose: (id) => prompter.rehearse(id)
        onWantsAFile: prompterBody.offerToOpen()
    }

    function askForTheLibrary() {
        prompter.listRehearsals()
    }

    onOnScreenChanged: if (prompterBody.onScreen) prompterBody.askForTheLibrary()

    Component.onCompleted: prompterBody.askForTheLibrary()

    function offerToRehearse(under) {

        prompterBody.askForTheLibrary()
        rehearsals.present(under)
    }

    function offerToOpen() {
        if (opening.currentFolder.toString().length === 0) {
            opening.currentFolder =
                Platform.StandardPaths.writableLocation(Platform.StandardPaths.MusicLocation)
        }
        opening.open()
    }

    Dialogs.FileDialog {
        id: opening

        fileMode: Dialogs.FileDialog.OpenFile

        title: qsTr("Rehearse against a file")

        nameFilters: [qsTr("Sound files (*.mp3 *.wav)")]
        onAccepted: prompter.rehearseFile(selectedFile)
    }

    Dialogs.FileDialog {
        id: exporting

        fileMode: Dialogs.FileDialog.SaveFile
        defaultSuffix: "txt"

        title: qsTr("Save the prompter")

        nameFilters: [qsTr("Text files (*.txt)"), qsTr("Spreadsheets (*.csv)")]
        onAccepted: prompter.save(selectedFile)
    }
}
