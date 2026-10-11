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

RowLayout {
    id: mini

    required property var page

    visible: settings.captureAvailable && settings.captureEnabled

    spacing: 4

    property int seconds: 0

    Timer {
        interval: 1000
        running: settings.captureRunning && mini.visible
        repeat: true
        triggeredOnStart: true
        onTriggered: mini.seconds = settings.captureElapsedSeconds()
    }

    ToolSeparator {
        Layout.fillHeight: true
        padding: 2
    }

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
        Layout.alignment: Qt.AlignVCenter
        visible: settings.captureRunning
        font.pixelSize: 12
        opacity: 0.75
        text: mini.page.elapsed(mini.seconds * 1000)
    }

    StripKey {
        enabled: settings.captureAvailable && settings.captureEnabled

        drawing: settings.captureRunning ? "record-stop" : "record"
        marked: settings.captureRunning
        tint: settings.captureRunning ? Mark.bad : Material.foreground

        hint: settings.captureRunning

              ? qsTr("Recording %1 — press to stop").arg(settings.captureWhat)

              : qsTr("Record a conversation happening in another application")

        onPressedHere: settings.captureRunning
                       ? settings.stopRecordingHere()
                       : settings.startRecordingHere(qsTr("Another application"))
    }
}
