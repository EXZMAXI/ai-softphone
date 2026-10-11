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

RowButton {
    id: act

    required property var page

    glyphs: act.page.glyphs
    box: act.page.controlTarget
    target: act.page.controlTarget
    circle: act.page.controlCircle
    pictureSize: act.page.rowGlyph

    glyph: "globe"

    property bool offered: settings.apiEnabled && settings.apiPort > 0

    text: qsTr("Open in a browser")

    onClicked: Qt.openUrlExternally("http://127.0.0.1:" + settings.apiPort + "/ui")
}
