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
    id: audioSection

    required property var page

    readonly property bool watching: page.visible
                                     && page.section === "audio"

    onWatchingChanged: settings.watchMicrophone(audioSection.watching)
    Component.onCompleted: if (audioSection.watching)
                               settings.watchMicrophone(true)
    Component.onDestruction: settings.watchMicrophone(false)

    Layout.fillWidth: true
    spacing: 12

    Label {
        Layout.fillWidth: true
        text: qsTr("Microphone")
        font.pixelSize: 15
        font.bold: true
    }

    PickerBox {
        id: microphonePicker

        Layout.fillWidth: true
        model: settings.microphones
        currentIndex: settings.microphoneIndex
        displayText: page.deviceLabel(model[currentIndex])

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: microphonePicker.width
            text: page.deviceLabel(modelData)
            highlighted: microphonePicker.highlightedIndex === index
        }

        onPressedChanged: if (pressed) settings.refreshDevices()

        onActivated: (chosen) => settings.chooseMicrophone(chosen)

        Connections {
            target: settings
            function onAudioChanged() {
                microphonePicker.currentIndex
                    = Qt.binding(() => settings.microphoneIndex)
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: settings.microphoneMissing
        text: qsTr("Your system's microphone is in use until this one is plugged back in.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        color: Ink.warn(Material.theme)
    }

    ProgressBar {
        Layout.fillWidth: true
        from: 0.0
        to: 1.0
        value: settings.microphoneLevel

        Behavior on value {
            NumberAnimation { duration: 60 }
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Say something — this moves when the microphone hears you.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    VolumeRow {
        label: qsTr("Volume")
        percent: settings.microphoneVolume
        onMoved: (percent) => settings.microphoneVolume = percent
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Speakers")
        font.pixelSize: 15
        font.bold: true
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        PickerBox {
            id: speakerPicker

            Layout.fillWidth: true
            model: settings.speakers
            currentIndex: settings.speakerIndex
            displayText: page.deviceLabel(model[currentIndex])

            delegate: ItemDelegate {
                required property var modelData
                required property int index

                width: speakerPicker.width
                text: page.deviceLabel(modelData)
                highlighted: speakerPicker.highlightedIndex === index
            }

            onPressedChanged: if (pressed) settings.refreshDevices()

            onActivated: (chosen) => settings.chooseSpeakers(chosen)

            Connections {
                target: settings
                function onAudioChanged() {
                    speakerPicker.currentIndex
                        = Qt.binding(() => settings.speakerIndex)
                }
            }
        }

        Button {
            id: testSpeakersButton

            text: qsTr("Test")
            enabled: settings.canTest
            onClicked: settings.testSpeakers()

            Hint {
                owner: testSpeakersButton
                visible: testSpeakersButton.hovered && !settings.canTest
                text: qsTr("Not while a call is up")
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: settings.speakersMissing
        text: qsTr("Your system's speakers are in use until these are plugged back in")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        color: Ink.warn(Material.theme)
    }

    VolumeRow {
        label: qsTr("Volume")
        percent: settings.speakerVolume
        onMoved: (percent) => settings.speakerVolume = percent
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Ringtone")
        font.pixelSize: 15
        font.bold: true
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        PickerBox {
            id: ringtonePicker

            Layout.fillWidth: true
            model: settings.ringtones
            currentIndex: settings.ringtoneIndex
            displayText: page.ringtoneLabel(model[currentIndex])

            delegate: ItemDelegate {
                required property var modelData
                required property int index

                width: ringtonePicker.width
                text: page.ringtoneLabel(modelData)
                highlighted: ringtonePicker.highlightedIndex === index
            }

            onActivated: (chosen) => settings.chooseRingtone(chosen)

            Connections {
                target: settings
                function onAudioChanged() {
                    ringtonePicker.currentIndex
                        = Qt.binding(() => settings.ringtoneIndex)
                }
            }
        }

        Button {
            id: browseRingtoneButton

            flat: true
            implicitWidth: implicitHeight
            icon.source: page.glyphs + "/folder-open.svg"
            icon.width: 20
            icon.height: 20
            onClicked: ringtoneDialog.open()

            Hint {
                owner: browseRingtoneButton
                visible: browseRingtoneButton.hovered
                text: qsTr("Ring with a sound of your own")
            }
        }

        Button {
            id: testRingtoneButton

            text: settings.ringPlaying ? qsTr("Stop") : qsTr("Test")
            enabled: settings.canTest || settings.ringPlaying
            onClicked: settings.ringPlaying ? settings.stopTest()
                                            : settings.testRinger()

            Hint {
                owner: testRingtoneButton
                visible: testRingtoneButton.hovered && !settings.canTest
                        && !settings.ringPlaying
                text: qsTr("Not while a call is up")
            }
        }
    }

    Dialogs.FileDialog {
        id: ringtoneDialog

        title: qsTr("Choose a ringtone")
        nameFilters: [qsTr("Sound files (*.wav)"), qsTr("All files (*)")]
        onAccepted: settings.chooseRingtoneFile(file)
    }

    Label {
        Layout.fillWidth: true
        visible: settings.ringtoneProblem !== SettingsViewModel.RingProblem.Playable
        text: page.ringtoneTrouble(settings.ringtoneProblem,
                                             settings.ringtoneProblemFile)
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        color: Ink.warn(Material.theme)
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        PickerBox {
            id: ringerPicker

            readonly property string followsTheSpeakers: qsTr("Same as the speakers")

            Layout.fillWidth: true
            model: settings.ringers
            currentIndex: settings.ringerIndex
            displayText: page.deviceLabel(model[currentIndex],
                                                    followsTheSpeakers)

            delegate: ItemDelegate {
                required property var modelData
                required property int index

                width: ringerPicker.width
                text: page.deviceLabel(modelData,
                                                 ringerPicker.followsTheSpeakers)
                highlighted: ringerPicker.highlightedIndex === index
            }

            onPressedChanged: if (pressed) settings.refreshDevices()

            onActivated: (chosen) => settings.chooseRinger(chosen)

            Connections {
                target: settings
                function onAudioChanged() {
                    ringerPicker.currentIndex
                        = Qt.binding(() => settings.ringerIndex)
                }
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: settings.ringerMissing
        text: qsTr("The ring goes to your speakers until this device is plugged back in.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        color: Ink.warn(Material.theme)
    }

    VolumeRow {
        label: qsTr("Volume")
        percent: settings.ringVolume
        onMoved: (percent) => settings.ringVolume = percent
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Other sounds")
        font.pixelSize: 15
        font.bold: true
    }

    VolumeRow {
        label: qsTr("Volume")
        percent: settings.noticeVolume
        onMoved: (percent) => settings.noticeVolume = percent
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("The tones for a busy number, a call that has ended, and a call answered for you.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Changing any of these takes effect straight away, in a call as well as out of one.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Item { Layout.fillHeight: true }

    Button {
        id: resetAudioButton

        Layout.fillWidth: true
        flat: true
        text: qsTr("Restore defaults")
        enabled: !settings.audioIsDefault
        onClicked: settings.resetAudio()

        Hint {
            owner: resetAudioButton
            visible: resetAudioButton.hovered
            text: qsTr("Back to the system's own devices, the ringtone the phone came with, and the volumes it started at")
        }
    }
}
