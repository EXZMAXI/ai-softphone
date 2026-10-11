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

    id: panel

    required property var page

    readonly property int leastHeight: 260

    property int seconds: 0

    readonly property color liveNear: settings.captureRunning
        ? panel.page.nearColour : panel.page.idleNearColour
    readonly property color liveFar: settings.captureRunning
        ? panel.page.farColour : panel.page.idleFarColour

    function sideLabel(side) {
        void Qt.uiLanguage
        return side === "near" ? qsTr("You") : qsTr("The other side")
    }

    function sideLevel(side) {
        return side === "near" ? settings.captureNearLevel : settings.captureFarLevel
    }

    Component.onCompleted: settings.useCaptureDrawing(settings.captureSketch)

    Connections {
        target: settings
        function onCaptureChanged() { settings.useCaptureDrawing(settings.captureSketch) }
    }

    Timer {
        interval: 1000
        repeat: true
        running: settings.captureRunning
        triggeredOnStart: true
        onTriggered: panel.seconds = settings.captureElapsedSeconds()
    }

    Connections {
        target: settings
        function onCaptureStateChanged() {
            if (!settings.captureRunning) {
                panel.seconds = 0
            }
        }
    }

    SignalDrawing {
        anchors.fill: parent
        visible: captureSound.showing && captureSound.behind
        feed: captureSound
        shape: SignalDrawing.Aurora
        localColour: panel.liveNear
        remoteColour: panel.liveFar
        faint: panel.page.faintColour
    }

    NameCapture {
        id: namingNow
    }

    RowLayout {
        anchors.centerIn: parent
        width: parent.width - 32
        spacing: 10

        SignalDrawing {
            Layout.preferredWidth: 7
            Layout.preferredHeight: theColumn.implicitHeight
            visible: captureSound.showing && captureSound.flanking
            feed: captureSound
            shape: SignalDrawing.Upright
            side: SignalDrawing.Remote
            localColour: panel.liveNear
            remoteColour: panel.liveFar
            faint: panel.page.faintColour
        }

    ColumnLayout {
        id: theColumn

        Layout.fillWidth: true
        spacing: 16

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 10

            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                implicitWidth: 12
                implicitHeight: 12
                radius: 6
                color: Mark.bad
                visible: settings.captureRunning
            }

            Label {
                id: itsName

                text: settings.captureRunning ? settings.captureWhat : qsTr("Capture")
                font.pixelSize: 20
                font.bold: true
                font.underline: settings.captureRunning && naming.hovered

                TapHandler {
                    enabled: settings.captureRunning
                    onTapped: namingNow.open()
                }

                HoverHandler {
                    id: naming

                    enabled: settings.captureRunning
                    cursorShape: Qt.PointingHandCursor
                }

                Hint {
                    owner: itsName
                    visible: settings.captureRunning && naming.hovered

                    text: qsTr("Press to rename")
                }
            }
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            opacity: 0.75

            text: settings.captureRunning
                  ? panel.page.elapsed(panel.seconds * 1000)
                  : !settings.captureAvailable
                    ? qsTr("This build cannot record other applications on this system yet.")
                    : !settings.captureEnabled
                      ? qsTr("Switch it on in Settings to record a conversation happening in another application.")
                      : settings.captureHeldOff
                        ? qsTr("The telephone is using the sound card. Capture waits for it.")
                        : settings.captureOnFinding !== SettingsViewModel.OnFinding.Never
                          && settings.captureWatchAvailable
                          ? qsTr("Waiting for a conversation in another application.")
                          : qsTr("Ready. Press Record when a conversation starts.")
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: captureSound.height
            visible: captureSound.showing && captureSound.height > 0

            SignalDrawing {
                anchors.fill: parent
                feed: captureSound
                shape: panel.page.shapeOf(captureSound.drawing)
                initials: panel.page.haloMark()
                localColour: panel.liveNear
                remoteColour: panel.liveFar
                faint: panel.page.faintColour
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            visible: captureSound.showing && captureSound.drawing === SoundViewModel.Levels
            spacing: 8

            Repeater {
                model: ["far", "near"]

                RowLayout {
                    required property string modelData

                    Layout.fillWidth: true
                    spacing: 8

                    Glyph {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16

                        source: panel.page.glyph(parent.modelData === "near" ? "mic" : "speaker")
                        opacity: 0.6
                    }

                    SignalDrawing {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 10
                        feed: captureSound
                        shape: SignalDrawing.Bar
                        side: parent.modelData === "near" ? SignalDrawing.Local
                                                          : SignalDrawing.Remote
                        localColour: panel.liveNear
                        remoteColour: panel.liveFar
                        faint: panel.page.faintColour
                    }
                }
            }
        }

        Button {
            id: recordHere

            Layout.fillWidth: true
            enabled: settings.captureAvailable && settings.captureEnabled
            highlighted: settings.captureRunning

            contentItem: Item {
                implicitHeight: 24

                Glyph {
                    anchors.centerIn: parent
                    width: 20
                    height: 20
                    source: panel.page.glyph(settings.captureRunning ? "record-stop" : "record")
                    color: recordHere.enabled ? recordHere.Material.foreground
                                              : recordHere.Material.hintTextColor
                }
            }

            onClicked: settings.captureRunning
                       ? settings.stopRecordingHere()
                       : settings.startRecordingHere(qsTr("Another application"))

            Hint {
                owner: recordHere
                visible: recordHere.hovered

                text: settings.captureRunning ? qsTr("Stop recording")
                                              : qsTr("Record this conversation")
            }
        }
    }

        SignalDrawing {
            Layout.preferredWidth: 7
            Layout.preferredHeight: theColumn.implicitHeight
            visible: captureSound.showing && captureSound.flanking
            feed: captureSound
            shape: SignalDrawing.Upright
            side: SignalDrawing.Local
            localColour: panel.liveNear
            remoteColour: panel.liveFar
            faint: panel.page.faintColour
        }
    }
}
