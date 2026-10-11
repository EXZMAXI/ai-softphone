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

Window {
    id: splash

    flags: Qt.SplashScreen

    width: 260
    height: 180

    x: Screen.virtualX + (Screen.width - splash.width) / 2
    y: Screen.virtualY + (Screen.height - splash.height) / 2

    color: "transparent"
    visible: true

    Rectangle {
        anchors.fill: parent
        color: Material.background
        border.width: 1
        border.color: "#808080"
    }

    readonly property bool darkWanted: settings.darkWanted

    Material.theme: splash.darkWanted ? Material.Dark : Material.Light
    Material.accent: Material.Teal

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20

        Image {
            Layout.alignment: Qt.AlignHCenter
            source: "qrc:/icons/ai-softphone.svg"
            sourceSize.width: 64
            sourceSize.height: 64
        }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: true
        }
    }
}
