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

import Qt.labs.platform as Platform

import Softphone

ColumnLayout {

    required property var page
    Layout.fillWidth: true
    spacing: 12

    Label {
        Layout.fillWidth: true
        text: qsTr("Autostart")
        font.pixelSize: 15
        font.bold: true
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        StartupSwitch {

            objectName: "theWayIn"
            text: qsTr("Start the phone when I log in")
            checked: settings.autostart
            onToggled: settings.setAutostart(checked)
        }

        StartupSwitch {
            enabled: settings.autostart
            text: qsTr("Start minimized, without a window")
            checked: settings.startMinimised
            onToggled: settings.setStartMinimised(checked)
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("A phone that starts with the session takes calls from the moment you sit down.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 8

        text: qsTr("Close", "heading: what closing the window does")
        font.pixelSize: 15
        font.bold: true
    }

    CheckBox {
        Layout.fillWidth: true
        text: qsTr("Keep the phone running when the window is closed")
        checked: settings.closeToTray

        onToggled: settings.setCloseToTray(checked)
    }

    Label {
        Layout.fillWidth: true

        text: page.trayShown
            ? qsTr("A closed window is not a hung-up phone: calls still arrive, and the icon in the notification area brings the window back. Turn this off and closing the window quits.")
            : qsTr("A closed window is not a hung-up phone: calls still arrive, and the phone waits minimised in the task bar. Turn this off and closing the window quits.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 12
        visible: settings.linksPossible

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 8

            text: qsTr("Call links")
            font.pixelSize: 15
            font.bold: true
        }

        Label {
            Layout.fillWidth: true
            text: settings.linksOurs
                  ? qsTr("This phone opens tel:, callto: and sip: links.")
                  : settings.linksHeldBy.length > 0
                    ? qsTr("Telephone numbers on web pages open with %1.").arg(settings.linksHeldBy)
                    : qsTr("Nothing on this computer opens telephone numbers on web pages.")
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: 0.6
        }

        Button {
            Layout.fillWidth: true

            text: qsTr("Open call links with this phone")
            enabled: !settings.linksOurs
            onClicked: settings.claimLinks()
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            ListCheckBox {
                text: qsTr("Call straight away, without pressing Call")
                checked: settings.callLinkDialsAtOnce
                onToggled: settings.setCallLinkDialsAtOnce(checked)
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("A clicked number arrives in the dialler and waits there. Turn this on to ring it the moment it arrives.")
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: 0.6
        }

        Component.onCompleted: settings.refreshLinks()

        Connections {
            target: page

            function onVisibleChanged() {
                if (page.visible) {
                    settings.refreshLinks()
                }
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 12
        visible: settings.fromAnImage

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 8
            text: qsTr("Applications menu")
            font.pixelSize: 15
            font.bold: true
        }

        CheckBox {
            Layout.fillWidth: true
            text: qsTr("Show the phone in the applications menu")
            checked: settings.menuEntryWanted
            onToggled: settings.setMenuEntry(checked)
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("The entry starts this file from where it is now. Move it and the phone puts the entry right the next time you run it; delete it and your desktop stops offering it.")
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: 0.6
        }
    }

    Item { Layout.fillHeight: true }
}
