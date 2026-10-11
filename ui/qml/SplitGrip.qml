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

Item {
    id: grip

    required property real sizeNow

    required property color hairline

    signal moved(real size)

    implicitHeight: 9

    Rectangle {
        anchors.centerIn: parent
        width: parent.width
        height: 1
        color: grip.hairline
    }

    HoverHandler {
        cursorShape: Qt.SizeVerCursor
    }

    DragHandler {
        id: dragging

        target: null
        xAxis.enabled: false
        cursorShape: Qt.SizeVerCursor

        property real began: 0

        onActiveChanged: if (active) { dragging.began = grip.sizeNow }

        onTranslationChanged: {
            if (dragging.active) {
                grip.moved(Math.max(1, dragging.began + dragging.translation.y))
            }
        }
    }
}
