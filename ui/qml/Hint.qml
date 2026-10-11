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
import QtQuick.Window

ToolTip {
    id: hint

    required property Item owner

    readonly property int drop: 2

    property real pointerX: -1

    HoverHandler {
        parent: hint.owner
        onPointChanged: hint.pointerX = point.position.x
    }

    readonly property Window inWindow: hint.owner !== null ? hint.owner.Window.window : null

    readonly property bool cramped: hint.inWindow !== null && hint.owner !== null
                                    && hint.inWindow.height - hint.owner.mapToItem(null, 0, 0).y
                                       - hint.owner.height - hint.drop < hint.implicitHeight

    popupType: hint.cramped ? Popup.Window : Popup.Item

    parent: hint.owner

    width: Math.ceil(hint.implicitWidth)

    delay: 900

    visible: false

    onAboutToShow: hint.stand()
    onOpened: hint.stand()

    onHeightChanged: hint.stand()
    onImplicitHeightChanged: hint.stand()

    onWidthChanged: hint.stand()
    onImplicitWidthChanged: hint.stand()

    function stand() {
        if (hint.owner === null) {
            return
        }

        if (!hint.cramped) {
            hint.y = hint.owner.height + hint.drop
            hint.x = hint.acrossTheOwner(Math.ceil(hint.implicitWidth))
            return
        }

        const tall = Math.max(hint.height, hint.implicitHeight)

        const top = hint.owner.mapToItem(null, 0, 0).y
        const on = hint.inWindow.screen
        const below = on === null
                      || hint.inWindow.y + hint.inWindow.height + hint.drop + tall
                         <= on.virtualY + on.height

        hint.y = below ? hint.inWindow.height - top + hint.drop
                       : -top - hint.drop - tall

        const wide = Math.max(hint.width, Math.ceil(hint.implicitWidth))
        const left = hint.inWindow.x + hint.owner.mapToItem(null, 0, 0).x
        const centred = left + hint.acrossTheOwner(wide)
        const inside = on === null
                       ? centred
                       : Math.max(on.virtualX + hint.drop,
                                  Math.min(centred, on.virtualX + on.width - wide - hint.drop))

        if (hint.opened) {
            hint.x = Math.round(inside - left)
        }
    }

    function acrossTheOwner(wide) {
        if (hint.owner === null) {
            return 0
        }

        const centred = (hint.owner.width - wide) / 2
        if (hint.pointerX < 0) {
            return centred
        }

        return Math.max(Math.min(hint.pointerX - wide / 2, hint.owner.width - wide), 0)
    }

    Component.onCompleted: Qt.callLater(hint.warmUp)
    onCrampedChanged: Qt.callLater(hint.warmUp)
    onPopupTypeChanged: Qt.callLater(hint.warmUp)

    property bool warmed: false

    function warmUp() {
        if (hint.warmed || !hint.cramped || hint.popupType !== Popup.Window) {
            return
        }

        hint.warmed = true
        hint.open()
        hint.close()
    }
}
