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

PickerBox {
    id: picker

    property string placeholderText: ""

    readonly property int effectiveHorizontalAlignment: Text.AlignLeft

    FloatingPlaceholderText {
        id: pickerLabel

        width: picker.width - (picker.leftPadding + picker.rightPadding)
        text: picker.placeholderText
        font: picker.font
        color: picker.enabled && picker.activeFocus ? picker.Material.accentColor
                                                    : picker.Material.hintTextColor
        elide: Text.ElideRight

        filled: false
        verticalPadding: picker.Material.textFieldVerticalPadding
        controlHasActiveFocus: picker.activeFocus

        controlHasText: true
        controlImplicitBackgroundHeight: picker.implicitBackgroundHeight
        controlHeight: picker.height
        leftPadding: picker.leftPadding
        floatingLeftPadding: picker.Material.textFieldHorizontalPadding
    }

    background: MaterialTextContainer {
        implicitWidth: 120
        implicitHeight: picker.Material.textFieldHeight

        filled: false
        outlineColor: (picker.enabled && picker.hovered) ? picker.Material.primaryTextColor
                                                         : picker.Material.hintTextColor
        focusedOutlineColor: picker.Material.accentColor
        placeholderTextWidth: Math.min(pickerLabel.width, pickerLabel.implicitWidth)
                              * pickerLabel.scale
        controlHasActiveFocus: picker.activeFocus
        controlHasText: true
        placeholderHasText: pickerLabel.text.length > 0
        horizontalPadding: picker.Material.textFieldHorizontalPadding
    }
}
