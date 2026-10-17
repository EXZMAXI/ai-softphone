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

import Qt.labs.platform as Platform
import QtQuick.Dialogs as Dialogs

import Softphone

Item {
    id: readingPanel

    implicitHeight: nothingYet.visible
                    ? nothingYet.implicitHeight
                    : (cameBackEmpty.visible ? cameBackEmpty.implicitHeight : 0)

    property string glyphs: ""

    property bool chosen: false

    readonly property var entries: transcript.choices.map((one) => ({id: one.id,
                                                                     label: one.label,
                                                                     glyph: one.primary
                                                                            ? "star" : "mic",
                                                                     working: false,
                                                                     failed: one.failed}))

    readonly property string openId: transcript.openId

    readonly property bool working: transcript.working

    property real playhead: 0

    onPlayheadChanged: transcript.setPosition(Math.round(readingPanel.playhead))

    property bool following: true

    readonly property string provenance: readingPanel.chosen ? readingPanel.provenanceText() : ""

    readonly property var notices: [{text: transcript.failure, kind: "failure"},
                                    {text: transcript.notice, kind: "notice"}]

    function open(id) {
        if (id.length > 0) {
            transcript.choose(id)
        }
    }

    property Component whenEmpty: Component {
        Button {
            id: firstAsk

            visible: !transcript.available
            text: qsTr("Transcribe")
            enabled: !transcript.working && recordings.openHasAudio
                     && !recordings.openUnfinished
            highlighted: true
            topInset: 0
            bottomInset: 0
            onClicked: readingPanel.askForAReading(firstAsk)

            Hint {
                owner: parent
                visible: parent.hovered
                text: transcript.working
                      ? qsTr("This conversation is being transcribed now")
                      : (recordings.openUnfinished
                         ? qsTr("This conversation is still being recorded")
                         : (recordings.openHasAudio
                            ? qsTr("Send this conversation to the recogniser")
                            : qsTr("The sound of this conversation is no longer here")))
            }
        }
    }

    property int jobRank: 10

    readonly property string jobsTitle: qsTr("Transcription")

    function jobsNow() {
        void Qt.uiLanguage

        const out = []
        if (!recordings.openHasAudio || recordings.openUnfinished) {
            return out
        }
        for (const one of providers.usable) {
            out.push({
                         label: one.isDefault

                                ? qsTr("%1 (default)").arg(one.name)
                                : one.name,
                         offered: !transcript.working,
                         picture: "",
                         act: () => transcript.transcribeWith(one.id)
                     })
        }
        return out
    }

    readonly property string jobsNote: {
        void Qt.uiLanguage

        if (recordings.openUnfinished) {
            return qsTr("This conversation is still being recorded")
        }
        if (!recordings.openHasAudio) {
            return qsTr("The sound of this conversation is no longer here")
        }
        if (providers.usable.length === 0) {
            return qsTr("No recogniser is set up yet. Add one in Settings, under Transcription.")
        }
        return ""
    }

    property int actRank: 20

    property Component acts: Component {
        Row {
            spacing: 0

            PanelAct {
                glyphs: readingPanel.glyphs

                picture: "star"
                shown: readingPanel.chosen && transcript.done && !transcript.primary

                text: qsTr("Use this transcription")
                hint: qsTr("Make this the transcription that searching and the summaries use")
                onClicked: transcript.makePrimary()
            }

            PanelAct {
                glyphs: readingPanel.glyphs

                picture: "copy"
                shown: readingPanel.chosen && transcript.done && transcript.count > 0

                text: qsTr("Copy")
                onClicked: settings.copyToClipboard(transcript.openAsText())
            }

            PanelAct {
                id: saveAct

                glyphs: readingPanel.glyphs

                picture: "save"
                shown: readingPanel.chosen && transcript.done
                text: qsTr("Save as…")
                onClicked: transcriptFormats.popup(saveAct, 0, saveAct.height)
            }

            PanelAct {
                glyphs: readingPanel.glyphs

                picture: "delete"
                shown: readingPanel.chosen && transcript.available

                text: qsTr("Delete this transcription")
                onClicked: removeReadingDialog.open()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            id: nothingYet

            Layout.fillWidth: true
            visible: !transcript.available
                     && transcript.failure.length === 0
            text: transcript.working
                  ? qsTr("Listening to it…")
                  : qsTr("This conversation has not been transcribed. Set up a recogniser in the settings, then press Transcribe.")
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: 0.6
        }

        Label {
            id: cameBackEmpty

            Layout.fillWidth: true
            visible: transcript.available
                     && transcript.done && transcript.count === 0
            text: qsTr("The recogniser heard nothing in this conversation. That is usually silence, a call too short to hear, or the wrong language — try another recogniser from the list above.")
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: 0.6
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: true
                     && !(transcript.available && transcript.count > 0)
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: transcript.available
                     && transcript.count > 0

            ListView {
                id: turns

                anchors.fill: parent
                clip: true
                spacing: 2
                model: transcript.lines

                ScrollBar.vertical: ThinScrollBar {

                    always: true

                    onPressedChanged: if (pressed) {
                        readingPanel.following = false
                    }
                }

                onMovementStarted: readingPanel.following = false

                delegate: ItemDelegate {
                    id: turn

                    required property var modelData
                    required property int index

                    width: turns.width
                    highlighted: index === transcript.currentLine

                    RowMenu {
                        extra: [{

                            label: qsTr("Copy this line"),
                            picture: settings.glyphSet + "/copy.svg",
                            act: () => settings.copyToClipboard(turn.modelData.text)
                        }, {

                            label: qsTr("Copy the whole transcript"),
                            picture: settings.glyphSet + "/copy.svg",
                            act: () => settings.copyToClipboard(transcript.openAsText())
                        }]
                    }

                    topPadding: 3
                    bottomPadding: 3

                    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding

                    onClicked: {
                        readingPanel.following = true
                        recordings.seek(transcript.startOf(index))
                    }

                    onDoubleClicked: {
                        readingPanel.following = true
                        recordings.seek(transcript.startOf(index))
                        recordings.startPlaying()
                    }

                    contentItem: RowLayout {
                        spacing: 8

                        Label {
                            Layout.alignment: Qt.AlignTop
                            Layout.preferredWidth: 46
                            text: Duration.text(modelData.atMs)
                            font.pixelSize: 11
                            opacity: 0.5
                        }

                        ColumnLayout {
                            Layout.alignment: Qt.AlignTop
                            Layout.fillWidth: true
                            spacing: 1

                            Label {
                                text: modelData.speaker.length > 0
                                      ? qsTr("%1 · %2").arg(modelData.who)
                                                       .arg(modelData.speaker)
                                      : modelData.who
                                font.pixelSize: 11
                                font.bold: true

                                opacity: modelData.near ? 0.85 : 0.6
                            }

                            Label {
                                Layout.fillWidth: true
                                text: readingPanel.spoken(
                                          modelData,
                                          index === transcript.currentLine
                                          ? transcript.currentWord : -1,
                                          Material.accentColor)
                                textFormat: Text.StyledText
                                wrapMode: Text.WordWrap
                                font.pixelSize: 13
                            }
                        }
                    }
                }

                Connections {
                    target: transcript

                    function onPlayheadChanged() {
                        if (!readingPanel.following || transcript.currentLine < 0) {
                            return
                        }
                        turns.positionViewAtIndex(transcript.currentLine,
                                                  ListView.Contain)
                    }
                }
            }
        }
    }

    function askForAReading(under) {
        if (providers.usable.length > 1) {
            whichProvider.popup(under, 0, under.height)
        } else {
            transcript.transcribe()
        }
    }

    function provenanceText() {
        void Qt.uiLanguage

        if (transcript.language.length === 0) {
            return transcript.provenance
        }
        return qsTr("%1 · %2").arg(transcript.provenance).arg(transcript.language)
    }

    function spoken(line, litWord, lit) {
        const escape = (text) => String(text).replace(/&/g, "&amp;")
                                             .replace(/</g, "&lt;")
                                             .replace(/>/g, "&gt;")

        const text = String(line.text)
        if (litWord < 0 || !line.wordAt || litWord >= line.wordAt.length) {
            return escape(text)
        }

        const from = line.wordAt[litWord]
        const to = line.wordTo[litWord]
        if (from < 0 || to <= from) {
            return escape(text)
        }

        return escape(text.substring(0, from))
               + "<font color=\"" + lit + "\">" + escape(text.substring(from, to)) + "</font>"
               + escape(text.substring(to))
    }

    Menu {
        id: whichProvider

        contentItem: ListView {
            implicitHeight: Math.min(contentHeight, 10 * 40)
            model: whichProvider.contentModel
            interactive: true
            clip: true
            currentIndex: whichProvider.currentIndex

            ScrollBar.vertical: ThinScrollBar {}
        }

        implicitWidth: {
            let widest = 0
            for (let at = 0; at < count; ++at) {
                const one = itemAt(at)
                if (one !== null) {
                    widest = Math.max(widest, one.implicitWidth)
                }
            }
            return Math.max(180, widest)
        }

        Repeater {
            model: providers.usable

            delegate: MenuItem {
                required property var modelData

                text: modelData.isDefault

                      ? qsTr("%1 (default)").arg(modelData.name)
                      : modelData.name
                onTriggered: transcript.transcribeWith(modelData.id)
            }
        }
    }

    Menu {
        id: transcriptFormats

        MenuItem {

            text: qsTr("As text")
            onTriggered: readingPanel.offerToSave(0)
        }

        MenuItem {

            text: qsTr("As subtitles (SRT)")
            onTriggered: readingPanel.offerToSave(1)
        }

        MenuItem {

            text: qsTr("As subtitles (VTT)")
            onTriggered: readingPanel.offerToSave(2)
        }

        MenuSeparator {
            visible: transcript.rawKept
            height: visible ? implicitHeight : 0
        }

        MenuItem {

            visible: transcript.rawKept
            height: visible ? implicitHeight : 0

            text: qsTr("Source (JSON)")
            onTriggered: readingPanel.offerToSave(3)
        }
    }

    ConfirmDialog {
        id: removeReadingDialog

        title: qsTr("Delete this transcription?")
        actionText: qsTr("Delete")
        onAccepted: {
            transcript.forget()
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("The conversation and its sound are untouched. What the recogniser made of it goes, and so does its own answer beside the file.")
        }
    }

    function offerToSave(format) {
        if (savingTranscript.currentFolder.toString().length === 0) {
            savingTranscript.currentFolder =
                Platform.StandardPaths.writableLocation(Platform.StandardPaths.DocumentsLocation)
        }
        savingTranscript.chosenFormat = format
        savingTranscript.selectedFile =
            savingTranscript.currentFolder + "/" + transcript.exportName(format)
        savingTranscript.open()
    }

    Dialogs.FileDialog {
        id: savingTranscript

        property int chosenFormat: 0

        title: qsTr("Save the transcript")
        fileMode: Dialogs.FileDialog.SaveFile
        nameFilters: [qsTr("Text files (*.txt)"), qsTr("Subtitles (*.srt *.vtt)"),
                      qsTr("Data files (*.json)"), qsTr("All files (*)")]

        onAccepted: transcript.exportTo(savingTranscript.selectedFile, savingTranscript.chosenFormat)
    }
}
