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
    id: lane

    property var bars: []

    property real from: 0
    property real to: 1

    property real played: 0

    property color ink: "grey"

    property color tint: "steelblue"

    readonly property int playedColumn: {
        const span = Math.max(0.000001, lane.to - lane.from)
        const columns = Math.max(1, Math.floor(lane.width / 3))
        return Math.round((lane.played - lane.from) / span * columns)
    }

    implicitHeight: 16

    Canvas {
        id: drawing

        anchors.fill: parent
        antialiasing: false

        onPaint: {
            const context = getContext("2d")
            context.reset()
            const bars = lane.bars
            if (!bars || bars.length === 0 || width <= 0) {
                return
            }

            const middle = height / 2
            const step = 3
            const columns = Math.max(1, Math.floor(width / step))
            const span = Math.max(0.000001, lane.to - lane.from)

            for (let column = 0; column < columns; ++column) {

                const at = lane.from + span * column / columns
                const next = lane.from + span * (column + 1) / columns

                const first = Math.max(0, Math.min(bars.length - 1,
                                                   Math.floor(at * bars.length)))
                const last = Math.max(first + 1,
                                      Math.min(bars.length, Math.ceil(next * bars.length)))
                let loudest = 0
                for (let bucket = first; bucket < last; ++bucket) {
                    loudest = Math.max(loudest, bars[bucket])
                }

                const half = Math.max(1, loudest * middle)
                const behind = at <= lane.played
                context.fillStyle = behind ? lane.tint : lane.ink
                context.globalAlpha = behind ? 0.9 : 1.0
                context.fillRect(column * step, middle - half, 2, half * 2)
            }
        }
    }

    onBarsChanged: drawing.requestPaint()
    onFromChanged: drawing.requestPaint()
    onToChanged: drawing.requestPaint()
    onPlayedColumnChanged: drawing.requestPaint()
    onWidthChanged: drawing.requestPaint()
    onHeightChanged: drawing.requestPaint()
    onInkChanged: drawing.requestPaint()
    onTintChanged: drawing.requestPaint()
}
