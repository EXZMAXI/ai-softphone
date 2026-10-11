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
import QtQuick.Dialogs as Dialogs

import Softphone

ColumnLayout {

    required property var page
    Layout.fillWidth: true
    spacing: 12

    Label {
        Layout.fillWidth: true
        text: qsTr("Recording")
        font.pixelSize: 15
        font.bold: true
    }

    PickerBox {
        id: recordingPolicyPicker

        objectName: "theWayIn"

        readonly property var values: [0, 1, 2, 3]

        Layout.fillWidth: true
        model: values
        displayText: page.recordingPolicyLabel(model[currentIndex])

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: recordingPolicyPicker.width
            text: page.recordingPolicyLabel(modelData)
            highlighted: recordingPolicyPicker.highlightedIndex === index
        }

        currentIndex: Math.max(0, values.indexOf(settings.recordingPolicy))
        onActivated: (chosen) => settings.recordingPolicy =
                         recordingPolicyPicker.values[chosen]

        Connections {
            target: settings
            function onRecordingChanged() {
                recordingPolicyPicker.currentIndex
                    = Qt.binding(() => Math.max(
                                     0, recordingPolicyPicker.values.indexOf(
                                         settings.recordingPolicy)))
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: settings.recordingPolicy === 3
        text: qsTr("Lines")
        wrapMode: Text.WordWrap
        Layout.topMargin: 4
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: settings.recordingPolicy === 3 ? accounts.accounts : []

            ListCheckBox {
                required property var modelData

                Layout.fillWidth: true
                text: modelData.label
                checked: settings.recordsAccount(modelData.id)
                onToggled: settings.setRecordsAccount(modelData.id, checked)
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: settings.recordingPolicy === 3
                 && accounts.accounts.length === 0
        text: qsTr("There are no lines yet.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Recording starts when the call is answered and never before, so ringing and the numbers you dial are not in the file.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Consent")
        font.pixelSize: 15
        font.bold: true
        Layout.topMargin: 8
    }

    PickerBox {
        id: recordingConsentPicker

        readonly property var values: [0, 1, 3]

        Layout.fillWidth: true
        model: values
        displayText: page.recordingConsentLabel(model[currentIndex])

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: recordingConsentPicker.width
            text: page.recordingConsentLabel(modelData)
            highlighted: recordingConsentPicker.highlightedIndex === index
        }

        currentIndex: Math.max(0, values.indexOf(settings.recordingConsent))
        onActivated: (chosen) => settings.recordingConsent =
                         recordingConsentPicker.values[chosen]

        Connections {
            target: settings
            function onRecordingChanged() {
                recordingConsentPicker.currentIndex
                    = Qt.binding(() => Math.max(
                                     0, recordingConsentPicker.values.indexOf(
                                         settings.recordingConsent)))
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: settings.recordingSilently
        text: qsTr("In many places — most of Europe, and several American states — recording a conversation without telling the other party is against the law. This is your decision to make.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        color: Ink.warn(Material.theme)
    }

    Label {
        Layout.fillWidth: true
        visible: settings.recordingConsent === 0
        text: settings.recordingAnnouncement.length > 0
              ? settings.recordingAnnouncement
              : qsTr("The shipped chime")
        wrapMode: Text.WrapAnywhere
        elide: Text.ElideMiddle
        opacity: settings.recordingAnnouncement.length > 0 ? 1.0 : 0.6
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 4

        visible: settings.recordingConsent === 0

        Button {
            Layout.fillWidth: true
            visible: settings.recordingConsent === 0
            text: qsTr("Choose…")
            onClicked: announcementDialog.open()
        }

        Button {
            Layout.fillWidth: true
            visible: settings.recordingConsent === 0
                     && settings.recordingAnnouncement.length > 0
            flat: true
            text: qsTr("Restore")
            onClicked: settings.useTheShippedChime()
        }
    }

    Dialogs.FileDialog {
        id: announcementDialog

        title: qsTr("Choose the announcement")
        nameFilters: [qsTr("Sound files (*.wav)"), qsTr("All files (*)")]
        onAccepted: settings.chooseAnnouncementFile(file)
    }

    Label {
        Layout.fillWidth: true
        visible: settings.recordingConsent === 0
        text: qsTr("With nothing chosen the phone plays a short chime.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        visible: settings.recordingConsent === 1
        text: qsTr("Tone interval: %n second(s)", "",
                   settings.recordingToneInterval)
        wrapMode: Text.WordWrap
    }

    Slider {
        Layout.fillWidth: true
        visible: settings.recordingConsent === 1
        from: settings.recordingToneMinimum
        to: settings.recordingToneMaximum
        stepSize: 5
        snapMode: Slider.SnapAlways
        value: settings.recordingToneInterval

        onPressedChanged: {
            if (!pressed) {
                settings.recordingToneInterval = Math.round(value)
            }
        }
    }

    CheckBox {
        Layout.fillWidth: true
        visible: settings.recordingConsent !== 3
        text: qsTr("Keep the notice in the recording")
        checked: settings.recordingKeepNotices
        onToggled: settings.setRecordingKeepNotices(checked)
    }

    Label {
        Layout.fillWidth: true
        visible: settings.recordingConsent !== 3
        text: qsTr("The announcement and the tone are played to the people on the call. Turn this on and they are in the file as well.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Conferences")
        font.pixelSize: 15
        font.bold: true
        Layout.topMargin: 8
    }

    CheckBox {
        Layout.fillWidth: true
        text: qsTr("A file for each person")
        checked: settings.recordingKeepStems
        onToggled: settings.setRecordingKeepStems(checked)
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("A call is one stereo file: you on one channel and everybody else on the other. In a conference the second channel is a mix, so an extra file per person is what makes a transcript able to say who said what.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Retention")
        font.pixelSize: 15
        font.bold: true
        Layout.topMargin: 8
    }

    LabelledPicker {
        id: keepForPicker

        readonly property var values: [0, 7, 30, 90, 180, 365, 730]

        Layout.fillWidth: true
        placeholderText: qsTr("Storage period")
        model: values

        displayText: page.keepForLabel(settings.recordingMaxAgeDays)

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: keepForPicker.width
            text: page.keepForLabel(modelData)
            highlighted: keepForPicker.highlightedIndex === index
        }

        currentIndex: values.indexOf(settings.recordingMaxAgeDays)
        onActivated: (chosen) => settings.recordingMaxAgeDays =
                         keepForPicker.values[chosen]
    }

    LabelledPicker {
        id: librarySizePicker

        readonly property var values: [0, 512, 1024, 5120, 10240, 51200,
                                       102400]

        Layout.fillWidth: true
        placeholderText: qsTr("Storage limit")
        model: values
        displayText: page.sizeLabel(settings.recordingMaxSize)

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: librarySizePicker.width
            text: page.sizeLabel(modelData)
            highlighted: librarySizePicker.highlightedIndex === index
        }

        currentIndex: values.indexOf(settings.recordingMaxSize)
        onActivated: (chosen) => settings.recordingMaxSize =
                         librarySizePicker.values[chosen]
    }

    LabelledPicker {
        id: stemAgePicker

        readonly property var values: [0, 7, 30, 90, 180, 365]

        Layout.fillWidth: true
        placeholderText: qsTr("Per-person files")
        visible: settings.recordingKeepStems
        model: values
        displayText: page.keepForLabel(settings.recordingStemDays)

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: stemAgePicker.width
            text: page.keepForLabel(modelData)
            highlighted: stemAgePicker.highlightedIndex === index
        }

        currentIndex: values.indexOf(settings.recordingStemDays)
        onActivated: (chosen) => settings.recordingStemDays =
                         stemAgePicker.values[chosen]
    }

    LabelledPicker {
        id: floorPicker

        readonly property var values: [200, 500, 1024, 5120, 10240]

        Layout.fillWidth: true
        placeholderText: qsTr("Disk space threshold")
        model: values
        displayText:
            page.sizeLabel(settings.recordingFreeSpaceFloor)

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: floorPicker.width
            text: page.sizeLabel(modelData)
            highlighted: floorPicker.highlightedIndex === index
        }

        currentIndex: values.indexOf(settings.recordingFreeSpaceFloor)
        onActivated: (chosen) => settings.recordingFreeSpaceFloor =
                         floorPicker.values[chosen]
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("A recording you have pinned is never deleted by any of these, and it still counts towards the limit. An hour of conversation is about 30 MB.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Item { Layout.fillHeight: true }
}
