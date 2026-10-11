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

ApplicationWindow {
    id: notice

    required property var phone

    title: qsTr("What is reported")

    flags: Qt.Dialog
    modality: Qt.ApplicationModal

    width: 520
    minimumWidth: 400

    height: Math.min(shell.implicitHeight + 24,
                     notice.screen ? notice.screen.height - 96 : 720)
    minimumHeight: 320

    Material.theme: settings.darkWanted ? Material.Dark : Material.Light
    Material.accent: Material.Teal

    function ask() {
        if (!notice.visible) {

            const on = notice.phone && notice.phone.screenForOurWindows
                       ? notice.phone.screenForOurWindows() : notice.screen
            if (on !== null && on !== undefined) {
                notice.x = on.virtualX + Math.round((on.width - notice.width) / 2)
                notice.y = on.virtualY + Math.round((on.height - notice.height) / 3)
            }
            notice.show()
        }

        notice.raise()
        notice.requestActivate()
    }

    function away() {
        notice.hide()
    }

    onClosing: (event) => {
        if (!notice.phone.quitting) {
            event.accepted = false
            notice.away()
        }
    }

    readonly property bool readToTheEnd:
        downTheSide.position + downTheSide.size >= 0.999

    ColumnLayout {
        id: shell

        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Label {
            Layout.fillWidth: true

            text: qsTr("This program sends us a small report about itself so that we know which versions and which systems to keep working. Here is exactly what that means, and you can change it whenever you like in Settings, under About.")
            wrapMode: Text.WordWrap
        }

        ScrollView {
            id: scroller

            Layout.fillWidth: true

            Layout.fillHeight: true
            Layout.minimumHeight: 0
            Layout.preferredHeight: Math.min(body.implicitHeight, 360)
            contentWidth: availableWidth
            clip: true

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ScrollBar.vertical: ThinScrollBar {
                id: downTheSide

                always: true

                parent: scroller
                x: scroller.mirrored ? 0 : scroller.width - width
                y: scroller.topPadding
                height: scroller.availableHeight
            }

            TelemetryChoice {
                id: body
                width: scroller.availableWidth
                explainOff: false
            }
        }

        Label {
            Layout.fillWidth: true

            text: qsTr("Read to the end to continue.")
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: notice.readToTheEnd ? 0 : 0.7
        }

        Button {
            Layout.fillWidth: true
            highlighted: true
            enabled: notice.readToTheEnd

            text: qsTr("Continue")
            onClicked: {
                telemetry.acceptNotice()
                notice.away()
            }
        }
    }
}
