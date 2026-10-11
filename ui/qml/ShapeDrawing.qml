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
import QtQuick.Controls.Material

Item {
    id: drawing

    required property string shape

    property bool picked: false

    implicitWidth: 96
    implicitHeight: 68

    readonly property real ratio: 96 / 68

    Item {
        id: canvas

        anchors.centerIn: parent
        width: Math.round(Math.min(drawing.width, drawing.height * drawing.ratio))
        height: Math.round(canvas.width / drawing.ratio)

        Repeater {
            model: drawing.parts

            Rectangle {
                required property var modelData

                x: Math.round(modelData.x * canvas.width)
                y: Math.round(modelData.y * canvas.height)
                width: Math.round(modelData.w * canvas.width)
                height: Math.round(modelData.h * canvas.height)
                radius: 2

                color: modelData.fill
                       ? (drawing.picked
                          ? Material.accent
                          : Qt.rgba(Material.foreground.r, Material.foreground.g,
                                    Material.foreground.b, 0.32))
                       : "transparent"
                border.width: modelData.fill ? 0 : 1
                border.color: drawing.picked
                              ? Material.accent
                              : Qt.rgba(Material.foreground.r, Material.foreground.g,
                                        Material.foreground.b, 0.32)
            }
        }
    }

    readonly property var parts: {
        switch (drawing.shape) {
        case "windows":

            return [{ x: 0.00, y: 0.05, w: 0.22, h: 0.90, fill: true },
                    { x: 0.26, y: 0.05, w: 0.24, h: 0.90, fill: false },
                    { x: 0.52, y: 0.05, w: 0.22, h: 0.90, fill: false },
                    { x: 0.76, y: 0.05, w: 0.24, h: 0.90, fill: false }]
        case "compact":

            return [{ x: 0.00, y: 0.24, w: 0.22, h: 0.56, fill: true },
                    { x: 0.26, y: 0.05, w: 0.72, h: 0.90, fill: false }]
        case "workspace":

            return [{ x: 0.00, y: 0.05, w: 1.00, h: 0.90, fill: false },
                    { x: 0.02, y: 0.12, w: 0.24, h: 0.76, fill: true }]
        case "workspace-and-phone":

            return [{ x: 0.00, y: 0.05, w: 0.30, h: 0.90, fill: true },
                    { x: 0.34, y: 0.05, w: 0.64, h: 0.90, fill: false }]
        case "bar":
            return [{ x: 0.00, y: 0.04, w: 1.00, h: 0.16, fill: true },
                    { x: 0.10, y: 0.30, w: 0.80, h: 0.66, fill: false }]
        case "no-window":

            return [{ x: 0.00, y: 0.40, w: 1.00, h: 0.20, fill: false },
                    { x: 0.62, y: 0.44, w: 0.10, h: 0.12, fill: true }]
        }
        return []
    }
}
