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
    id: answerPanel

    property string glyphs: ""

    property bool chosen: false

    readonly property var entries: artifacts.tabs.map((one) => ({id: one.id,
                                                                 label: one.label,
                                                                 glyph: "sparkle",
                                                                 working: one.working,
                                                                 failed: one.failed}))

    readonly property string openId: artifacts.openId

    readonly property bool working: artifacts.working

    property real playhead: 0

    readonly property string provenance: artifacts.madeBy

    property Component whenEmpty: null

    readonly property var notices: []

    function open(id) {
        artifacts.open(id)
    }

    property int jobRank: 20

    readonly property string jobsTitle: qsTr("Processing")

    function jobsNow() {
        void Qt.uiLanguage

        const out = []
        if (!transcript.available || artifacts.refusal.length > 0) {
            return out
        }
        for (const one of artifacts.choices) {
            out.push({

                         label: one.asks

                                ? qsTr("%1…").arg(one.label)
                                : one.label,
                         offered: transcript.done,
                         picture: "",
                         act: () => {
                             if (one.asks) {
                                 askingQuestion.ask(one.id, one.label, "")
                             } else {
                                 artifacts.run(one.id, "")
                             }
                         }
                     })
        }
        return out
    }

    readonly property string jobsNote: {
        void Qt.uiLanguage

        if (!transcript.available) {
            return ""
        }
        if (artifacts.refusal.length > 0) {
            return artifacts.refusal
        }
        if (!transcript.done) {
            return qsTr("This conversation is being transcribed now.")
        }
        return ""
    }

    property int actRank: 10

    property Component acts: Component {
        Row {
            spacing: 0

            PanelAct {
                glyphs: answerPanel.glyphs

                picture: "copy"
                shown: answerPanel.chosen && artifacts.failure.length === 0
                       && !artifacts.working

                text: qsTr("Copy")
                onClicked: settings.copyToClipboard(artifacts.openAsText())
            }

            PanelAct {
                glyphs: answerPanel.glyphs

                picture: "save"
                shown: answerPanel.chosen && artifacts.failure.length === 0
                       && !artifacts.working
                text: qsTr("Save as…")
                onClicked: answerPanel.offerToSaveTheAnswer()
            }

            PanelAct {
                glyphs: answerPanel.glyphs

                picture: "delete"
                shown: answerPanel.chosen

                text: qsTr("Delete")
                onClicked: removeArtefactDialog.ask(artifacts.openName())
            }
        }
    }

    property var answer: null

    readonly property int verdictRoom: {
        void Qt.uiLanguage

        let widest = 0
        if (answerPanel.answer && answerPanel.answer.criteria) {
            for (const one of answerPanel.answer.criteria) {
                verdictMetrics.text = artifacts.verdictText(one.verdict)
                widest = Math.max(widest, Math.ceil(verdictMetrics.advanceWidth))
            }
        }
        return Math.max(48, widest + 2)
    }

    TextMetrics {
        id: verdictMetrics

        font.pixelSize: 12
        font.bold: true
    }

    function readTheAnswer() {
        if (artifacts.structured.length === 0) {
            answerPanel.answer = null
            return
        }
        try {
            answerPanel.answer = JSON.parse(artifacts.structured)
        } catch (e) {

            answerPanel.answer = null
        }
    }

    Component.onCompleted: answerPanel.readTheAnswer()

    readonly property bool saidNothing: {
        if (artifacts.openId.length === 0 || artifacts.working
                || artifacts.failure.length > 0) {
            return false
        }
        const said = answerPanel.answer
        switch (artifacts.shape) {
        case "flags":
            return !said || (said.flags || []).length === 0
        case "list":
            return !said || (said.items || []).length === 0
        case "labels":
            return !said || (said.labels || []).length === 0
        case "rubric":
            return !said || ((said.criteria || []).length === 0
                             && said.overall === undefined)
        case "score":
            return !said || (said.score === undefined && said.overall === undefined)
        case "qa":
            return !said || ((said.answer || "").trim().length === 0
                             && (said.citations || said.evidence || []).length === 0)
        case "moments":
            return !said || (said.moments || []).length === 0
        default:
            return artifacts.content.trim().length === 0
        }
    }

    Connections {
        target: artifacts

        function onOpenChanged() {
            answerPanel.readTheAnswer()
        }
    }

    Label {
        anchors.fill: parent
        anchors.rightMargin: 16
        visible: artifacts.failure.length > 0 || artifacts.working
        text: artifacts.failure.length > 0
              ? artifacts.failure

              : qsTr("Working on it…")
        wrapMode: Text.WordWrap
        color: artifacts.failure.length > 0
               ? Ink.bad(Material.theme)
               : Material.foreground
        opacity: artifacts.failure.length > 0 ? 1.0 : 0.6

        RowMenu {
            extra: answerPanel.wholeActs()
        }
    }

    function wholeActs() {
        void Qt.uiLanguage

        const out = []
        if (artifacts.failure.length === 0) {
            out.push({

                         label: qsTr("Copy this answer"),
                         picture: settings.glyphSet + "/copy.svg",
                         act: () => settings.copyToClipboard(artifacts.openAsText())
                     })
        }

        if (artifacts.exchangeKept) {
            out.push({

                         label: qsTr("Copy the exchange with the model"),
                         picture: settings.glyphSet + "/copy.svg",
                         act: () => {
                             const said = artifacts.exchangeAsText()
                             if (said.length > 0) {
                                 settings.copyToClipboard(said)
                             }
                         }
                     })
        }
        return out
    }

    function rowActs(index) {
        void Qt.uiLanguage

        return [{

                    label: qsTr("Copy this line"),
                    picture: settings.glyphSet + "/copy.svg",
                    act: () => settings.copyToClipboard(artifacts.entryAsText(index))
                }].concat(answerPanel.wholeActs())
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: artefactBody.implicitHeight
        clip: true
        visible: artifacts.failure.length === 0 && !artifacts.working

        ScrollBar.vertical: ThinScrollBar { always: true }

        ColumnLayout {
            id: artefactBody

            width: parent.width - 16
            spacing: 8

            Label {
                Layout.fillWidth: true
                visible: answerPanel.saidNothing
                text: artifacts.shape === "flags"

                      ? qsTr("The model raised no red flags in this conversation.")

                      : qsTr("The model wrote nothing about this conversation.")
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6

                RowMenu {
                    extra: answerPanel.wholeActs()
                }
            }

            RowLayout {
                spacing: 8
                visible: artifacts.shape === "score"
                         || artifacts.shape === "rubric"

                Label {
                    text: answerPanel.answer
                          ? String(answerPanel.answer.overall
                                   !== undefined
                                   ? answerPanel.answer.overall
                                   : answerPanel.answer.score)
                          : ""
                    font.pixelSize: 22
                    font.bold: true
                }

                Label {
                    Layout.fillWidth: true
                    text: answerPanel.answer
                          ? (answerPanel.answer.rationale || "") : ""
                    wrapMode: Text.WordWrap
                    opacity: 0.8

                    RowMenu {
                        extra: answerPanel.wholeActs()
                    }
                }
            }

            Repeater {
                model: artifacts.shape === "rubric" && answerPanel.answer
                       ? answerPanel.answer.criteria : []

                delegate: Item {
                    id: criterion

                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    implicitHeight: criterionRow.implicitHeight

                    RowLayout {
                        id: criterionRow

                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: 8

                        Label {

                            Layout.preferredWidth: answerPanel.verdictRoom

                            text: {
                                void Qt.uiLanguage
                                return artifacts.verdictText(criterion.modelData.verdict)
                            }
                            font.pixelSize: 12
                            font.bold: true
                            color: criterion.modelData.verdict === "fail"
                                   ? Ink.bad(Material.theme)
                                   : criterion.modelData.verdict === "weak"
                                     ? Ink.warn(Material.theme)
                                     : Ink.good(Material.theme)
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            Label {
                                Layout.fillWidth: true
                                text: criterion.modelData.name
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                visible: (criterion.modelData.note || "").length > 0
                                text: criterion.modelData.note
                                wrapMode: Text.WordWrap
                                font.pixelSize: 12
                                opacity: 0.6
                            }
                        }
                    }

                    RowMenu {
                        extra: answerPanel.rowActs(criterion.index)
                    }
                }
            }

            Repeater {
                model: artifacts.shape === "list" && answerPanel.answer
                       ? answerPanel.answer.items : []

                delegate: Item {
                    id: action

                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    implicitHeight: actionRow.implicitHeight

                    RowLayout {
                        id: actionRow

                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: 8

                        Label {
                            Layout.alignment: Qt.AlignTop
                            text: "•"
                            opacity: 0.5
                        }

                        Label {
                            Layout.fillWidth: true
                            text: action.modelData.text
                            wrapMode: Text.WordWrap
                        }

                        Label {
                            Layout.alignment: Qt.AlignTop
                            visible: (action.modelData.owner || "").length > 0
                                     || (action.modelData.due || "").length > 0
                            text: [action.modelData.owner, action.modelData.due]
                                  .filter((one) => (one || "").length > 0)
                                  .join(" · ")
                            font.pixelSize: 12
                            opacity: 0.6
                        }
                    }

                    RowMenu {
                        extra: answerPanel.rowActs(action.index)
                    }
                }
            }

            Flow {
                spacing: 6
                visible: artifacts.shape === "labels"

                Repeater {
                    model: artifacts.shape === "labels" && answerPanel.answer
                           ? answerPanel.answer.labels : []

                    delegate: Label {
                        id: chip

                        required property var modelData
                        required property int index

                        text: taxonomy.titleOf(
                                  artifacts.openKind === "tags"
                                  ? "tag" : "category", chip.modelData.code)
                        padding: 4
                        leftPadding: 8
                        rightPadding: 8
                        background: Rectangle {
                            radius: 10
                            color: Material.color(Material.Grey,
                                                  Material.Shade300)
                            opacity: 0.35
                        }

                        RowMenu {
                            extra: answerPanel.rowActs(chip.index)
                        }
                    }
                }
            }

            Repeater {
                model: artifacts.shape === "flags" && answerPanel.answer
                       ? answerPanel.answer.flags : []

                delegate: ItemDelegate {
                    id: flag

                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    padding: 6

                    implicitHeight: flagBody.implicitHeight + topPadding + bottomPadding

                    enabled: flag.modelData.at_ms !== undefined
                    onClicked: recordings.seek(flag.modelData.at_ms)

                    onDoubleClicked: {
                        recordings.seek(flag.modelData.at_ms)
                        recordings.startPlaying()
                    }

                    RowMenu {
                        extra: answerPanel.rowActs(flag.index)
                    }

                    contentItem: ColumnLayout {
                        id: flagBody

                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                text: taxonomy.titleOf("red_flag",
                                                       flag.modelData.code)
                                elide: Text.ElideRight
                                color: flag.modelData.severity === "high"
                                       ? Ink.bad(Material.theme)
                                       : Material.foreground
                            }

                            Label {
                                text: {
                                    void Qt.uiLanguage
                                    return artifacts.severityText(
                                                flag.modelData.severity || "")
                                }
                                visible: text.length > 0
                                font.pixelSize: 12
                                color: flag.modelData.severity === "high"
                                       ? Ink.bad(Material.theme)
                                       : flag.modelData.severity === "medium"
                                         ? Ink.warn(Material.theme)
                                         : Material.foreground
                                opacity: flag.modelData.severity === "low" ? 0.6 : 1.0
                            }

                            Label {
                                visible: flag.modelData.at_ms !== undefined
                                text: Duration.text(flag.modelData.at_ms)
                                font.pixelSize: 12
                                opacity: 0.6
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: (flag.modelData.evidence || "").length > 0
                            text: "“" + flag.modelData.evidence + "”"
                            wrapMode: Text.WordWrap
                            font.pixelSize: 12
                            font.italic: true
                            opacity: 0.7
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                visible: artifacts.shape === "qa" && answerPanel.answer
                text: answerPanel.answer
                      ? (answerPanel.answer.answer || "") : ""
                wrapMode: Text.WordWrap

                RowMenu {
                    extra: answerPanel.wholeActs()
                }
            }

            Repeater {
                model: (artifacts.shape === "qa" || artifacts.shape === "score")
                       && answerPanel.answer
                       ? (answerPanel.answer.citations
                          || answerPanel.answer.evidence || [])
                       : []

                delegate: ItemDelegate {
                    id: citation

                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    padding: 6
                    implicitHeight: citationBody.implicitHeight + topPadding + bottomPadding

                    enabled: citation.modelData.at_ms !== undefined
                    onClicked: recordings.seek(citation.modelData.at_ms)

                    onDoubleClicked: {
                        recordings.seek(citation.modelData.at_ms)
                        recordings.startPlaying()
                    }

                    RowMenu {
                        extra: answerPanel.rowActs(citation.index)
                    }

                    contentItem: RowLayout {
                        id: citationBody

                        spacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: "“" + citation.modelData.quote + "”"
                            wrapMode: Text.WordWrap
                            font.italic: true
                            opacity: 0.7
                        }

                        Label {
                            Layout.alignment: Qt.AlignTop
                            visible: citation.modelData.at_ms !== undefined
                            text: Duration.text(citation.modelData.at_ms)
                            font.pixelSize: 12
                            opacity: 0.6
                        }
                    }
                }
            }

            Repeater {
                model: artifacts.shape === "moments" && answerPanel.answer
                       ? (answerPanel.answer.moments || []) : []

                delegate: ItemDelegate {
                    id: moment

                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    padding: 6
                    implicitHeight: momentBody.implicitHeight + topPadding + bottomPadding

                    enabled: moment.modelData.at_ms !== undefined
                    onClicked: recordings.seek(moment.modelData.at_ms)
                    onDoubleClicked: {
                        recordings.seek(moment.modelData.at_ms)
                        recordings.startPlaying()
                    }

                    RowMenu {
                        extra: answerPanel.rowActs(moment.index)
                    }

                    contentItem: RowLayout {
                        id: momentBody

                        spacing: 8

                        Label {
                            Layout.alignment: Qt.AlignTop
                            Layout.fillWidth: false
                            text: Duration.text(moment.modelData.at_ms || 0)
                            font.pixelSize: 12
                            opacity: 0.6
                        }

                        Label {
                            Layout.fillWidth: true
                            text: moment.modelData.text || ""
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                visible: artifacts.shape === "plain"
                         || artifacts.shape === "json"
                         || artifacts.shape.length === 0
                text: artifacts.content
                wrapMode: Text.WordWrap
                textFormat: Text.PlainText
                font.family: artifacts.shape === "json"
                             ? "monospace" : font.family

                RowMenu {
                    extra: answerPanel.wholeActs()
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 24

                RowMenu {
                    extra: answerPanel.wholeActs()
                }
            }
        }
    }

    Label {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 16
        visible: artifacts.provenance.length > 0
        text: artifacts.provenance
        elide: Text.ElideRight
        font.pixelSize: 12
        opacity: 0.6
    }
    function offerToSaveTheAnswer() {
        if (savingAnswer.currentFolder.toString().length === 0) {
            savingAnswer.currentFolder =
                Platform.StandardPaths.writableLocation(Platform.StandardPaths.DocumentsLocation)
        }
        savingAnswer.selectedFile = savingAnswer.currentFolder + "/" + artifacts.exportName()
        savingAnswer.open()
    }

    Dialogs.FileDialog {
        id: savingAnswer

        title: qsTr("Save the answer")
        fileMode: Dialogs.FileDialog.SaveFile
        nameFilters: [qsTr("Text files (*.txt)"), qsTr("All files (*)")]
        onAccepted: artifacts.saveTo(savingAnswer.selectedFile)
    }
    ConfirmDialog {
        id: removeArtefactDialog

        property string label: ""
        function ask(name) {
            removeArtefactDialog.label = name
            removeArtefactDialog.open()
        }

        title: qsTr("Delete this?")
        actionText: qsTr("Delete")
        onAccepted: {
            artifacts.forget()
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("%1 will be deleted. The conversation and its transcript are kept, and you can ask for it again. Deleting cannot be undone.").arg(removeArtefactDialog.label)
        }
    }

    ConfirmDialog {
        id: askingQuestion

        property string templateId: ""
        property string promptName: ""
        function ask(id, name, wording) {
            askingQuestion.templateId = id
            askingQuestion.promptName = name
            questionField.text = wording
            askingQuestion.open()
            questionField.forceActiveFocus()
        }
        title: askingQuestion.promptName

        onAccepted: {
            const wording = questionField.text.trim()
            if (wording.length === 0) {
                return
            }
            artifacts.run(askingQuestion.templateId, wording)
        }
        actionText: qsTr("Ask")
        destructive: false
        actionEnabled: questionField.text.trim().length > 0

        TextField {
            id: questionField

            TextMenu {}

            Layout.fillWidth: true

            placeholderText: qsTr("Your question")
            onAccepted: {
                if (text.trim().length > 0) {
                    askingQuestion.accept()
                }
            }
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("The model answers from this conversation.")
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: 0.6
        }
    }
}
