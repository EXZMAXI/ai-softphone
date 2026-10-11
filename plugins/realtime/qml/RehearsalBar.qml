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

Frame {
    id: bar

    required property string glyphs

    padding: 8

    Layout.topMargin: Math.ceil(itsName.height / 2)

    readonly property int windowMs: 60000

    property int offsetMs: 0

    readonly property bool zoomed: prompter.rehearsalLengthMs > bar.windowMs

    readonly property int shownMs: bar.zoomed
                                   ? bar.windowMs
                                   : Math.max(1, prompter.rehearsalLengthMs)

    Item {
        id: body

        anchors.fill: parent

        implicitHeight: theRow.implicitHeight

        Label {
            id: itsName

            x: 4
            y: -bar.topPadding - itsName.height / 2
            z: 1

            width: Math.min(implicitWidth, bar.width - 48)
            text: prompter.rehearsalName
            elide: Text.ElideRight
            font: bar.font
            leftPadding: 4
            rightPadding: 4
            color: bar.Material.hintTextColor

            background: Rectangle {
                color: bar.Material.backgroundColor
            }

            HoverHandler {
                id: overTheName
            }

            Hint {
                owner: itsName
                visible: overTheName.hovered && itsName.truncated
                text: prompter.rehearsalName
            }
        }

        RowLayout {
            id: theRow

            anchors.fill: parent
            spacing: 8

            PanelAct {
                glyphs: bar.glyphs

                picture: prompter.rehearsalPlaying ? "hold" : "resume"

                text: prompter.rehearsalPlaying ? qsTr("Pause") : qsTr("Play")
                hint: prompter.rehearsalPlaying

                      ? qsTr("Stop the recording where it is")

                      : qsTr("Play the recording into the prompter")
                onClicked: prompter.playRehearsal(!prompter.rehearsalPlaying)
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 2

                Item {
                    id: picture

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    implicitHeight: picture.stereo ? 44 : 34

                    readonly property var nearBars: {
                        void prompter.rehearsalWaveRevision
                        return prompter.rehearsalWaveNow("near")
                    }

                    readonly property var farBars: {
                        void prompter.rehearsalWaveRevision
                        return prompter.rehearsalWaveNow("far")
                    }

                    readonly property bool stereo: prompter.rehearsalStereo
                                                   && picture.farBars.length > 0

                    readonly property real from: prompter.rehearsalLengthMs > 0
                                                 ? bar.offsetMs / prompter.rehearsalLengthMs : 0
                    readonly property real to: prompter.rehearsalLengthMs > 0
                                               ? (bar.offsetMs + bar.shownMs)
                                                 / prompter.rehearsalLengthMs : 1
                    readonly property real played: prompter.rehearsalLengthMs > 0
                                                   ? prompter.rehearsalAtMs
                                                     / prompter.rehearsalLengthMs : 0

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: picture.stereo ? 2 : 0

                        Lane {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            bars: picture.nearBars
                            from: picture.from
                            to: picture.to
                            played: picture.played
                            ink: bar.ink
                            tint: Material.accent
                        }

                        Lane {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            visible: picture.stereo
                            bars: picture.farBars
                            from: picture.from
                            to: picture.to
                            played: picture.played
                            ink: bar.ink
                            tint: Material.accent
                        }
                    }

                    Rectangle {
                        x: (prompter.rehearsalAtMs - bar.offsetMs) / bar.shownMs * parent.width - 1
                        width: 2
                        height: parent.height
                        visible: x > -2 && x < parent.width
                        color: Material.accent
                    }

                    HoverHandler {
                        cursorShape: Qt.PointingHandCursor
                    }

                    TapHandler {
                        onTapped: (point) => bar.seekTo(point.position.x / picture.width)
                    }

                    DragHandler {
                        target: null
                        yAxis.enabled: false
                        onCentroidChanged: if (active) {
                            bar.seekTo(centroid.position.x / picture.width)
                        }
                    }

                    WheelHandler {
                        enabled: bar.zoomed
                        onWheel: (event) => {
                            const steps = event.angleDelta.y / 120
                            bar.scrollTo(bar.offsetMs - steps * bar.windowMs * 0.25)
                        }
                    }
                }

                ThinScrollBar {
                    id: waveScroll

                    Layout.fillWidth: true

                    visible: bar.zoomed
                    orientation: Qt.Horizontal
                    always: true
                    size: prompter.rehearsalLengthMs > 0
                          ? bar.shownMs / prompter.rehearsalLengthMs : 1

                    onPositionChanged: {
                        const wanted = waveScroll.position * prompter.rehearsalLengthMs
                        if (Math.abs(wanted - bar.offsetMs) > 1) {
                            bar.scrollTo(wanted)
                        }
                    }
                }
            }

            Label {

                text: qsTr("%1 / %2").arg(bar.asClock(prompter.rehearsalAtMs))
                                     .arg(bar.asClock(prompter.rehearsalLengthMs))
                font.pixelSize: 12
                opacity: 0.6
            }

            PanelAct {
                glyphs: bar.glyphs

                picture: "dismiss"

                text: qsTr("Stop rehearsing")
                hint: qsTr("Back to listening to calls")
                onClicked: prompter.stopRehearsing()
            }
        }
    }

    readonly property color ink: Qt.rgba(Material.foreground.r, Material.foreground.g,
                                         Material.foreground.b, 0.45)

    function asClock(ms) {
        const whole = Math.max(0, Math.floor(ms / 1000))
        const minutes = Math.floor(whole / 60)
        const seconds = whole % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    function scrollTo(ms) {
        const most = Math.max(0, prompter.rehearsalLengthMs - bar.windowMs)
        bar.offsetMs = Math.round(Math.min(most, Math.max(0, ms)))
        bar.showTheHandle()
    }

    function showTheHandle() {
        if (prompter.rehearsalLengthMs <= 0) {
            return
        }
        const wanted = bar.offsetMs / prompter.rehearsalLengthMs
        if (Math.abs(wanted - waveScroll.position) > 0.0005) {
            waveScroll.position = wanted
        }
    }

    function seekTo(fraction) {
        if (prompter.rehearsalLengthMs <= 0) {
            return
        }
        const ms = bar.offsetMs + Math.min(1, Math.max(0, fraction)) * bar.shownMs
        prompter.seekRehearsal(
            Math.round(Math.min(prompter.rehearsalLengthMs, Math.max(0, ms))))
    }

    function keepUp() {
        if (!bar.zoomed) {
            bar.offsetMs = 0
            bar.showTheHandle()
            return
        }
        const head = prompter.rehearsalAtMs
        if (head < bar.offsetMs || head > bar.offsetMs + bar.windowMs) {
            bar.scrollTo(head - bar.windowMs / 2)
        }
    }

    Connections {
        target: prompter

        function onRehearsalAtChanged() { bar.keepUp() }

        function onRehearsalChanged() {
            bar.offsetMs = 0
            bar.showTheHandle()
        }
    }
}
