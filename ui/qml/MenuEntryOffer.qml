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
    id: offer

    required property var phone

    signal answered(bool wanted)

    title: qsTr("Add to the menu?")

    flags: Qt.Dialog
    modality: Qt.ApplicationModal

    width: 460
    minimumWidth: 380

    height: Math.min(shell.implicitHeight + 24,
                     offer.screen ? offer.screen.height - 96 : 420)
    minimumHeight: 200

    Material.theme: settings.darkWanted ? Material.Dark : Material.Light
    Material.accent: Material.Teal

    function ask() {
        if (!offer.visible) {

            const on = offer.phone && offer.phone.screenForOurWindows
                       ? offer.phone.screenForOurWindows() : offer.screen
            if (on !== null && on !== undefined) {
                offer.x = on.virtualX + Math.round((on.width - offer.width) / 2)
                offer.y = on.virtualY + Math.round((on.height - offer.height) / 3)
            }
            offer.show()
        }

        offer.raise()
        offer.requestActivate()
    }

    function away() {
        offer.hide()
    }

    onClosing: (event) => {
        if (!offer.phone.quitting) {
            event.accepted = false
            offer.answered(false)
            offer.away()
        }
    }

    ColumnLayout {
        id: shell

        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("This phone is one file, and a desktop will not start it by double-clicking. An entry in the applications menu is what lets you find it where you find your other programs.")
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: 0.6
            text: qsTr("You can change this later in Settings, under Startup.")
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Button {
            Layout.fillWidth: true
            highlighted: true
            text: qsTr("Add")
            onClicked: {
                offer.answered(true)
                offer.away()
            }
        }

        Button {
            Layout.fillWidth: true
            flat: true
            text: qsTranslate("QPlatformTheme", "Cancel")
            onClicked: {
                offer.answered(false)
                offer.away()
            }
        }
    }
}
