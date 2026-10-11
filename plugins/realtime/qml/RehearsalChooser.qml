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

Popup {
    id: chooser

    required property Item host

    required property string glyphs

    property var rows: []

    property string empty: ""

    property string fileEntry: ""

    signal chose(string id)

    signal wantsAFile()

    readonly property Window inWindow: chooser.host ? chooser.host.Window.window : null

    readonly property int mostRows: 10

    readonly property int rowHeight: 32

    readonly property int edge: 8

    readonly property bool ownWindow: chooser.popupType === Popup.Window

    readonly property rect room: {
        if (!chooser.inWindow) {
            return Qt.rect(0, 0, 440, 392)
        }
        if (!chooser.ownWindow) {
            return Qt.rect(0, 0, chooser.inWindow.width, chooser.inWindow.height)
        }

        const screen = chooser.inWindow.screen
        if (!screen) {
            return Qt.rect(0, 0, chooser.inWindow.width, chooser.inWindow.height)
        }
        return Qt.rect(screen.virtualX - chooser.inWindow.x,
                       screen.virtualY - chooser.inWindow.y, screen.width, screen.height)
    }

    popupType: settings.runningLayout === SettingsViewModel.Layout.Bar
               ? Popup.Window : Popup.Item

    padding: 4
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside

    width: Math.min(440, chooser.room.width - 2 * chooser.edge)
    height: Math.min(implicitHeight,
                     chooser.mostRows * chooser.rowHeight + 2 * chooser.rowHeight + 2 * padding,
                     chooser.room.height - 2 * chooser.edge)
    implicitHeight: body.implicitHeight + 2 * padding

    Material.elevation: 4
    Material.roundedScale: Material.ExtraSmallScale

    ColumnLayout {
        id: body

        anchors.fill: parent
        spacing: 0

        ItemDelegate {
            Layout.fillWidth: true
            implicitHeight: chooser.rowHeight
            horizontalPadding: 12
            verticalPadding: 6
            text: chooser.fileEntry
            icon.source: chooser.glyphs === "" ? "" : chooser.glyphs + "/folder-open.svg"
            icon.width: 18
            icon.height: 18
            onClicked: {
                chooser.close()
                chooser.wantsAFile()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            implicitHeight: 1
            color: Material.foreground
            opacity: 0.12
            visible: chooser.rows.length > 0
        }

        ListView {
            id: list

            Layout.fillWidth: true
            Layout.fillHeight: true
            implicitHeight: Math.min(chooser.mostRows, chooser.rows.length) * chooser.rowHeight
            visible: chooser.rows.length > 0
            clip: true
            model: chooser.rows
            currentIndex: -1
            focus: true

            ScrollBar.vertical: ThinScrollBar {
                always: true
            }

            Keys.onReturnPressed: list.chooseCurrent()
            Keys.onEnterPressed: list.chooseCurrent()

            function chooseCurrent() {
                if (list.currentIndex < 0 || list.currentIndex >= chooser.rows.length) {
                    return
                }
                const row = chooser.rows[list.currentIndex]
                chooser.close()
                chooser.chose(row.id ?? "")
            }

            delegate: ItemDelegate {
                id: entry

                required property var modelData
                required property int index

                width: ListView.view.width
                implicitHeight: chooser.rowHeight
                horizontalPadding: 12
                verticalPadding: 6
                highlighted: ListView.isCurrentItem

                text: entry.modelData.label ?? ""

                icon.source: chooser.glyphs === ""
                             ? ""
                             : chooser.glyphs + "/"
                               + RecordingGlyph.forRecording(entry.modelData) + ".svg"
                icon.width: 18
                icon.height: 18

                onClicked: {
                    chooser.close()
                    chooser.chose(entry.modelData.id ?? "")
                }
                onHoveredChanged: if (hovered) list.currentIndex = entry.index
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.margins: 12
            visible: chooser.rows.length === 0
            text: chooser.empty
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: 0.6
        }
    }

    function present(under) {

        OneMenu.opening(chooser)

        if (under) {
            chooser.opener = under
            chooser.place()
        }
        chooser.open()
        list.forceActiveFocus()
    }

    property Item opener: null

    function place() {
        const under = chooser.opener
        if (!under || !chooser.inWindow) {
            return
        }

        const corner = under.mapToItem(null, 0, 0)
        const room = chooser.room
        const least = chooser.edge
        const rightmost = room.x + room.width - chooser.width - least
        const lowest = room.y + room.height - chooser.height - least

        let x = corner.x
        if (x > rightmost) {
            x = corner.x + under.width - chooser.width
        }
        x = Math.max(room.x + least, Math.min(x, rightmost))

        let y = corner.y + under.height
        if (y > lowest) {
            const above = corner.y - chooser.height
            y = above >= room.y + least ? above : Math.max(room.y + least, lowest)
        }

        chooser.parent = under
        chooser.x = x - corner.x
        chooser.y = y - corner.y
    }

    onClosed: OneMenu.closed(chooser)

    Connections {
        target: chooser.visible ? chooser.inWindow : null

        function onXChanged() { chooser.close() }
        function onYChanged() { chooser.close() }

        function onWidthChanged() { chooser.place() }
        function onHeightChanged() { chooser.place() }
    }
}
