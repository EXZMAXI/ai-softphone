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

import Softphone

ToolbarAct {
    id: historyAct

    required property var page

    offered: page.hasWindow("people")
             && page.panelHasTab("people", "softphone.history")

    text: qsTr("History")

    icon.source: settings.glyphSet + "/history.svg"

    marked: history.unseenMissed

    onMarkedChanged: uiRegistry.mark("softphone.history", historyAct.marked)
    Component.onCompleted: uiRegistry.mark("softphone.history", historyAct.marked)

    Component.onDestruction: uiRegistry.mark("softphone.history", false)

    onClicked: page.showPanelSection("people", "softphone.history")
}
