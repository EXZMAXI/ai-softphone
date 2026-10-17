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

    property real fullMs: 0
    property real offsetMs: 0
    property real windowMs: 0

    property real headMs: 0

    property color ink: "grey"

    property color tint: "steelblue"

    readonly property int barStep: 3
    readonly property int barWidth: 2

    implicitHeight: 16

    function originPx() {
        if (lane.windowMs <= 0 || drawing.width <= 0) {
            return 0
        }
        return Math.round(lane.offsetMs * drawing.width / lane.windowMs)
    }

    function perBar() {
        if (drawing.width <= 0) {
            return 1
        }
        return lane.barStep * lane.windowMs / drawing.width
    }

    function pictureAs() {
        if (drawing.width <= 0) {
            return ""
        }
        return [drawing.width, drawing.height, lane.originPx(),
                Math.floor(lane.headMs / lane.perBar()), lane.ink, lane.tint].join("/")
    }

    property string paintedAs: ""

    function repaintIfItMoved() {
        if (lane.pictureAs() !== lane.paintedAs) {
            drawing.requestPaint()
        }
    }

    Canvas {
        id: drawing

        anchors.fill: parent
        antialiasing: false

        renderStrategy: Canvas.Immediate

        onPaint: {
            lane.paintedAs = lane.pictureAs()

            const context = getContext("2d")
            context.reset()

            const bars = lane.bars
            if (!bars || bars.length === 0 || width <= 0 || lane.windowMs <= 0
                    || lane.fullMs <= 0) {
                return
            }

            const middle = height / 2
            const perBar = lane.perBar()
            const perBucket = lane.fullMs / bars.length
            const originPx = lane.originPx()
            const first = Math.floor(lane.offsetMs / perBar)
            const last = Math.ceil((lane.offsetMs + lane.windowMs) / perBar)

            for (let bar = first; bar <= last; ++bar) {
                const at = bar * perBar
                if (at < 0 || at > lane.fullMs) {
                    continue
                }

                const from = Math.max(0, Math.min(bars.length - 1,
                                                  Math.floor(at / perBucket)))
                const to = Math.max(from + 1,
                                    Math.min(bars.length, Math.ceil((at + perBar) / perBucket)))
                let loudest = 0
                for (let bucket = from; bucket < to; ++bucket) {
                    if (bars[bucket] > loudest) {
                        loudest = bars[bucket]
                    }
                }

                const half = Math.max(1, loudest * middle)
                const behind = at <= lane.headMs
                context.fillStyle = behind ? lane.tint : lane.ink
                context.globalAlpha = behind ? 0.9 : 1.0
                context.fillRect(bar * lane.barStep - originPx, middle - half,
                                 lane.barWidth, half * 2)
            }
        }
    }

    onOffsetMsChanged: lane.repaintIfItMoved()
    onWindowMsChanged: lane.repaintIfItMoved()
    onHeadMsChanged: lane.repaintIfItMoved()
    onFullMsChanged: lane.repaintIfItMoved()
    onInkChanged: lane.repaintIfItMoved()
    onTintChanged: lane.repaintIfItMoved()

    onBarsChanged: {

        lane.paintedAs = ""
        drawing.requestPaint()
    }
    onWidthChanged: drawing.requestPaint()
    onHeightChanged: drawing.requestPaint()
}
