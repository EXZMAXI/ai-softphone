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
    id: bar

    required property var page

    readonly property int keyHeight: 34

    implicitHeight: 54

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 6
        anchors.topMargin: 6
        anchors.bottomMargin: 6
        spacing: 8

        DialField {
            id: field

            implicitHeight: bar.keyHeight

            Layout.preferredWidth: 200
            Layout.alignment: Qt.AlignVCenter

            cleaner: calls
            placeholderText: qsTr("Number")
            onDialEdited: dialled => calls.dialText = dialled
            onAccepted: calls.dial()

            Binding {
                target: field
                property: "text"
                value: calls.dialText
                restoreMode: Binding.RestoreNone
            }
        }

        Button {
            id: dialKey

            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: bar.keyHeight
            Layout.preferredHeight: bar.keyHeight

            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            bottomPadding: 0
            topInset: 0
            bottomInset: 0
            leftInset: 0
            rightInset: 0

            enabled: (calls.canDial || calls.lastDialled !== "") && accounts.connected

            Accessible.name: qsTr("Call")

            background: Rectangle {
                radius: height / 2

                color: dialKey.enabled
                       ? (dialKey.down ? Qt.darker(Mark.good, 1.35) : Mark.good)
                       : Qt.rgba(Material.foreground.r, Material.foreground.g,
                                 Material.foreground.b, 0.12)
            }

            contentItem: Item {
                Glyph {
                    anchors.centerIn: parent
                    width: Math.round(bar.keyHeight * 0.58)
                    height: width
                    source: settings.glyphSet + "/call.svg"
                    color: dialKey.enabled ? "white"
                                           : Qt.rgba(Material.foreground.r, Material.foreground.g,
                                                     Material.foreground.b, 0.4)
                }
            }

            onClicked: {
                if (calls.canDial) {
                    calls.dial()
                } else {
                    calls.dialText = calls.lastDialled
                }
            }
        }

        ToolSeparator {
            Layout.fillHeight: true
            padding: 2
        }

        Flickable {
            id: callsHere

            Layout.fillWidth: true
            Layout.fillHeight: true

            contentWidth: callRow.implicitWidth
            contentHeight: height
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.HorizontalFlick

            RowLayout {
                id: callRow

                height: callsHere.height
                spacing: 6

                Repeater {
                    model: calls.callList

                    Rectangle {
                        id: callChip

                        required property var modelData

                        readonly property bool ringing:
                            callChip.modelData.state === CallViewModel.RingingIn
                        readonly property bool over:
                            callChip.modelData.state === CallViewModel.Ended

                        Layout.preferredHeight: bar.keyHeight
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: chipRow.implicitWidth + 16

                        radius: 6
                        color: callChip.modelData.current
                               ? Qt.rgba(Material.accent.r, Material.accent.g,
                                         Material.accent.b, 0.12)
                               : Qt.rgba(Material.foreground.r, Material.foreground.g,
                                         Material.foreground.b, 0.06)

                        TapHandler {
                            onTapped: calls.select(callChip.modelData.callId)
                        }

                        RowLayout {
                            id: chipRow

                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 6

                            Rectangle {
                                Layout.preferredWidth: 8
                                Layout.preferredHeight: 8
                                Layout.alignment: Qt.AlignVCenter
                                radius: 4
                                color: bar.page.callColor(callChip.modelData)
                            }

                            ColumnLayout {
                                Layout.alignment: Qt.AlignVCenter
                                Layout.maximumWidth: 180
                                spacing: 0

                                Label {
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    text: callChip.modelData.who
                                    font.pixelSize: 12
                                    font.bold: true
                                }

                                Label {
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    text: bar.page.rowStateText(callChip.modelData)
                                    font.pixelSize: 10
                                    opacity: 0.7
                                }
                            }

                            BarKey {
                                offered: callChip.ringing
                                tint: Mark.good
                                drawing: "call"

                                hint: qsTr("Answer")
                                onPressedHere: {
                                    calls.select(callChip.modelData.callId)
                                    calls.answer()
                                }
                            }

                            BarKey {
                                offered: !callChip.ringing && !callChip.over
                                drawing: callChip.modelData.held ? "resume" : "hold"
                                hint: callChip.modelData.held ? qsTr("Resume") : qsTr("Hold")
                                onPressedHere: calls.toggleHoldCall(callChip.modelData.callId)
                            }

                            BarKey {
                                offered: callChip.modelData.live && !callChip.over
                                drawing: calls.muted ? "mic-off" : "mic"
                                hint: calls.muted ? qsTr("Unmute") : qsTr("Mute")
                                onPressedHere: calls.toggleMute()
                            }

                            BarKey {
                                offered: callChip.modelData.current && calls.talking
                                         && calls.canRecord
                                enabled: calls.canStartRecording
                                drawing: !calls.recording ? "record"
                                                          : (calls.recordingPaused
                                                             ? "record" : "record-pause")
                                hint: !calls.recording ? qsTr("Record")
                                                       : (calls.recordingPaused
                                                          ? qsTr("Resume recording")
                                                          : qsTr("Pause recording"))
                                onPressedHere: calls.toggleRecording()
                            }

                            BarKey {
                                offered: callChip.modelData.current && calls.canTransfer
                                drawing: "transfer"

                                hint: qsTr("Transfer…")
                                onPressedHere: bar.page.askWhereToTransfer()
                            }

                            BarKey {
                                offered: callChip.modelData.current && calls.canMerge
                                drawing: "merge"
                                hint: calls.mergePartner.length > 0
                                      ? qsTr("Merge with %1").arg(calls.mergePartner)
                                      : qsTr("Merge all")
                                onPressedHere: calls.merge()
                            }

                            BarKey {
                                offered: callChip.modelData.current && calls.inConference
                                drawing: "split"
                                hint: qsTr("Split")
                                onPressedHere: calls.split()
                            }

                            BarKey {
                                offered: !callChip.over
                                tint: Mark.bad
                                drawing: callChip.ringing ? "call-dismiss" : "call-end"
                                hint: callChip.ringing ? qsTr("Decline") : qsTr("Hang up")
                                onPressedHere: {
                                    if (callChip.ringing) {
                                        calls.rejectCall(callChip.modelData.callId)
                                    } else {
                                        calls.hangUpCall(callChip.modelData.callId)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        ToolSeparator {
            Layout.fillHeight: true
            padding: 2
        }

        Repeater {
            model: accounts.count

            ToolButton {
                id: lineChip

                required property int index

                readonly property var line: {
                    void accounts.count
                    return accounts.accountAt(lineChip.index)
                }

                visible: lineChip.line.enabled === true

                Layout.alignment: Qt.AlignVCenter
                implicitHeight: 26
                topInset: 0
                bottomInset: 0
                topPadding: 0
                bottomPadding: 0
                leftPadding: 8
                rightPadding: 8
                implicitWidth: Math.min(150, lineRow.implicitWidth + leftPadding + rightPadding)
                focusPolicy: Qt.NoFocus
                hoverEnabled: true

                onClicked: accounts.chooseDialAccount(lineChip.line.id)

                Accessible.name: qsTr("Call from %1").arg(lineChip.line.shortLabel)

                background: Rectangle {
                    radius: 13
                    color: lineChip.hovered
                           ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                                     Material.foreground.b, 0.10)
                           : "transparent"
                    border.width: lineChip.line.dialsFrom === true ? 1 : 0
                    border.color: Qt.rgba(Material.accentColor.r, Material.accentColor.g,
                                          Material.accentColor.b, 0.55)
                }

                contentItem: Row {
                    id: lineRow

                    spacing: 6

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        anchors.verticalCenter: parent.verticalCenter
                        color: bar.page.lineColor(lineChip.line)
                    }

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.min(implicitWidth, 110)
                        elide: Text.ElideRight
                        text: lineChip.line.shortLabel
                        font.pixelSize: 11
                        opacity: 0.85
                    }
                }
            }
        }

        BarKey {
            id: dndKey

            offered: true
            marked: calls.doNotDisturb
            tint: Mark.bad
            drawing: "call-prohibited"
            hint: calls.doNotDisturb
                  ? qsTr("Do not disturb is on — every incoming call is refused. Click to turn it off.")
                  : qsTr("Do not disturb: refuse every incoming call")
            onPressedHere: calls.toggleDoNotDisturb()
        }
    }

    component BarKey: StripKey {}
}
