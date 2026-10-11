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
    id: strip

    required property var page

    property bool offered: settings.captureAvailable && settings.captureEnabled
                           && !strip.saidLargerAbove

    readonly property bool saidLargerAbove: {
        void uiRegistry.slotRevision
        const pages = uiRegistry.itemsIn("primary")
        return pages.length > 0 && pages[0] === "capture.page"
    }

    implicitHeight: body.implicitHeight

    readonly property color liveNear: settings.captureRunning
        ? strip.page.nearColour : strip.page.idleNearColour
    readonly property color liveFar: settings.captureRunning
        ? strip.page.farColour : strip.page.idleFarColour

    property int seconds: 0

    function useTheDrawing() {
        if (strip.offered) {
            settings.useCaptureDrawing(settings.captureStripSketch)
        }
    }

    onOfferedChanged: strip.useTheDrawing()
    Component.onCompleted: strip.useTheDrawing()

    Connections {
        target: settings
        function onCaptureChanged() { strip.useTheDrawing() }
    }

    Timer {
        interval: 1000
        repeat: true
        running: settings.captureRunning
        triggeredOnStart: true
        onTriggered: strip.seconds = settings.captureElapsedSeconds()
    }

    Connections {
        target: settings
        function onCaptureStateChanged() {
            if (!settings.captureRunning) {
                strip.seconds = 0
            }
        }
    }

    SignalDrawing {
        anchors.fill: parent
        visible: captureSound.showing && captureSound.behind
        feed: captureSound
        shape: SignalDrawing.Aurora
        localColour: strip.liveNear
        remoteColour: strip.liveFar
        faint: strip.page.faintColour
    }

    NameCapture {
        id: namingHere
    }

    ColumnLayout {
        id: body

        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: strip.page.faintColour
        }

        RowLayout {
            id: line

            Layout.fillWidth: true

            Layout.topMargin: 4
            Layout.bottomMargin: 4

            Layout.leftMargin: strip.page.pageEdge
            Layout.rightMargin: strip.page.pageEdge

            spacing: 8

        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: 8
            implicitHeight: 8
            radius: 4
            color: Mark.bad
            visible: settings.captureRunning

            SequentialAnimation on opacity {
                running: settings.captureRunning
                loops: Animation.Infinite
                NumberAnimation { to: 0.35; duration: 900 }
                NumberAnimation { to: 1.0; duration: 900 }
            }
        }

        Label {
            id: itsName

            Layout.fillWidth: true
            elide: Text.ElideRight
            font.pixelSize: 12
            font.underline: settings.captureRunning && naming.hovered

            TapHandler {
                enabled: settings.captureRunning
                onTapped: namingHere.open()
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

            text: settings.captureRunning
                  ? settings.captureWhat
                  : settings.captureOnFinding !== SettingsViewModel.OnFinding.Never
                    && settings.captureWatchAvailable

                    ? qsTr("Capture · waiting for a conversation")

                    : qsTr("Capture · ready")
        }

        Label {
            Layout.alignment: Qt.AlignVCenter
            visible: settings.captureRunning
            font.pixelSize: 12
            opacity: 0.75
            text: page.elapsed(strip.seconds * 1000)
        }

        StripKey {
            implicitWidth: 24
            implicitHeight: 24

            icon.width: 14
            icon.height: 14

            enabled: settings.captureAvailable && settings.captureEnabled

            drawing: settings.captureRunning ? "record-stop" : "record"
            marked: settings.captureRunning
            tint: settings.captureRunning ? Mark.bad : Material.foreground

            hint: settings.captureRunning ? qsTr("Stop recording")
                                          : qsTr("Record this conversation")

            onPressedHere: settings.captureRunning
                           ? settings.stopRecordingHere()
                           : settings.startRecordingHere(qsTr("Another application"))
        }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.bottomMargin: 4
            visible: captureSound.showing && captureSound.drawing === SoundViewModel.Levels
            spacing: 4

            Repeater {
                model: ["far", "near"]

                SignalDrawing {
                    required property string modelData

                    Layout.fillWidth: true
                    Layout.preferredHeight: 6
                    feed: captureSound
                    shape: SignalDrawing.Bar
                    side: modelData === "near" ? SignalDrawing.Local : SignalDrawing.Remote
                    localColour: strip.liveNear
                    remoteColour: strip.liveFar
                    faint: strip.page.faintColour
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.bottomMargin: 4
            implicitHeight: 28
            visible: captureSound.showing && !captureSound.behind
                     && captureSound.drawing !== SoundViewModel.Levels

            SignalDrawing {
                anchors.fill: parent
                feed: captureSound
                shape: strip.page.shapeOf(captureSound.drawing)
                localColour: strip.liveNear
                remoteColour: strip.liveFar
                faint: strip.page.faintColour
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: strip.page.faintColour
        }
    }
}
