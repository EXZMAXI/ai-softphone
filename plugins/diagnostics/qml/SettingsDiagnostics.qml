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

    id: diagnosticsPage

    onVisibleChanged: if (visible) settings.refreshLog()

    Timer {
        interval: 3000
        repeat: true

        running: diagnosticsPage.visible
        onTriggered: settings.refreshLog()
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Debugging")
        font.pixelSize: 15
        font.bold: true
    }

    Button {
        Layout.fillWidth: true
        text: qsTr("Open the diagnostics")
        onClicked: page.diagnosticsRequested()
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Every SIP message this phone sends or receives, while it is happening, with the audio statistics of the calls that are up. It collects only while it is open and keeps nothing after it closes.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 16

        text: qsTr("Logging")
        font.pixelSize: 15
        font.bold: true
    }

    LabelledPicker {
        id: verbosityPicker

        readonly property var values: [SettingsViewModel.Verbosity.Error,
                                       SettingsViewModel.Verbosity.Warning,
                                       SettingsViewModel.Verbosity.Info,
                                       SettingsViewModel.Verbosity.Debug,
                                       SettingsViewModel.Verbosity.Trace]

        Layout.fillWidth: true
        placeholderText: qsTr("Log detail")
        model: values
        displayText: page.verbosityLabel(model[currentIndex])

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: verbosityPicker.width
            text: page.verbosityLabel(modelData)
            highlighted: verbosityPicker.highlightedIndex === index
        }

        currentIndex: Math.max(0, values.indexOf(settings.logLevel))
        onActivated: (chosen) => settings.chooseLogLevel(
                         verbosityPicker.values[chosen])

        Connections {
            target: settings
            function onDiagnosticsChanged() {
                verbosityPicker.currentIndex
                    = Qt.binding(() => Math.max(
                                     0, verbosityPicker.values.indexOf(
                                         settings.logLevel)))
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Takes effect straight away, including on a call that is already up — which is the one you want the record of. The most detailed setting writes down every SIP message; it is large, and passwords are removed from it before anything is written, so the file is safe to send with a support request.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    CheckBox {
        Layout.fillWidth: true
        Layout.topMargin: 4
        visible: settings.systemLogAvailable
        text: qsTr("Send a copy to the system log")
        checked: settings.systemLog
        onToggled: settings.setSystemLog(checked)
    }

    Label {
        Layout.fillWidth: true
        visible: settings.systemLogAvailable
        text: qsTr("For a machine whose logs are collected centrally. The file below is written either way, and it is the one to attach to a support request.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    PathRow {
        settingsPage: page
        visible: settings.systemLogAvailable && settings.systemLog
        caption: qsTr("To read it")
        path: settings.systemLogCommand
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 8
        text: qsTr("Files")
        font.pixelSize: 15
        font.bold: true
    }

    PathRow {
        settingsPage: page
        caption: qsTr("Settings")
        path: settings.configPath
        bytes: settings.configBytes
    }

    PathRow {
        settingsPage: page
        caption: qsTr("Database")
        path: settings.databasePath
        bytes: settings.databaseBytes
    }

    PathRow {
        settingsPage: page
        caption: qsTr("Recordings")
        path: settings.recordingsPath
    }

    PathRow {
        settingsPage: page
        caption: qsTr("Log")
        path: settings.logPath
        bytes: settings.logBytes
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 8
        spacing: 8

        Button {
            Layout.fillWidth: true
            text: qsTr("Open")
            enabled: settings.logBytes > 0
            onClicked: settings.openLog()
        }

        Button {
            Layout.fillWidth: true
            text: qsTr("Clear")
            enabled: settings.logBytes > 0

            onClicked: page.clearLog.open()
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Clear the log just before you reproduce a problem. Clearing cannot be undone.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Item { Layout.fillHeight: true }
}
