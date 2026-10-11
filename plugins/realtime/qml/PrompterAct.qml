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

    required property var page

    text: qsTr("Prompter")

    offered: prompter.offered || prompter.running

    icon.source: settings.glyphSet + "/sparkle.svg"

    onClicked: page.showPanelNamed("prompter")
}
