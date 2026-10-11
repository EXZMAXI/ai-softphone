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

RowLayout {
    id: volumeRow

    required property string label
    required property int percent

    signal moved(int percent)

    Layout.fillWidth: true
    spacing: 8

    Label {
        text: volumeRow.label
        font.pixelSize: 12
        opacity: 0.7
        Layout.preferredWidth: 64
    }

    Slider {
        id: volumeSlider

        Layout.fillWidth: true
        from: 0
        to: 100
        stepSize: 1
        snapMode: Slider.SnapAlways
        value: volumeRow.percent

        onMoved: volumeRow.moved(Math.round(value))
    }

}
