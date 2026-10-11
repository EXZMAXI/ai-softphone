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

RadioButton {
    id: control

    padding: 0
    leftPadding: 4
    rightPadding: 4
    implicitHeight: 30

    indicator: RadioIndicator {
        x: control.text
           ? (control.mirrored ? control.width - width - control.rightPadding
                               : control.leftPadding)
           : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        control: control

        Ripple {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: 20
            height: 20

            z: -1
            anchor: control
            pressed: control.pressed
            active: control.down || control.visualFocus || control.hovered
            color: control.checked ? control.Material.highlightedRippleColor
                                   : control.Material.rippleColor
        }
    }
}
