// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

import QtQuick.Window
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ComboBox {
    id: box

    property int visibleRows: 10

    selectTextByMouse: true

    property string picture: ""

    readonly property int pictureRoom: box.picture === "" ? 0 : 16 + 8

    Binding {
        target: box.contentItem
        property: "leftPadding"
        value: 16 + box.pictureRoom
        when: box.contentItem !== null && box.picture !== ""
        restoreMode: Binding.RestoreBindingOrValue
    }

    Glyph {
        visible: box.picture !== ""
        source: box.picture
        width: 16
        height: 16
        x: 16
        anchors.verticalCenter: parent.verticalCenter
        opacity: box.enabled ? 1 : 0.5
    }

    TextMetrics {
        id: shown

        font: box.font
        text: box.displayText
    }

    readonly property real insideLeft: contentItem !== null ? (contentItem.leftPadding || 0) : 0
    readonly property real insideRight: contentItem !== null ? (contentItem.rightPadding || 0) : 0
    readonly property real arrowRoom: indicator !== null ? indicator.width : 0

    implicitWidth: box.widthFor(shown.advanceWidth)

    function widthFor(advance) {
        return Math.max(implicitBackgroundWidth + leftInset + rightInset,
                        Math.ceil(advance) + 2 + leftPadding + insideLeft
                            + Math.max(rightPadding, insideRight + arrowRoom))
    }

    function widthOfText(words) {
        measuring.text = words
        return box.widthFor(measuring.advanceWidth)
    }

    TextMetrics {
        id: measuring
        font: box.font
    }

    property real popupWidth: width

    popup: Popup {
        id: list_popup

        y: box.height
        width: box.popupWidth

        onAboutToShow: {
            const inWindow = box.mapToItem(null, 0, 0).x
            const room = box.Window.width - kEdge - width - inWindow
            x = Math.max(kEdge - inWindow, Math.min(0, room))
        }

        readonly property real kEdge: 8
        implicitHeight: Math.min(list.contentHeight, box.visibleRows * rowHeight) +
                        topPadding + bottomPadding
        padding: 1

        readonly property real rowHeight: list.count > 0 && list.contentHeight > 0
                                          ? list.contentHeight / list.count
                                          : 48

        contentItem: ListView {
            id: list

            clip: true
            implicitHeight: contentHeight
            model: box.popup.visible ? box.delegateModel : null
            currentIndex: box.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds

            onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)

            ScrollBar.vertical: ThinScrollBar {}
        }

        background: Rectangle {
            radius: 4
            color: box.Material.dialogColor
            border.width: 1
            border.color: Qt.rgba(box.Material.foreground.r, box.Material.foreground.g,
                                  box.Material.foreground.b, 0.12)
        }
    }
}
