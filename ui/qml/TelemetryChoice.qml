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
import QtQuick.Layouts

import Softphone

ColumnLayout {
    id: choice

    property bool explainOff: true

    spacing: 12

    Label {
        Layout.fillWidth: true
        text: qsTr("Always sent")
        font.weight: Font.Medium
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: telemetry.always
            delegate: Label {
                required property string modelData
                Layout.fillWidth: true
                text: "· " + modelData
                wrapMode: Text.WordWrap
                opacity: 0.75
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Sent as well, in Extended mode")
        font.weight: Font.Medium
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: telemetry.extended
            delegate: Label {
                required property string modelData
                Layout.fillWidth: true
                text: "· " + modelData
                wrapMode: Text.WordWrap
                opacity: 0.75
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Never sent, in any mode")
        font.weight: Font.Medium
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: telemetry.never
            delegate: Label {
                required property string modelData
                Layout.fillWidth: true
                text: "· " + modelData
                wrapMode: Text.WordWrap
                opacity: 0.75
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: telemetry.identity
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.7
    }

    Label {
        Layout.fillWidth: true
        text: telemetry.lawful
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.7
    }

    Label {
        Layout.fillWidth: true

        text: qsTr("Reporting")
        font.weight: Font.Medium
    }

    ColumnLayout {

        objectName: "reportingChoice"

        Layout.fillWidth: true
        spacing: 0

        ListRadioButton {
            Layout.fillWidth: true
            text: qsTr("Extended")
            checked: telemetry.mode === "extended"
            onToggled: if (checked) telemetry.chooseMode("extended")
        }

        ListRadioButton {
            Layout.fillWidth: true
            text: qsTr("Basic")
            checked: telemetry.mode === "basic"
            onToggled: if (checked) telemetry.chooseMode("basic")
        }

        ListRadioButton {
            id: offEntry

            Layout.fillWidth: true
            text: qsTr("Disabled")
            enabled: telemetry.offAvailable
            checked: telemetry.mode === "off"
            onToggled: if (checked) telemetry.chooseMode("off")

            Hint {
                owner: offEntry
                visible: offEntry.hovered && !telemetry.offAvailable

                text: qsTr("Available in the Enterprise edition")
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: choice.explainOff && !telemetry.offAvailable

        text: qsTr("Turning reporting off altogether is available in the Enterprise edition.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.7
    }
}
