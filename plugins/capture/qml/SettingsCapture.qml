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
    id: capturePage

    required property var page

    function sideLabel(side) {
        void Qt.uiLanguage
        return side === "near" ? qsTr("You") : qsTr("The other side")
    }

    function sideLevel(side) {
        return side === "near" ? settings.captureNearLevel : settings.captureFarLevel
    }

    function answerLabel(answer) {
        void Qt.uiLanguage
        switch (answer) {
        case SettingsViewModel.OnFinding.Never:
            return qsTr("Never")
        case SettingsViewModel.OnFinding.Record:
            return qsTr("Always")
        default:
            return qsTr("Ask me")
        }
    }

    property int answerRoom: 96

    readonly property string measuredIn: Qt.uiLanguage
    onMeasuredInChanged: capturePage.measureAnswers()
    Component.onCompleted: capturePage.measureAnswers()

    function measureAnswers() {
        let widest = 0
        for (const one of [SettingsViewModel.OnFinding.Never,
                           SettingsViewModel.OnFinding.Ask,
                           SettingsViewModel.OnFinding.Record]) {
            widest = Math.max(widest, Math.ceil(
                findingPicker.widthOfText(capturePage.answerLabel(one))))
        }

        capturePage.answerRoom = Math.max(96, widest)
    }

    Layout.fillWidth: true
    spacing: 12

    Label {
        Layout.fillWidth: true
        text: qsTr("Capture")
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        visible: !settings.captureAvailable
        text: qsTr("This build cannot record other applications on this system yet.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0
        enabled: settings.captureAvailable

        ListCheckBox {
            id: enabledBox

            objectName: "theWayIn"

            Layout.fillWidth: true
            text: qsTr("Allow sound capture")
            checked: settings.captureEnabled
            onToggled: settings.captureEnabled = checked
        }

        ListCheckBox {
            Layout.fillWidth: true
            enabled: enabledBox.checked
            text: qsTr("Remind me to inform the others about recording")
            checked: settings.captureRemind
            onToggled: settings.captureRemind = checked
        }
    }

    LabelledPicker {
        id: findingPicker

        readonly property var values: [SettingsViewModel.OnFinding.Never,
                                       SettingsViewModel.OnFinding.Ask,
                                       SettingsViewModel.OnFinding.Record]

        Layout.fillWidth: true
        placeholderText: qsTr("Automatic start")

        visible: enabledBox.checked && settings.captureWatchAvailable
        model: values
        displayText: capturePage.answerLabel(model[currentIndex])

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: findingPicker.width
            text: capturePage.answerLabel(modelData)
            highlighted: findingPicker.highlightedIndex === index
        }

        currentIndex: Math.max(0, values.indexOf(settings.captureOnFinding))
        onActivated: (chosen) => settings.captureOnFinding = findingPicker.values[chosen]

        Connections {
            target: settings
            function onCaptureChanged() {
                findingPicker.currentIndex = Qt.binding(
                    () => Math.max(0, findingPicker.values.indexOf(settings.captureOnFinding)))
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: enabledBox.checked && settings.captureWatchAvailable
        text: qsTr("What happens when a conversation is heard in another application. "
                   + "Asking loses nothing: the seconds before you answer are already kept.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        visible: settings.captureAvailable && !settings.captureWatchAvailable
        text: qsTr("This build cannot yet see a conversation begin on this system. "
                   + "Recordings you start yourself work as usual.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Everything the computer plays is recorded, not only the conversation. "
                   + "This phone cannot announce a recording into somebody else's meeting, "
                   + "so saying so is yours to do.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        visible: enabledBox.checked
        text: qsTr("Before the start: %n second(s)", "", settings.capturePreRoll)
        wrapMode: Text.WordWrap
    }

    Slider {
        Layout.fillWidth: true
        visible: enabledBox.checked
        from: 0
        to: settings.maxCapturePreRoll
        stepSize: 5
        snapMode: Slider.SnapAlways
        value: settings.capturePreRoll

        onPressedChanged: {
            if (!pressed) {
                settings.capturePreRoll = Math.round(value)
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: enabledBox.checked
        text: qsTr("Seconds kept from before a recording starts, so that nothing "
                   + "is lost while the conversation is being noticed.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    LabelledPicker {
        id: pagePicker

        readonly property var values: [SettingsViewModel.Sketch.Nothing,
                                       SettingsViewModel.Sketch.Uprights,
                                       SettingsViewModel.Sketch.Levels,
                                       SettingsViewModel.Sketch.Histogram,
                                       SettingsViewModel.Sketch.Wave,
                                       SettingsViewModel.Sketch.Spectrum,
                                       SettingsViewModel.Sketch.Aurora,
                                       SettingsViewModel.Sketch.Halo]

        Layout.fillWidth: true

        placeholderText: qsTr("Picture in the window")
        visible: enabledBox.checked && settings.captureAvailable
        model: values
        displayText: capturePage.page.sketchLabel(model[currentIndex])

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: pagePicker.width
            text: capturePage.page.sketchLabel(modelData)
            highlighted: pagePicker.highlightedIndex === index
        }

        currentIndex: Math.max(0, values.indexOf(settings.captureSketch))
        onActivated: (chosen) => settings.chooseCaptureSketch(pagePicker.values[chosen])

        Connections {
            target: settings
            function onCaptureChanged() {
                pagePicker.currentIndex = Qt.binding(
                    () => Math.max(0, pagePicker.values.indexOf(settings.captureSketch)))
            }
        }
    }

    LabelledPicker {
        id: stripPicker

        readonly property var values: settings.sketchesInARow()

        Layout.fillWidth: true

        placeholderText: qsTr("Picture at the foot")
        visible: enabledBox.checked && settings.captureAvailable
        model: values
        displayText: capturePage.page.sketchLabel(model[currentIndex])

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: stripPicker.width
            text: capturePage.page.sketchLabel(modelData)
            highlighted: stripPicker.highlightedIndex === index
        }

        currentIndex: Math.max(0, values.indexOf(settings.captureStripSketch))
        onActivated: (chosen) => settings.chooseCaptureStripSketch(stripPicker.values[chosen])

        Connections {
            target: settings
            function onCaptureChanged() {
                stripPicker.currentIndex = Qt.binding(
                    () => Math.max(0, stripPicker.values.indexOf(settings.captureStripSketch)))
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: enabledBox.checked && settings.captureAvailable

        text: qsTr("Test", "heading over the level meters")
        font.pixelSize: 12
    }

    Repeater {
        model: enabledBox.checked && settings.captureAvailable ? ["near", "far"] : []

        RowLayout {
            required property string modelData

            Layout.fillWidth: true
            spacing: 10

            Label {
                Layout.preferredWidth: 110
                text: capturePage.sideLabel(parent.modelData)
                font.pixelSize: 12
                opacity: 0.6
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 6
                radius: 3
                color: Qt.rgba(Material.foreground.r, Material.foreground.g,
                               Material.foreground.b, 0.14)

                Rectangle {
                    width: parent.width
                           * Math.max(0, Math.min(1, capturePage.sideLevel(
                                          parent.parent.modelData)))
                    height: parent.height
                    radius: parent.radius
                    color: Material.accentColor
                }
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: enabledBox.checked && settings.captureAvailable
        text: qsTr("The upper bar moves when you speak, the lower when something plays.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        visible: exceptionList.count > 0
        text: qsTr("Applications that answer differently")
        font.pixelSize: 12
    }

    Repeater {
        id: exceptionList
        model: settings.captureExceptions

        RowLayout {
            id: exceptionRow

            required property string modelData

            Layout.fillWidth: true
            spacing: 6

            Label {
                Layout.fillWidth: true
                text: exceptionRow.modelData
                elide: Text.ElideRight
            }

            PickerBox {
                id: rowPicker

                readonly property var values: [SettingsViewModel.OnFinding.Never,
                                               SettingsViewModel.OnFinding.Ask,
                                               SettingsViewModel.OnFinding.Record]

                Layout.preferredWidth: capturePage.answerRoom
                model: values
                displayText: capturePage.answerLabel(model[currentIndex])

                delegate: ItemDelegate {
                    required property var modelData
                    required property int index

                    width: rowPicker.width
                    text: capturePage.answerLabel(modelData)
                    highlighted: rowPicker.highlightedIndex === index
                }

                currentIndex: Math.max(
                    0, values.indexOf(settings.capturePolicyFor(exceptionRow.modelData)))
                onActivated: (chosen) => settings.setCapturePolicyFor(
                    exceptionRow.modelData, rowPicker.values[chosen])

                Connections {
                    target: settings
                    function onCaptureChanged() {
                        rowPicker.currentIndex = Qt.binding(
                            () => Math.max(0, rowPicker.values.indexOf(
                                settings.capturePolicyFor(exceptionRow.modelData))))
                    }
                }
            }

            ToolButton {
                id: forgetRow

                icon.source: settings.glyphSet + "/delete.svg"
                icon.width: 18
                icon.height: 18

                Accessible.name: qsTr("Delete")
                onClicked: {
                    forgetAnswer.application = exceptionRow.modelData
                    forgetAnswer.open()
                }

                Hint {
                    owner: forgetRow
                    visible: forgetRow.hovered
                    text: qsTr("Delete")
                }
            }
        }
    }

    ConfirmDialog {
        id: forgetAnswer

        property string application: ""

        title: qsTr("Delete the answer for %1?").arg(forgetAnswer.application)
        actionText: qsTr("Delete")
        destructive: false
        onAccepted: settings.captureFollowsTheDefault(forgetAnswer.application)

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("%1 will follow the default again.").arg(forgetAnswer.application)
        }
    }
}
