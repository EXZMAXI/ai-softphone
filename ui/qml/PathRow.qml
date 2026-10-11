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
import QtQuick.Controls.Material.impl
import QtQuick.Layouts

import Softphone

ColumnLayout {
    id: pathRow

    required property var settingsPage
    required property string caption
    required property string path

    property real bytes: -1

    readonly property string shown: settings.readablePath(pathRow.path)

    Layout.fillWidth: true
    spacing: 2

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label {
            text: pathRow.caption
            font.pixelSize: 12
            opacity: 0.6
        }

        Item { Layout.fillWidth: true }

        Label {
            visible: pathRow.bytes >= 0
            text: settingsPage.logSizeText(pathRow.bytes)
            font.pixelSize: 12
            opacity: 0.6
        }
    }

    Label {
        id: pathText

        Layout.fillWidth: true
        text: pathRow.shown
        wrapMode: Text.WrapAnywhere
        font.pixelSize: 13
        font.family: "monospace"

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }

        TapHandler {
            onTapped: {
                settings.copyToClipboard(pathRow.shown)

                pathText.ToolTip.show(
                    qsTr("The path is on the clipboard"), 2000)
            }
        }
    }
}
