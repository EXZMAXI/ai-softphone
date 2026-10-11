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

Item {
    id: acts

    property bool over: false
    property bool picked: false

    property int fade: 28

    default property alias content: line.data

    function offered() {
        const out = []
        for (const one of line.children) {
            if (one.visible && one.text !== undefined && one.text !== "") {
                out.push(one)
            }
        }
        return out
    }

    readonly property color coat: {
        let under = Material.background
        if (acts.picked) {
            under = acts.laidOver(Material.listHighlightColor, under)
        }
        if (acts.over) {
            under = acts.laidOver(Material.rippleColor, under)
        }
        return under
    }

    function laidOver(top, bottom) {
        const a = top.a
        return Qt.rgba(top.r * a + bottom.r * (1 - a),
                       top.g * a + bottom.g * (1 - a),
                       top.b * a + bottom.b * (1 - a),
                       1)
    }

    z: 1

    implicitWidth: acts.fade + line.implicitWidth
    implicitHeight: line.implicitHeight

    Rectangle {
        x: acts.fade
        width: parent.width - acts.fade
        height: parent.height
        color: acts.coat
    }

    Rectangle {
        width: acts.fade
        height: parent.height

        gradient: Gradient {
            orientation: Gradient.Horizontal

            GradientStop {
                position: 0.0
                color: Qt.rgba(acts.coat.r, acts.coat.g, acts.coat.b, 0)
            }

            GradientStop { position: 1.0; color: acts.coat }
        }
    }

    Row {
        id: line

        x: acts.fade
        spacing: 0
    }
}
