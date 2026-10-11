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
    id: transferWindow

    required property var phone

    readonly property alias body: shell

    title: qsTr("Transfer %1").arg(calls.remoteLabel)

    flags: Qt.Dialog
    modality: Qt.ApplicationModal

    width: 360
    minimumWidth: 320

    height: shell.implicitHeight + 24
    minimumHeight: shell.implicitHeight + 24
    maximumHeight: shell.implicitHeight + 24

    Material.theme: transferWindow.phone.darkWanted ? Material.Dark : Material.Light
    Material.accent: transferWindow.phone.Material.accent

    function ask() {
        shell.begin()

        if (!transferWindow.visible) {

            const on = transferWindow.phone && transferWindow.phone.screenForOurWindows
                       ? transferWindow.phone.screenForOurWindows() : transferWindow.screen
            if (on !== null && on !== undefined) {
                transferWindow.x = on.virtualX + Math.round((on.width - transferWindow.width) / 2)
                transferWindow.y = on.virtualY + Math.round((on.height - transferWindow.height) / 3)
            }
            transferWindow.show()
        }

        transferWindow.raise()
        transferWindow.requestActivate()
        shell.takeTheKeyboard()

        calls.beginTransfer()
    }

    function away() {
        if (!shell.arranged) {
            calls.cancelTransfer()
        }
        transferWindow.hide()
    }

    onClosing: (event) => {
        if (!transferWindow.phone.quitting) {
            event.accepted = false
            transferWindow.away()
        }
    }

    Connections {
        target: calls
        function onCallChanged() {
            if (transferWindow.visible && !calls.canTransfer) {
                transferWindow.away()
            }
        }
    }

    TransferBody {
        id: shell

        anchors.fill: parent
        anchors.margins: 12

        page: transferWindow.phone
        onFinished: transferWindow.away()
    }
}
