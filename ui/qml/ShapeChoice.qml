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
    id: choice

    required property var phone

    required property bool trayAvailable

    property var picked: settings.layout

    title: qsTr("How should the phone look?")

    flags: Qt.Dialog
    modality: Qt.ApplicationModal

    width: 560
    minimumWidth: 560
    height: shell.implicitHeight + 24
    minimumHeight: shell.implicitHeight + 24
    maximumHeight: shell.implicitHeight + 24

    Material.theme: settings.darkWanted ? Material.Dark : Material.Light
    Material.accent: Material.Teal

    function ask() {
        if (!choice.visible) {

            const on = choice.phone && choice.phone.screenForOurWindows
                       ? choice.phone.screenForOurWindows() : choice.screen
            if (on !== null && on !== undefined) {
                choice.x = on.virtualX + Math.round((on.width - choice.width) / 2)
                choice.y = on.virtualY + Math.round((on.height - choice.height) / 3)
            }
            choice.show()
        }

        choice.raise()
        choice.requestActivate()
    }

    function away() {
        choice.hide()
    }

    onClosing: (event) => {
        if (!choice.phone.quitting) {
            event.accepted = false
            choice.away()
        }
    }

    ColumnLayout {
        id: shell

        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Label {
            Layout.fillWidth: true

            text: qsTr("The same windows in every arrangement — what differs is where they stand.")
            wrapMode: Text.WordWrap
        }

        GridLayout {
            Layout.fillWidth: true

            columns: 3
            columnSpacing: 8
            rowSpacing: 8

            Repeater {

                model: [SettingsViewModel.Layout.WorkspaceAndPhone,
                        SettingsViewModel.Layout.Windows,
                        SettingsViewModel.Layout.Compact,
                        SettingsViewModel.Layout.Workspace,
                        SettingsViewModel.Layout.Bar,
                        SettingsViewModel.Layout.NoWindow]

                ItemDelegate {
                    id: shapeCell

                    required property var modelData

                    readonly property bool marked: choice.picked === shapeCell.modelData

                    Layout.fillWidth: true
                    Layout.preferredHeight: 118

                    padding: 8

                    enabled: shapeCell.modelData !== SettingsViewModel.Layout.NoWindow
                             || choice.trayAvailable
                    opacity: shapeCell.enabled ? 1 : 0.4

                    onClicked: choice.picked = shapeCell.modelData

                    contentItem: ColumnLayout {
                        spacing: 8

                        ShapeDrawing {

                            Layout.fillWidth: true
                            Layout.preferredHeight: 62

                            shape: choice.wordFor(shapeCell.modelData)
                            picked: shapeCell.marked
                        }

                        Label {
                            Layout.fillWidth: true
                            text: choice.labelFor(shapeCell.modelData)
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            font.pixelSize: 12
                            font.bold: shapeCell.marked
                        }
                    }

                    background: Rectangle {
                        radius: 6
                        color: shapeCell.down
                               ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                                         Material.foreground.b, 0.12)
                               : shapeCell.hovered
                                 ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                                           Material.foreground.b, 0.06)
                                 : "transparent"
                        border.width: shapeCell.marked ? 1 : 0
                        border.color: Material.accent
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true

            text: qsTr("This can be changed at any time in Settings, under Appearance.")
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: 0.6
        }

        Button {
            Layout.fillWidth: true

            highlighted: true

            text: choice.picked === settings.runningLayout

                  ? qsTr("Keep this one", "the arrangement of the windows")

                  : qsTr("Use this one and restart")

            onClicked: choice.settle()
        }
    }

    function labelFor(value) {
        void Qt.uiLanguage
        return settingsWords.labelFor(value)
    }

    function wordFor(value) {
        return settingsWords.wordFor(value)
    }

    function settle() {
        windows.rememberNotice(choice.noticeKey)
        settings.chooseLayout(choice.picked)
        choice.away()
        if (choice.picked !== settings.runningLayout) {
            settings.applyTheLayout()
        }
    }

    readonly property string noticeKey: "layout-1"

    ShapeWords {
        id: settingsWords
    }
}
