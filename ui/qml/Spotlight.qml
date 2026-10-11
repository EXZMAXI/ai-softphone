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
    id: spotlight

    property Item inside: null

    property int attempts: 10
    property int interval: 80

    property real tracks: 0

    signal found(var what)

    function clear() {
        looking.stop()
        ring.target = null
        ring.opacity = 0
    }

    function reveal(name) {
        spotlight.clear()
        ring.wanted = name === undefined || name === null ? "" : name
        ring.tries = 0

        if (ring.wanted !== "") {
            looking.restart()
        }
    }

    Timer {
        id: looking

        interval: spotlight.interval
        repeat: true

        onTriggered: {
            ring.tries += 1

            const was = ItemsCalled.under(spotlight.inside, ring.wanted)
            if (was !== null && was.width > 0 && was.height > 0) {
                looking.stop()
                ring.target = was
                spotlight.found(was)
                pulse.restart()
                return
            }

            if (ring.tries >= spotlight.attempts) {
                looking.stop()
            }
        }
    }

    Rectangle {
        id: ring

        property Item target: null
        property string wanted: ""
        property int tries: 0

        readonly property point at: {
            void spotlight.tracks
            void spotlight.width
            void spotlight.height
            void ring.target
            return ring.target === null
                    ? Qt.point(0, 0)
                    : ring.target.mapToItem(spotlight, 0, 0)
        }

        visible: ring.target !== null && opacity > 0
        opacity: 0

        x: ring.at.x - 6
        y: ring.at.y - 6
        width: ring.target === null ? 0 : ring.target.width + 12
        height: ring.target === null ? 0 : ring.target.height + 12

        radius: 8
        color: "transparent"
        border.width: 2
        border.color: Material.accent

        enabled: false

        SequentialAnimation {
            id: pulse

            loops: 3

            NumberAnimation {
                target: ring
                property: "opacity"
                to: 1
                duration: 180
            }
            NumberAnimation {
                target: ring
                property: "opacity"
                to: 0.15
                duration: 320
            }

            onFinished: ring.opacity = 0
        }
    }
}
