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

SidePanel {
    id: panel

    required property string windowId

    memoryKey: panel.windowId

    title: body.windowTitle

    readonly property string sectionNow: body.sectionNow

    function showSection(which) {
        body.showSection(which)
    }

    function reveal(control) {
        body.reveal(control)
    }

    signal recordingsRequested(string callId)

    width: body.wantedWidth
    minimumWidth: body.leastWidth

    WindowBody {
        id: body

        anchors.fill: parent

        windowId: panel.windowId

        glyphs: panel.glyphs
        onScreen: panel.visible

        kept: panel.visible || panel.awayWithThePhone
        settling: panel.placing
        focused: panel.active

        onRecordingsRequested: (callId) => panel.recordingsRequested(callId)

        onSectionShown: panel.openBeside(panel.phone)
    }
}
