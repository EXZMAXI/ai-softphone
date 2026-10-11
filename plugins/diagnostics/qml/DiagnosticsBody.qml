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

import Softphone

Item {
    id: diagnosticsBody

    objectName: "diagnosticsBody"

    implicitWidth: 560

    property int leastWidth: 460

    readonly property string drawing: "pulse"

    property string glyphs: ""

    property bool onScreen: false

    property bool kept: false

    property bool chosen: true

    property bool settling: false

    property bool focused: false

    property int section: 0

    property var openedMessage: 0

    readonly property var textLocale: Qt.uiLanguage ? Qt.locale(Qt.uiLanguage) : Qt.locale()

    readonly property bool watching: diagnosticsBody.kept && diagnosticsBody.chosen

    onWatchingChanged: diagnostics.watch(diagnosticsBody.watching)

    Component.onCompleted: {
        diagnostics.watch(diagnosticsBody.watching)
    }

    function timeText(ms) {
        void Qt.uiLanguage
        return new Date(ms).toLocaleTimeString(diagnosticsBody.textLocale, "HH:mm:ss.zzz")
    }

    function callTimes(call) {
        void Qt.uiLanguage

        const started = new Date(call.atMs).toLocaleTimeString(diagnosticsBody.textLocale,
                                                              "HH:mm:ss")
        if (call.endedAtMs <= 0) {
            return started
        }

        const ended = new Date(call.endedAtMs).toLocaleTimeString(diagnosticsBody.textLocale,
                                                                  "HH:mm:ss")
        return qsTr("%1 – %2").arg(started).arg(ended)
    }

    function byteText(bytes) {
        void Qt.uiLanguage

        const locale = diagnosticsBody.textLocale
        if (bytes < 1024)
            return qsTr("%1 B").arg(Number(bytes).toLocaleString(locale, 'f', 0))
        if (bytes < 1024 * 1024)
            return qsTr("%1 kB").arg(Number(bytes / 1024).toLocaleString(locale, 'f', 0))
        return qsTr("%1 MB").arg(Number(bytes / (1024 * 1024)).toLocaleString(locale, 'f', 1))
    }

    function delayText(milliseconds) {
        void Qt.uiLanguage
        return qsTr("%1 ms").arg(
            Number(milliseconds).toLocaleString(diagnosticsBody.textLocale, 'f', 1))
    }

    function callHeading(call) {
        void Qt.uiLanguage

        const who = call.name !== "" && call.name !== call.number
                  ? qsTr("%1 (%2)").arg(call.name).arg(call.number)
                  : (call.number !== "" ? call.number : qsTr("unknown"))

        const state = diagnosticsBody.callStateText(call.state)
        if (call.durationSeconds > 0)
            return qsTr("%1 · %2 · %3").arg(who).arg(state)
                     .arg(diagnosticsBody.durationText(call.durationSeconds))
        return qsTr("%1 · %2").arg(who).arg(state)
    }

    function callStateText(state) {
        void Qt.uiLanguage

        switch (state) {
        case 0:
            return qsTr("idle")
        case 1:
            return qsTr("dialling")
        case 2:
            return qsTr("ringing out")
        case 3:
            return qsTr("ringing in")
        case 4:
            return qsTr("connected")
        case 5:
            return qsTr("ended")
        }
        return ""
    }

    function durationText(seconds) {
        void Qt.uiLanguage

        const minutes = Math.floor(seconds / 60)
        const rest = seconds % 60
        return qsTr("%1:%2").arg(minutes).arg(String(rest).padStart(2, "0"))
    }

    function codecText(call) {
        void Qt.uiLanguage

        if (call.codec === "")
            return qsTr("No codec negotiated yet")

        const rate = qsTr("%1 · %2 kHz").arg(call.codec)
                       .arg(Math.round(call.clockRate / 1000))

        const codec = call.encrypted ? qsTr("%1 · encrypted").arg(rate) : rate
        if (!call.payloadsDiffer)
            return codec

        return qsTr("%1 · sending payload %2, receiving %3").arg(codec)
                 .arg(call.sentPayload).arg(call.receivedPayload)
    }

    function countText(current, total) {
        void Qt.uiLanguage

        if (total > current)
            return qsTr("%1 (%2 in all)").arg(current).arg(total)
        return String(current)
    }

    function counterLabel(counter) {
        void Qt.uiLanguage

        switch (counter.name) {
        case "registrations_succeeded_total":
            return qsTr("Registrations accepted")
        case "registrations_failed_total":
            return qsTr("Registrations refused")
        case "calls_incoming_total":
            return qsTr("Calls arrived")
        case "calls_outgoing_total":
            return qsTr("Calls placed")
        case "calls_answered_total":
            return qsTr("Calls answered")
        case "calls_missed_total":
            return qsTr("Calls missed")
        case "calls_declined_total":
            return qsTr("Calls refused")
        case "calls_failed_total":
            return qsTr("Calls failed")
        case "api_requests_total":
            return qsTr("Requests to this phone")
        case "api_requests_refused_total":
            return qsTr("Requests turned away")
        case "webhooks_delivered_total":
            return qsTr("Webhooks delivered")
        case "webhooks_failed_total":
            return qsTr("Webhooks failed")
        case "webhooks_dropped_total":
            return qsTr("Webhooks dropped")
        }

        return counter.help
    }

    function emptyText() {
        void Qt.uiLanguage

        if (diagnosticsBody.section === 0) {
            if (diagnostics.total > 0)
                return qsTr("Nothing here matches what you typed.")
            return qsTr("Nothing yet. This window starts empty every time it is opened.")
        }

        return qsTr("No calls at the moment.")
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        focus: true

        Keys.onEscapePressed: (event) => {
            diagnosticsBody.openedMessage = 0
            diagnosticsBody.forceActiveFocus()
            event.accepted = true
        }

        TabBar {
            id: sections

            Layout.fillWidth: true
            currentIndex: diagnosticsBody.section
            onCurrentIndexChanged: diagnosticsBody.section = currentIndex

            background: Rectangle {
                color: Material.foreground
                opacity: 0.04
            }

            TabButton {
                text: qsTr("SIP")
            }

            TabButton {
                text: qsTr("Calls")
            }

        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 4
            spacing: 8
            visible: diagnosticsBody.section === 0

            TextField {
                id: filterField

                TextMenu {}

                Layout.fillWidth: true
                placeholderText: qsTr("Search")
                text: diagnostics.filter
                onTextEdited: diagnostics.filter = text

                rightPadding: clearFilter.visible ? clearFilter.width + 8 : undefined

                ToolButton {
                    id: clearFilter

                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    visible: filterField.text !== ""
                    text: "✕"
                    font.pixelSize: 12
                    onClicked: {
                        diagnostics.filter = ""
                        filterField.text = ""
                    }
                }
            }

            Button {
                text: qsTr("Clear")
                enabled: diagnostics.total > 0
                onClicked: {
                    diagnosticsBody.openedMessage = 0
                    diagnostics.clear()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 4
            spacing: 8
            visible: diagnosticsBody.section === 1

            Label {
                Layout.fillWidth: true
                text: qsTr("Calls are kept until this window is closed.")
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6
            }

            Button {
                text: qsTr("Clear")
                enabled: diagnostics.calls.length > 0
                onClicked: diagnostics.clearCalls()
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Label {
                anchors.fill: parent
                anchors.margins: 32
                text: diagnosticsBody.emptyText()
                visible: diagnosticsBody.section === 0 && diagnostics.shown === 0
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                opacity: 0.6
                font.pixelSize: 13
            }

            ListView {
                id: messageList

                anchors.fill: parent
                visible: diagnosticsBody.section === 0
                clip: true

                model: diagnostics.rows
                spacing: 2

                ScrollBar.vertical: ThinScrollBar {}

                delegate: ItemDelegate {
                    id: messageRow

                    required property var modelData

                    readonly property bool opened:
                        diagnosticsBody.openedMessage === messageRow.modelData.sequence

                    width: messageList.width
                    leftPadding: 16
                    rightPadding: 16
                    height: messageBody.implicitHeight + 12

                    onClicked: diagnosticsBody.openedMessage =
                               messageRow.opened ? 0 : messageRow.modelData.sequence

                    RowMenu {
                        visible: !messageRow.opened
                        extra: [{
                            label: qsTr("Copy the message"),
                            picture: settings.glyphSet + "/copy.svg",
                            act: () => settings.copyToClipboard(
                                     diagnostics.textOf(messageRow.modelData.sequence))
                        }]
                    }

                    contentItem: ColumnLayout {
                        id: messageBody

                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                text: messageRow.modelData.incoming ? "←" : "→"
                                color: messageRow.modelData.incoming
                                       ? Ink.info(Material.theme)
                                       : Ink.good(Material.theme)
                                font.pixelSize: 15
                            }

                            Label {
                                text: diagnosticsBody.timeText(messageRow.modelData.atMs)
                                font.pixelSize: 11
                                font.family: "monospace"
                                opacity: 0.6
                            }

                            Label {
                                Layout.fillWidth: true
                                text: messageRow.modelData.summary
                                elide: Text.ElideRight
                                font.pixelSize: 13
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            Layout.leftMargin: 23
                            text: messageRow.modelData.incoming
                                  ? qsTr("from %1 over %2").arg(
                                        messageRow.modelData.remoteAddress).arg(
                                        messageRow.modelData.transport)
                                  : qsTr("to %1 over %2").arg(
                                        messageRow.modelData.remoteAddress).arg(
                                        messageRow.modelData.transport)
                            elide: Text.ElideRight
                            font.pixelSize: 11
                            opacity: 0.6
                        }

                        TextEdit {

                            TextMenu {
                                extra: [{
                                    label: qsTr("Copy the message"),
                                    picture: settings.glyphSet + "/copy.svg",
                                    act: () => settings.copyToClipboard(
                                             diagnostics.textOf(messageRow.modelData.sequence))
                                }]
                            }

                            Layout.fillWidth: true
                            Layout.topMargin: 6
                            visible: messageRow.opened
                            text: messageRow.opened
                                  ? diagnostics.textOf(messageRow.modelData.sequence)
                                  : ""
                            readOnly: true
                            selectByMouse: true
                            wrapMode: TextEdit.WrapAnywhere
                            font.pixelSize: 11
                            font.family: "monospace"
                            color: Material.foreground
                            opacity: 0.85
                        }
                    }
                }

                footer: Label {

                    width: messageList.width
                    leftPadding: 16
                    rightPadding: 16
                    visible: diagnostics.dropped > 0
                    height: visible ? implicitHeight + 8 : 0
                    text: qsTr("%n older message(s) dropped to keep this window small.", "",
                               diagnostics.dropped)
                    wrapMode: Text.WordWrap
                    font.pixelSize: 11
                    opacity: 0.5
                }
            }

            ListView {
                id: callList

                anchors.fill: parent
                anchors.topMargin: 8
                visible: diagnosticsBody.section === 1
                clip: true
                model: diagnostics.calls
                spacing: 12

                ScrollBar.vertical: ThinScrollBar {}

                header: Label {
                    width: callList.width
                    leftPadding: 16
                    rightPadding: 16
                    visible: diagnostics.droppedCalls > 0
                    height: visible ? implicitHeight + 8 : 0
                    text: qsTr("%n older call(s) dropped to keep this window small.", "",
                               diagnostics.droppedCalls)
                    wrapMode: Text.WordWrap
                    font.pixelSize: 11
                    opacity: 0.5
                }

                delegate: Item {
                    id: callCardRow

                    required property var modelData

                    width: callList.width
                    implicitHeight: callCard.implicitHeight
                    height: implicitHeight

                    ColumnLayout {
                        id: callCard

                        readonly property var modelData: callCardRow.modelData

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 3

                        Label {
                            Layout.fillWidth: true
                            text: diagnosticsBody.callHeading(callCard.modelData)
                            wrapMode: Text.WordWrap
                            font.pixelSize: 15
                            font.bold: true
                        }

                        Label {
                            Layout.fillWidth: true
                            text: diagnosticsBody.callTimes(callCard.modelData)
                            font.pixelSize: 11
                            font.family: "monospace"
                            opacity: 0.6
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: !callCard.modelData.everHadMedia
                            text: qsTr("No audio yet")
                            font.pixelSize: 13
                            opacity: 0.7
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: callCard.modelData.everHadMedia && !callCard.modelData.live
                            text: qsTr("The audio has stopped; these are its last figures.")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 13
                            opacity: 0.7
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: callCard.modelData.everHadMedia
                            text: diagnosticsBody.codecText(callCard.modelData)
                            wrapMode: Text.WordWrap
                            font.pixelSize: 13
                            opacity: 0.85
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: callCard.modelData.everHadMedia &&
                                     callCard.modelData.localAddress !== ""
                            text: qsTr("RTP %1 → %2").arg(callCard.modelData.localAddress)
                                  .arg(callCard.modelData.remoteAddress)
                            wrapMode: Text.WrapAnywhere
                            font.pixelSize: 13
                            font.family: "monospace"
                            opacity: 0.85
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: callCard.modelData.everHadMedia
                            text: qsTr("Sent: %1 packets, %2, %3 lost")
                                  .arg(diagnosticsBody.countText(callCard.modelData.sentPackets,
                                                                   callCard.modelData.sentPacketsTotal))
                                  .arg(diagnosticsBody.byteText(callCard.modelData.sentBytes))
                                  .arg(callCard.modelData.sentLost)
                            wrapMode: Text.WordWrap
                            font.pixelSize: 13
                            opacity: 0.85
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: callCard.modelData.everHadMedia
                            text: qsTr("Received: %1 packets, %2, %3 lost, %4 discarded")
                                  .arg(diagnosticsBody.countText(
                                           callCard.modelData.receivedPackets,
                                           callCard.modelData.receivedPacketsTotal))
                                  .arg(diagnosticsBody.byteText(callCard.modelData.receivedBytes))
                                  .arg(callCard.modelData.receivedLost)
                                  .arg(callCard.modelData.receivedDiscarded)
                            wrapMode: Text.WordWrap
                            font.pixelSize: 13
                            opacity: 0.85
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: callCard.modelData.everHadMedia
                            text: qsTr("Jitter %1 · round trip %2")
                                  .arg(diagnosticsBody.delayText(
                                           callCard.modelData.receivedJitterMs))
                                  .arg(diagnosticsBody.delayText(callCard.modelData.roundTripMs))
                            font.pixelSize: 13
                            opacity: 0.85
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: callCard.modelData.everHadMedia
                            text: qsTr("The loss figure for what was sent is the far end's report, not ours.")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 12
                            opacity: 0.55
                        }
                    }
                }

                footer: Item {
                    id: totalsRow

                    width: callList.width
                    implicitHeight: totals.implicitHeight
                    height: implicitHeight

                    ColumnLayout {
                        id: totals

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 2

                        Label {
                            Layout.fillWidth: true
                            Layout.topMargin: 16
                            text: qsTr("Totals since start")
                            font.pixelSize: 15
                            font.bold: true
                            visible: diagnostics.counters.length > 0
                        }

                        Repeater {
                            model: diagnostics.counters

                            delegate: Label {
                                required property var modelData

                                Layout.fillWidth: true
                                text: diagnosticsBody.counterLabel(modelData) + ": " +
                                      Math.round(modelData.value)
                                wrapMode: Text.WordWrap
                                font.pixelSize: 14
                                opacity: 0.85
                            }
                        }
                    }
                }
            }
        }
    }
}
