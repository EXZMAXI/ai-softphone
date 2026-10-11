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
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Window {
    id: notice

    property string heading: ""

    property string name: ""

    property bool darkWanted: false

    signal pressed()

    default property alias actions: acts.data

    width: 340
    height: frame.implicitHeight + 2

    transientParent: null
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"
    visible: false

    Material.theme: notice.darkWanted ? Material.Dark : Material.Light
    Material.accent: Material.Teal

    Rectangle {
        id: frame

        anchors.fill: parent
        anchors.margins: 1
        radius: 12
        color: Material.background

        border.width: 1
        border.color: Material.color(Material.Grey, notice.darkWanted
                                                    ? Material.Shade700 : Material.Shade300)

        implicitHeight: layout.implicitHeight + 24

        MouseArea {
            anchors.fill: parent
            onClicked: notice.pressed()
        }

        ColumnLayout {
            id: layout

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 12
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: notice.heading
                font.pixelSize: 12
                opacity: 0.6
                elide: Text.ElideRight
                visible: text.length > 0
            }

            Label {
                Layout.fillWidth: true
                text: notice.name
                font.pixelSize: 16
                font.bold: true
                elide: Text.ElideRight
                visible: text.length > 0
            }

            ColumnLayout {
                id: acts

                Layout.fillWidth: true
                spacing: 8
            }
        }
    }
}
