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

ColumnLayout {
    id: historyTab

    readonly property bool newestInView:
        visible && Window.window !== null && Window.window.visible
        && Window.window.visibility !== Window.Minimized
        && historyList.atYBeginning

    onNewestInViewChanged: history.watching(historyTab, newestInView)
    Component.onCompleted: history.watching(historyTab, newestInView)

    property string glyphs: "qrc:/glyphs/regular"

    property bool chosen: false

    readonly property string title: qsTr("History")

    readonly property string drawing: "history"

    property var dial: function (place) { place() }

    property var addContact: undefined

    signal recordingsRequested(string callId)

    signal dialRefused(int reason)

    Connections {
        target: history

        function onDialRefused(reason) {
            historyTab.dialRefused(reason)
        }
    }

    spacing: 12

    property string chosenCall: ""

    property string chosenSeance: ""

    onChosenChanged: {
        if (!historyTab.chosen) {
            historyTab.chosenCall = ""
            historyTab.chosenSeance = ""
        }
    }

    function glyph(name) {
        return historyTab.glyphs === "" ? "" : historyTab.glyphs + "/" + name + ".svg"
    }

    readonly property int controlHeight: 40

    readonly property int filterHeight: 44

    readonly property var textLocale: Qt.uiLanguage ? Qt.locale(Qt.uiLanguage) : Qt.locale()

    readonly property var formatLocale: Qt.locale()

    function capitalised(text) {
        return text.length > 0 ? text.charAt(0).toUpperCase() + text.slice(1) : text
    }

    function filterLabel(filter) {
        void Qt.uiLanguage

        switch (filter) {
        case HistoryViewModel.Filter.Missed:
            return qsTr("Missed", "filter, several calls")
        case HistoryViewModel.Filter.Declined:

            return qsTr("Declined", "filter, several calls")
        case HistoryViewModel.Filter.Incoming:
            return qsTr("Incoming", "filter, several calls")
        case HistoryViewModel.Filter.Outgoing:
            return qsTr("Outgoing", "filter, several calls")
        default:
            return qsTr("All calls")
        }
    }

    function outcomeText(row) {
        void Qt.uiLanguage

        switch (row.outcome) {
        case HistoryViewModel.Outcome.Answered:
            return row.incoming ? qsTr("Incoming", "one call") : qsTr("Outgoing", "one call")
        case HistoryViewModel.Outcome.Missed:
            return row.incoming ? qsTr("Missed", "one call") : qsTr("No answer", "one call")
        case HistoryViewModel.Outcome.Declined:
            return row.incoming ? qsTr("Declined") : qsTr("Busy or declined")
        case HistoryViewModel.Outcome.PickedUpElsewhere:

            return qsTr("Answered elsewhere")
        default:
            return qsTr("Did not go through")
        }
    }

    function answeredText(row) {
        void Qt.uiLanguage

        switch (row.answeredBy) {
        case HistoryViewModel.Answered.Invitation:
        case HistoryViewModel.Answered.SwitchRequest:
            return qsTr("Answered by the switch")
        case HistoryViewModel.Answered.AccountSetting:
            return qsTr("Answered automatically")
        default:
            return ""
        }
    }

    function durationText(seconds) {
        void Qt.uiLanguage

        if (seconds <= 0) {
            return ""
        }
        const minutes = Math.floor(seconds / 60)
        return minutes > 0 ? qsTr("%1 min %2 s").arg(minutes).arg(seconds % 60)
                           : qsTr("%1 s").arg(seconds)
    }

    function whenText(row) {
        void Qt.uiLanguage

        const at = new Date(row.startedAtMs)
        const clock = at.toLocaleTimeString(historyTab.formatLocale, Locale.ShortFormat)

        switch (row.stamp) {
        case HistoryViewModel.Stamp.Time:
            return clock
        case HistoryViewModel.Stamp.Yesterday:
            return qsTr("Yesterday, %1").arg(clock)
        case HistoryViewModel.Stamp.Weekday:
            return qsTr("%1, %2")

                   .arg(historyTab.capitalised(
                            historyTab.textLocale.dayName(at.getDay(), Locale.LongFormat)))
                   .arg(clock)
        case HistoryViewModel.Stamp.DayMonth:

            return qsTr("%1, %2")
                   .arg(historyTab.capitalised(
                            at.toLocaleDateString(historyTab.textLocale, qsTr("d MMMM"))))
                   .arg(clock)
        default:
            return qsTr("%1, %2")
                   .arg(at.toLocaleDateString(historyTab.formatLocale, Locale.ShortFormat))
                   .arg(clock)
        }
    }

    function rowDetail(row) {
        void Qt.uiLanguage

        const when = historyTab.whenText(row)
        const parts = [when, historyTab.outcomeText(row)]

        const lasted = historyTab.durationText(row.durationSeconds)
        if (lasted !== "") {
            parts.push(lasted)
        }

        const answered = historyTab.answeredText(row)
        if (answered !== "") {
            parts.push(answered)
        }
        if (row.name !== "" && row.name !== row.number) {
            parts.push(row.number)
        }

        if (row.attempts > 1) {
            parts.push(qsTr("%n attempt(s)", "", row.attempts))
        }

        if (row.lineLabel !== "" && !row.lineMissing) {
            parts.push(row.lineLabel)
        }
        return parts.join(" · ")
    }

    function rowLine(row) {
        void Qt.uiLanguage

        return row.lineMissing
             ? qsTr("%1 — this line is gone; the call goes out on the current one")
               .arg(row.lineLabel)
             : ""
    }

    function emptyText() {
        void Qt.uiLanguage

        if (!history.available) {
            return qsTr("The call history cannot be opened, so recent calls cannot be shown. The phone works as usual.")
        }
        if (history.shown > 0) {
            return ""
        }

        if (history.search !== "") {
            return qsTr("No calls match what you typed.")
        }
        return history.filtered ? qsTr("No calls of that kind yet.")
                                : qsTr("No calls yet. Every call you make or take will be listed here.")
    }

    ConfirmDialog {
        id: forgetCall

        property string callId: ""
        property string callName: ""

        title: qsTr("Delete this call?")
        actionText: qsTr("Delete")
        onAccepted: {
            history.forget(forgetCall.callId)
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap

            text: qsTr("The call to %1 leaves the history and cannot be brought back. A recording of it, if there is one, stays in the library.").arg(forgetCall.callName)
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16

        Layout.topMargin: 0
        spacing: 8
        visible: history.available

        PickerBox {
            id: filterPicker

            Layout.fillWidth: true
            implicitHeight: historyTab.filterHeight

            model: [HistoryViewModel.Filter.All, HistoryViewModel.Filter.Missed,
                    HistoryViewModel.Filter.Declined, HistoryViewModel.Filter.Incoming,
                    HistoryViewModel.Filter.Outgoing]

            displayText: historyTab.filterLabel(currentValue)

            delegate: ItemDelegate {

                required property var modelData
                required property int index

                width: filterPicker.width
                text: historyTab.filterLabel(modelData)
                highlighted: filterPicker.highlightedIndex === index
            }

            onActivated: history.filter = currentValue
        }

        TextField {
            id: searchField

            TextMenu {}

            implicitHeight: historyTab.filterHeight

            Layout.fillWidth: true
            placeholderText: qsTr("Search")
            text: history.search
            onTextEdited: history.search = text

            rightPadding: clearSearch.visible ? clearSearch.width + 8 : undefined

            ToolButton {
                id: clearSearch

                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: searchField.text !== ""
                display: AbstractButton.IconOnly
                icon.source: historyTab.glyph("dismiss")
                icon.width: 14
                icon.height: 14
                flat: true
                text: qsTr("Clear the search")
                Accessible.name: text

                Hint {
                    owner: clearSearch
                    visible: clearSearch.hovered
                    text: clearSearch.text
                }
                onClicked: {
                    searchField.clear()
                    history.search = ""
                }
            }
        }
    }

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ListView {
            id: historyList

            anchors.fill: parent
            visible: history.available && history.shown > 0
            clip: true

            spacing: 0

            model: history.rows

            ScrollBar.vertical: ThinScrollBar {}

            footer: Item {
                width: historyList.width

                height: earlier.visible ? earlier.height + 4 : 0

                Button {
                    id: earlier

                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 4
                    width: parent.width - 32
                    height: 36
                    flat: true

                    topInset: 0
                    bottomInset: 0
                    visible: history.more
                    text: qsTr("Show earlier calls")
                    onClicked: history.showMore()
                }
            }

            delegate: ItemDelegate {
                id: historyRowItem

                required property var modelData

                width: historyList.width

                height: Math.max(historyText.implicitHeight, historyTab.controlHeight) + 16
                hoverEnabled: true
                highlighted: historyTab.chosenCall === modelData.id

                Rectangle {
                    anchors.fill: parent
                    z: -1
                    visible: !historyRowItem.highlighted && historyTab.chosenSeance.length > 0
                             && historyRowItem.modelData.seanceId === historyTab.chosenSeance
                    color: Material.accentColor
                    opacity: 0.12
                }

                readonly property bool showsControls:
                    historyRowItem.hovered
                    || historyTab.chosenCall === historyRowItem.modelData.id

                RowMenu {
                    acts: historyRowActs
                    number: modelData.number
                }

                onClicked: {
                    const again = historyTab.chosenCall === modelData.id
                    historyTab.chosenCall = again ? "" : modelData.id
                    historyTab.chosenSeance = again ? "" : (modelData.seanceId || "")
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    onDoubleTapped: historyTab.dial(
                        () => history.redial(historyRowItem.modelData.id))
                }

                RowLayout {
                    id: historyRow

                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 0

                    Glyph {

                        Layout.rightMargin: 8
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16

                        source: historyTab.glyph(CallGlyph.forRow(modelData))

                        color: CallInk.forMark(modelData.mark)
                    }

                    ColumnLayout {
                        id: historyText

                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 2

                        Label {
                            Layout.fillWidth: true
                            text: modelData.name !== "" ? modelData.name : modelData.number
                            elide: Text.ElideRight
                            font.pixelSize: 15
                        }

                        Label {
                            Layout.fillWidth: true
                            text: historyTab.rowDetail(modelData)
                            elide: Text.ElideRight
                            opacity: 0.7
                            font.pixelSize: 13
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: text !== ""
                            text: historyTab.rowLine(modelData)
                            elide: Text.ElideRight
                            opacity: 0.7
                            font.pixelSize: 12
                        }
                    }
                }

                RowActs {
                    id: historyRowActs

                    anchors.right: parent.right
                    anchors.rightMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    visible: historyRowItem.showsControls
                    over: historyRowItem.hovered
                    picked: historyRowItem.highlighted

                        RowButton {
                            glyphs: historyTab.glyphs

                            visible: historyRowItem.showsControls && !modelData.known
                                     && historyTab.addContact !== undefined
                            glyph: "person-add"
                            text: qsTr("Add to contacts")

                            onClicked: {
                                const seed = history.contactSeed(modelData.id)
                                historyTab.addContact(seed.number, seed.name)
                            }
                        }

                        RowButton {
                            glyphs: historyTab.glyphs
                            visible: historyRowItem.showsControls && modelData.recordings > 0
                            glyph: "resume"
                            text: qsTr("Listen to the recording")

                            onClicked: historyTab.recordingsRequested(modelData.callId)
                        }

                        RowButton {
                            glyphs: historyTab.glyphs
                            glyph: "delete"
                            text: qsTr("Delete this call")

                            onClicked: {
                                forgetCall.callId = historyRowItem.modelData.id
                                forgetCall.callName =
                                    historyRowItem.modelData.name.length > 0
                                        ? historyRowItem.modelData.name
                                        : historyRowItem.modelData.number
                                forgetCall.open()
                            }
                        }

                        RowButton {
                            glyphs: historyTab.glyphs
                            glyph: "call"
                            text: qsTr("Call")
                            onClicked: historyTab.dial(() => history.redial(modelData.id))
                        }
                }
            }
        }

        Label {
            anchors.centerIn: parent
            width: parent.width - 32
            visible: text !== ""
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            opacity: 0.7
            text: historyTab.emptyText()
        }
    }

}
