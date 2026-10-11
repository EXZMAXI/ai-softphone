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

import Softphone

Item {
    id: body

    required property string windowId

    property string glyphs: ""
    property bool onScreen: false
    property bool kept: false
    property bool settling: false
    property bool focused: false

    signal recordingsRequested(string callId)

    signal sectionShown

    readonly property int wantedWidth: bodyLoader.item ? bodyLoader.item.implicitWidth : 560
    readonly property int leastWidth: bodyLoader.item ? bodyLoader.item.leastWidth : 460

    readonly property string windowTitle: {
        void uiRegistry.revision
        if (bodyLoader.item && bodyLoader.item.windowTitle !== undefined) {
            return bodyLoader.item.windowTitle
        }
        return uiRegistry.windowTitle(body.windowId)
    }

    readonly property var item: bodyLoader.item

    function reveal(control) {
        spotlight.reveal(control)
    }

    Spotlight {
        id: spotlight

        anchors.fill: parent
        z: 10

        inside: body
        attempts: 25
    }

    property bool ownStrip: true

    readonly property var tabsHere:
        bodyLoader.item && bodyLoader.item.tabIds !== undefined ? bodyLoader.item.tabIds : []

    function titleOfTab(id) {
        return bodyLoader.item && bodyLoader.item.titleOfTab !== undefined
               ? bodyLoader.item.titleOfTab(id) : ""
    }

    function glyphOfTab(id) {
        if (bodyLoader.item && bodyLoader.item.glyphOfTab !== undefined) {
            return bodyLoader.item.glyphOfTab(id)
        }
        return body.drawing
    }

    readonly property string drawing:
        bodyLoader.item && bodyLoader.item.drawing !== undefined
        ? bodyLoader.item.drawing : ""

    readonly property string sectionNow:
        bodyLoader.item && bodyLoader.item.section !== undefined ? bodyLoader.item.section : ""

    function showSection(which) {
        if (bodyLoader.item && bodyLoader.item.showSection !== undefined) {
            bodyLoader.item.showSection(which)
        }
    }

    implicitWidth: body.wantedWidth

    Loader {
        id: bodyLoader

        anchors.fill: parent

        readonly property url wanted: {
            void uiRegistry.slotRevision
            const where = uiRegistry.componentOfWindow(body.windowId)
            return where == "" ? "TabbedBody.qml" : where
        }

        source: bodyLoader.wanted

        onLoaded: {

            if (bodyLoader.item.windowId !== undefined) {
                bodyLoader.item.windowId = Qt.binding(() => body.windowId)
            }

            bodyLoader.item.glyphs = Qt.binding(() => body.glyphs)

            if (bodyLoader.item.ownStrip !== undefined) {
                bodyLoader.item.ownStrip = Qt.binding(() => body.ownStrip)
            }
            bodyLoader.item.onScreen = Qt.binding(() => body.onScreen)
            bodyLoader.item.kept = Qt.binding(() => body.kept)
            bodyLoader.item.settling = Qt.binding(() => body.settling)
            bodyLoader.item.focused = Qt.binding(() => body.focused)

            if (bodyLoader.item.recordingsRequested !== undefined) {
                bodyLoader.item.recordingsRequested.connect(body.recordingsRequested)
            }
            if (bodyLoader.item.sectionShown !== undefined) {
                bodyLoader.item.sectionShown.connect(body.sectionShown)
            }
        }

        property int attempts: 0

        onStatusChanged: {
            if (bodyLoader.status !== Loader.Error) {
                return
            }

            if (bodyLoader.attempts < 3) {
                bodyLoader.attempts += 1
                console.warn("the body of", body.windowId,
                             "was interrupted while it was being built; asking again, attempt",
                             bodyLoader.attempts)

                Qt.callLater(bodyLoader.buildAgain)
                return
            }

            console.warn("the body of", body.windowId, "did not load:", bodyLoader.source)
        }

        function buildAgain() {
            bodyLoader.source = ""
            bodyLoader.source = Qt.binding(() => bodyLoader.wanted)
        }
    }
}
