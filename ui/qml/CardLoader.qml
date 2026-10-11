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

import Softphone

Loader {

    required property var settingsPage

    property var about: providers

    required property Component card

    property bool placed: false

    property Item shows: this

    Layout.fillWidth: true

    Layout.preferredHeight: item ? item.implicitHeight : 0

    sourceComponent: card

    onLoaded: placed = false

    onHeightChanged: {
        if (!placed && height > 0) {
            placed = true
            settingsPage.bringIntoView(shows)
            takeTheCursor()
        }
    }

    function takeTheCursor() {
        if (item && item.firstField) {
            item.firstField.forceActiveFocus()
        }
    }
}
