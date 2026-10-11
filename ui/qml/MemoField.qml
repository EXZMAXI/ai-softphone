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

ScrollView {
    id: memo

    property alias text: memoText.text
    property alias placeholderText: memoText.placeholderText
    property alias wrapMode: memoText.wrapMode
    property alias readOnly: memoText.readOnly

    property alias textFont: memoText.font

    property int shortest: 72
    property int tallest: 260

    signal edited(string value)

    Layout.fillWidth: true
    Layout.preferredHeight: Math.max(memo.shortest,
                                     Math.min(memo.tallest, memoText.implicitHeight + 4))
    clip: true

    ScrollBar.vertical: ThinScrollBar { always: true }

    Binding {
        target: memo.contentItem
        property: "interactive"
        value: memo.canScroll
    }

    readonly property bool canScroll: memo.contentItem !== null
                                      && memo.contentItem.contentHeight
                                         > memo.contentItem.height + 1

    WheelHandler {

        readonly property Item outer: {
            let item = memo.parent
            while (item) {
                if (item.contentY !== undefined && item.contentHeight !== undefined) {
                    return item
                }
                item = item.parent
            }
            return null
        }

        onWheel: (event) => {
            const target = memo.canScroll ? memo.contentItem : outer
            if (!target) {
                return
            }
            const most = Math.max(0, target.contentHeight - target.height)
            target.contentY = Math.max(0, Math.min(most, target.contentY - event.angleDelta.y))
        }
    }

    TextArea {
        id: memoText

        wrapMode: TextEdit.Wrap
        onTextChanged: memo.edited(memoText.text)

        TextMenu { field: memoText }

        topPadding: memoText.topInset + memoText.Material.textFieldVerticalPadding
    }
}
