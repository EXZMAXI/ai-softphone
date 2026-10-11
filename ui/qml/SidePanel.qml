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

import Softphone

ApplicationWindow {
    id: panel

    property var phone: panel.transientParent

    property string memoryKey: ""

    property bool follows: true

    property int takenRight: 0
    property int takenLeft: 0

    minimumHeight: panel.phone ? panel.phone.minimumHeight : 640

    property string glyphs: "qrc:/glyphs/regular"

    Component.onCompleted: {

        desktop.learnFrame(panel)

        if (panel.memoryKey === "") {
            return
        }

        if (!panel.follows) {
            panel.standWhereItBelongs()
            return
        }

        const size = windows.restoreSize(panel.memoryKey, panel.minimumWidth, panel.height)
        if (size.width > 0) {
            panel.width = size.width
        }
    }

    Timer {
        id: rememberGeometry

        interval: 500
        onTriggered: {
            if (panel.memoryKey === "") {
                return
            }
            if (panel.follows) {
                windows.rememberSize(panel.memoryKey, panel.width, panel.height)
            } else {
                windows.remember(panel.memoryKey, panel.x, panel.y, panel.width, panel.height)
            }
        }
    }

    onWidthChanged: {
        rememberGeometry.restart()

        if (desktop.pointerHeld()) {
            panel.resizing = true
            settling.restart()
        }

        if (panel.phone && panel.phone.panelsMoved) {
            panel.phone.panelsMoved(panel.resizing ? panel : null)
        }
    }

    property bool resizing: false

    property int phoneX: 0
    property int phoneY: 0

    flags: (panel.follows ? Qt.Dialog : Qt.Window)
           | Qt.WindowTitleHint | Qt.WindowSystemMenuHint
           | Qt.WindowCloseButtonHint

    Material.theme: settings.darkWanted ? Material.Dark : Material.Light
    Material.accent: Material.Teal

    property string stuckTo: ""

    property bool awayWithThePhone: false

    function standWhereItBelongs() {
        if (panel.follows || panel.memoryKey === "") {
            return
        }

        const left = windows.restore(panel.memoryKey, panel.minimumWidth,
                                     panel.minimumHeight)
        const room = panel.phone && panel.phone.roomBesideTheBar
                     ? panel.phone.roomBesideTheBar() : null

        const hereStill = left.width > 0 && panel.phone && panel.phone.ourScreenHolds
                          ? panel.phone.ourScreenHolds(left.x, left.y)
                          : true

        if (left.width > 0 && hereStill) {
            panel.width = left.width
            panel.height = left.height
            panel.standAt(left.x, left.y)
        } else if (room !== null) {
            panel.width = Math.min(left.width > 0 ? left.width : panel.width, room.width)
            panel.height = Math.min(
                Math.max(left.height > 0 ? left.height : panel.height, panel.minimumHeight),
                room.height)
            panel.standAt(Math.round(room.x + (room.width - panel.width) / 2), room.y)
        }
    }

    function openBeside(other) {
        if (!panel.follows) {

            panel.standWhereItBelongs()
            panel.showInPlace()
            panel.raise()
            panel.requestActivate()
            return
        }

        if (!visible && other) {

            const at = desktop.panelPlace(other, phoneX, phoneY, width, takenRight, takenLeft)

            if (at && at.stuck) {
                stuckTo = at.right ? "right" : "left"
            } else if (at) {

                stuckTo = ""
                wanted = Qt.point(at.x, at.y)
                placeKnown = true
            } else {
                stuckTo = "right"
            }

            follow()
        }

        desktop.setTaskbarEntry(panel, false)

        show()
        raise()
        requestActivate()
    }

    readonly property int snapDistance: 16

    property bool placing: false

    function flushPositions() {
        if (!phone) {
            return []
        }

        return [
            { "side": "right",
              "x": desktop.panelStand(phone, phoneX, phoneY, width, takenRight, true),
              "y": phoneY },
            { "side": "left",
              "x": desktop.panelStand(phone, phoneX, phoneY, width, takenLeft, false),
              "y": phoneY }
        ]
    }

    function follow() {
        placeAfterMap()
        standAsTall()
    }

    function standAsTall() {
        if (!phone || !panel.follows) {
            return
        }

        const wantedHeight = Math.max(minimumHeight, phone.height)
        if (height === wantedHeight) {
            return
        }

        height = wantedHeight
    }

    function place() {
        placeThrough(false)
    }

    function placeAfterMap() {
        placeThrough(true)
    }

    function placeThrough(checked) {
        if (!panel.follows) {
            return
        }

        if (stuckTo === "") {
            if (!placeKnown) {
                return
            }

            placing = true
            if (checked) {
                desktop.keepPlace(panel, wanted.x, wanted.y)
            } else {
                desktop.placeNow(panel, wanted.x, wanted.y)
            }
            placed.restart()
            return
        }

        for (const at of flushPositions()) {
            if (at.side !== stuckTo) {
                continue
            }

            wanted = Qt.point(at.x, at.y)
            placeKnown = true

            placing = true

            if (checked) {
                desktop.keepPlace(panel, at.x, at.y)
            } else {
                desktop.placeNow(panel, at.x, at.y)
            }

            placed.restart()
            return
        }
    }

    function matchHeight() {
        if (stuckTo === "") {
            return
        }

        standAsTall()
    }

    Timer {
        id: placed

        interval: 300
        onTriggered: panel.placing = false
    }

    property point wanted: Qt.point(0, 0)
    property bool placeKnown: false

    function standAt(atX, atY) {
        panel.x = atX
        panel.y = atY
        panel.wanted = Qt.point(atX, atY)
        panel.placeKnown = true
    }

    function settle() {

        if (desktop.pointerHeld()) {
            settling.restart()
            return
        }

        wanted = Qt.point(x, y)
        placeKnown = true

        if (resizing) {
            resizing = false

            if (stuckTo !== "") {
                place()

                if (panel.phone && panel.phone.panelsMoved) {
                    panel.phone.panelsMoved(panel)
                }
                return
            }
        }

        for (const at of flushPositions()) {
            if (Math.abs(x - at.x) <= snapDistance && Math.abs(y - at.y) <= snapDistance) {
                stuckTo = at.side

                place()
                return
            }
        }

        stuckTo = ""
    }

    Timer {
        id: settling

        interval: 150
        onTriggered: panel.settle()
    }

    Timer {
        id: replacing

        interval: 150
        onTriggered: {
            if (panel.stuckTo !== "") {
                panel.placeAfterMap()
            } else if (panel.placeKnown) {

                panel.placing = true
                desktop.keepPlace(panel, panel.wanted.x,
                                  panel.wanted.y)
                placed.restart()
            } else {
                panel.placing = false
            }

            desktop.setTaskbarEntry(panel, false)
        }
    }

    function showInPlace() {
        placing = true

        if (!panel.follows && panel.placeKnown) {
            const room = panel.phone && panel.phone.roomBesideTheBar
                         ? panel.phone.roomBesideTheBar() : null
            const frame = desktop.frameOffset()

            if (room !== null && panel.wanted.y < room.y + frame.y) {
                panel.standAt(panel.wanted.x, room.y + frame.y)
            }
        }

        if (stuckTo !== "") {
            placeAfterMap()
        } else if (placeKnown) {
            desktop.keepPlace(panel, wanted.x, wanted.y)
        }

        desktop.setTaskbarEntry(panel, false)

        show()
    }

    onVisibleChanged: {

        placing = true
        settling.stop()

        if (visible) {
            replacing.restart()
            if (panel.phone && panel.phone.panelOpened) {
                panel.phone.panelOpened(panel)
            }
        } else {
            placed.stop()

            if (panel.phone && panel.phone.panelClosed) {
                panel.phone.panelClosed(panel)
            }
        }
    }

    function restick() {
        if (stuckTo !== "") {
            place()
        }
    }

    onXChanged: {
        panel.maybeMoved()
        if (!panel.follows) {
            rememberGeometry.restart()
        }
    }

    onYChanged: {
        panel.maybeMoved()
        if (!panel.follows) {
            rememberGeometry.restart()
        }
    }

    function maybeMoved() {
        if (placing || !visible) {
            return
        }

        if (phone && phone.visibility !== Window.Windowed) {
            return
        }

        settling.restart()
    }

    Connections {
        target: panel.phone

        function onXChanged() { panel.place() }
        function onYChanged() { panel.place() }
        function onWidthChanged() { panel.place() }

        function onHeightChanged() { panel.matchHeight() }
    }

    onClosing: function(close) {
        if (panel.phone && panel.phone.quitting) {
            close.accepted = true
            return
        }

        close.accepted = false
        panel.hide()
    }

}
