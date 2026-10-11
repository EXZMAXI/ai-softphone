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

import Qt.labs.platform as Platform

import Softphone

ColumnLayout {

    required property var page
    Layout.fillWidth: true
    spacing: 12

    Label {
        Layout.fillWidth: true
        text: qsTr("Audio formats")
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Offered in this order. The other end picks from what you offer.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: settings.codecs

            delegate: RowLayout {
                id: codecRow

                required property var modelData
                required property int index

                Layout.fillWidth: true
                spacing: 4

                ListCheckBox {
                    checked: codecRow.modelData.enabled

                    enabled: !codecRow.modelData.enabled
                             || settings.offeredCodecCount > 1

                    implicitWidth: implicitIndicatorWidth

                    onToggled: settings.offerCodec(codecRow.index, checked)
                }

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: 4
                    text: page.codecLabel(codecRow.modelData)
                    opacity: codecRow.modelData.enabled ? 1.0 : 0.5
                }

                Label {
                    text: qsTr("wideband")
                    visible: codecRow.modelData.wideband
                    font.pixelSize: 11
                    opacity: 0.6
                }

                Button {
                    flat: true
                    text: "▲"
                    Accessible.name: qsTr("Move up")
                    enabled: codecRow.index > 0

                    leftPadding: 0
                    rightPadding: 0
                    topPadding: 0
                    bottomPadding: 0
                    topInset: 0
                    bottomInset: 0
                    implicitWidth: 30
                    implicitHeight: 30

                    onClicked: settings.moveCodec(codecRow.index, -1)
                }

                Button {
                    flat: true
                    text: "▼"
                    Accessible.name: qsTr("Move down")
                    enabled: codecRow.index < settings.codecs.length - 1

                    leftPadding: 0
                    rightPadding: 0
                    topPadding: 0
                    bottomPadding: 0
                    topInset: 0
                    bottomInset: 0
                    implicitWidth: 30
                    implicitHeight: 30

                    onClicked: settings.moveCodec(codecRow.index, 1)
                }
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("These are agreed when a call starts, so a change applies from your next call.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 8
        text: qsTr("Call waiting")
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        Layout.fillWidth: true

        text: qsTr("What happens when somebody rings while you are already on a call. An intercom page from your own switch always comes through — that is how a call placed from a CTI panel reaches this phone.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    PickerBox {
        id: waitingPicker

        readonly property var values: [SettingsViewModel.CallWaiting.Allowed,
                                       SettingsViewModel.CallWaiting.Busy]

        Layout.fillWidth: true
        model: values

        displayText: page.callWaitingLabel(settings.callWaiting)

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: waitingPicker.width
            text: page.callWaitingLabel(modelData)
            highlighted: waitingPicker.highlightedIndex === index
        }

        currentIndex: values.indexOf(settings.callWaiting)
        onActivated: (chosen) => settings.chooseCallWaiting(waitingPicker.values[chosen])
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 8

        text: qsTr("Autodial")
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("When a call cannot get through, the card offers to keep dialling until it does.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Wait between attempts: %n second(s)", "",
                   settings.redialIntervalSeconds)
        wrapMode: Text.WordWrap
    }

    Slider {
        id: redialIntervalSlider

        Layout.fillWidth: true
        from: settings.minRedialIntervalSeconds
        to: settings.maxRedialIntervalSeconds
        stepSize: 5
        snapMode: Slider.SnapAlways
        value: settings.redialIntervalSeconds

        onPressedChanged: {
            if (!pressed) {
                settings.chooseRedialInterval(Math.round(value))
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Give up after: %n minute(s)", "",
                   settings.redialGiveUpMinutes)
        wrapMode: Text.WordWrap
    }

    Slider {
        id: redialGiveUpSlider

        Layout.fillWidth: true
        from: settings.minRedialGiveUpMinutes
        to: settings.maxRedialGiveUpMinutes
        stepSize: 1
        snapMode: Slider.SnapAlways
        value: settings.redialGiveUpMinutes

        onPressedChanged: {
            if (!pressed) {
                settings.chooseRedialGiveUp(Math.round(value))
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("History")
        font.pixelSize: 15
        font.bold: true
        Layout.topMargin: 8
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("A call log is evidence, so nothing is removed from it unless you say so here.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    LabelledPicker {
        id: historyPicker

        readonly property var values: [0, 30, 90, 180, 365, 730]

        Layout.fillWidth: true
        placeholderText: qsTr("Storage period")
        model: values

        displayText: page.keepForLabel(settings.historyDays)

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: historyPicker.width
            text: page.keepForLabel(modelData)
            highlighted: historyPicker.highlightedIndex === index
        }

        currentIndex: values.indexOf(settings.historyDays)
        onActivated: (chosen) => settings.chooseHistoryDays(historyPicker.values[chosen])
    }

    Button {
        Layout.fillWidth: true
        text: qsTr("Clear the call history")
        enabled: history.available && history.shown > 0
        onClicked: clearHistory.open()
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Deletes every call at once, whatever the period above says.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    ConfirmDialog {
        id: clearHistory

        title: qsTr("Clear the call history?")
        actionText: qsTr("Clear")
        onAccepted: history.clear()

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Every call is deleted. Nothing else is affected.")
        }
    }

    Item { Layout.fillHeight: true }
}
