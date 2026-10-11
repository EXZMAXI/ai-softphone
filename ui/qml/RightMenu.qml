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
import QtQuick.Window

Menu {
    id: rightMenu

    required property Item host

    readonly property Window inWindow: rightMenu.host ? rightMenu.host.Window.window : null

    popupType: settings.runningLayout === SettingsViewModel.Layout.Bar
               ? Popup.Window : Popup.Item

    verticalPadding: 4

    implicitHeight: contentHeight + topPadding + bottomPadding

    function present(under) {
        rightMenu.fitToEntries()

        OneMenu.opening(rightMenu)

        if (under) {
            rightMenu.popup(under, 0, under.height)
        } else {
            rightMenu.popup()
        }
    }

    function fitToEntries() {
        let widest = 0
        for (let at = 0; at < rightMenu.count; ++at) {
            const entry = rightMenu.itemAt(at)
            if (entry) {
                widest = Math.max(widest, entry.implicitWidth)
            }
        }
        rightMenu.contentWidth = widest
    }

    onClosed: OneMenu.closed(rightMenu)

    Connections {
        target: rightMenu.visible ? rightMenu.inWindow : null

        function onXChanged() { rightMenu.close() }
        function onYChanged() { rightMenu.close() }
    }

    Connections {
        target: rightMenu.visible ? desktop : null

        function onFocusWindowChanged() {
            if (desktop.focusWindow !== rightMenu.inWindow) {
                rightMenu.close()
            }
        }
    }
}
