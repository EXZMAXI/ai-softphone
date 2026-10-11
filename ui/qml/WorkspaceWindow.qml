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

SidePanel {
    id: window

    readonly property var zones: theZones

    memoryKey: "workspace"

    title: window.phone.title

    width: 900
    minimumWidth: 640

    follows: !window.phone.isTheBar && !window.phone.hasNoWindow

    height: 640

    minimumHeight: 420

    function showAgain() {
        if (!window.visible) {
            window.openBeside(window.phone)
        }
        window.raise()
        window.requestActivate()
    }

    WorkspaceZones {
        id: theZones

        anchors.fill: parent

        page: window.phone
        active: window.active
    }
}
