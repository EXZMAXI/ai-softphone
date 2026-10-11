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

        text: qsTr("Layout")
        font.pixelSize: 15
        font.bold: true
    }

    GridLayout {
        Layout.fillWidth: true

        columns: 3
        columnSpacing: 8
        rowSpacing: 8

        Repeater {

            model: [SettingsViewModel.Layout.WorkspaceAndPhone,
                    SettingsViewModel.Layout.Windows,
                    SettingsViewModel.Layout.Compact,
                    SettingsViewModel.Layout.Workspace,
                    SettingsViewModel.Layout.Bar,
                    SettingsViewModel.Layout.NoWindow]

            ItemDelegate {
                id: shapeChoice

                required property var modelData

                readonly property bool picked: settings.layout === shapeChoice.modelData

                enabled: shapeChoice.modelData !== SettingsViewModel.Layout.NoWindow
                         || page.trayAvailable
                opacity: shapeChoice.enabled ? 1 : 0.4

                Layout.fillWidth: true
                Layout.preferredHeight: 108

                padding: 6

                onClicked: settings.chooseLayout(shapeChoice.modelData)

                contentItem: ColumnLayout {
                    spacing: 6

                    ShapeDrawing {

                        Layout.fillWidth: true
                        Layout.preferredHeight: 62

                        shape: page.layoutWord(shapeChoice.modelData)
                        picked: shapeChoice.picked
                    }

                    Label {
                        Layout.fillWidth: true
                        text: page.layoutLabel(shapeChoice.modelData)
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                        font.bold: shapeChoice.picked
                    }
                }

                background: Rectangle {
                    radius: 6
                    color: shapeChoice.down
                           ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                                     Material.foreground.b, 0.12)
                           : shapeChoice.hovered
                             ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                                       Material.foreground.b, 0.06)
                             : "transparent"
                    border.width: shapeChoice.picked ? 1 : 0
                    border.color: Material.accent
                }
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Every arrangement holds the same windows; what differs is where they stand. The phone closes and opens again to change it.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Button {
        Layout.fillWidth: true

        visible: settings.layoutAwaitsARestart

        enabled: !calls.busy

        text: qsTr("Apply and restart the phone")

        onClicked: applyingTheLayout.open()
    }

    Label {
        Layout.fillWidth: true

        visible: settings.layoutAwaitsARestart && calls.busy

        text: qsTr("Not while a call is in progress.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    ConfirmDialog {
        id: applyingTheLayout

        title: qsTr("Change the layout?")

        actionText: qsTr("Restart")

        destructive: false

        onAccepted: settings.applyTheLayout()

        Label {
            Layout.fillWidth: true
            text: qsTr("The phone will close and open again in the %1 arrangement. Nothing you have set is lost.")
                      .arg(page.layoutLabel(settings.layout))
            wrapMode: Text.WordWrap
        }
    }

    LabelledPicker {
        id: barScreenPicker

        readonly property var screens: Qt.application.screens

        visible: settings.layout === SettingsViewModel.Layout.Bar
                 && barScreenPicker.screens.length > 1

        Layout.fillWidth: true
        placeholderText: qsTr("Screen")

        model: barScreenPicker.screens
        textRole: "name"

        currentIndex: {
            for (let at = 0; at < barScreenPicker.screens.length; ++at) {
                if (barScreenPicker.screens[at].name === settings.barScreen) {
                    return at
                }
            }
            return 0
        }

        onActivated: (chosen) => settings.chooseBarScreen(
                         barScreenPicker.screens[chosen].name)

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: barScreenPicker.width
            text: modelData.name
            highlighted: barScreenPicker.highlightedIndex === index
        }
    }

    Label {
        Layout.fillWidth: true

        visible: barScreenPicker.visible

        text: qsTr("The strip stands along the top of this display.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    LabelledPicker {
        id: languagePicker

        readonly property var entries: [{ code: "", name: "" }]
            .concat(settings.languages)

        Layout.fillWidth: true
        placeholderText: qsTr("Language")
        model: entries
        displayText: page.languageLabel(model[currentIndex])

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: languagePicker.width
            text: page.languageLabel(modelData)
            highlighted: languagePicker.highlightedIndex === index
        }

        currentIndex: page.indexOfLanguage(entries,
                                                     settings.language)
        onActivated: (chosen) => settings.chooseLanguage(
                         languagePicker.entries[chosen].code)

        Connections {
            target: settings
            function onAppearanceChanged() {
                languagePicker.currentIndex
                    = Qt.binding(() => page.indexOfLanguage(
                                     languagePicker.entries,
                                     settings.language))
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("The interface only. What language is spoken on a call is a separate setting, and it arrives with the transcripts.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    LabelledPicker {
        id: themePicker

        readonly property var values: [SettingsViewModel.Theme.Default,
                                       SettingsViewModel.Theme.Light,
                                       SettingsViewModel.Theme.Dark]

        Layout.fillWidth: true
        placeholderText: qsTr("Theme")
        model: values
        displayText: page.themeLabel(model[currentIndex])

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: themePicker.width
            text: page.themeLabel(modelData)
            highlighted: themePicker.highlightedIndex === index
        }

        currentIndex: Math.max(0, values.indexOf(settings.theme))
        onActivated: (chosen) => settings.chooseTheme(themePicker.values[chosen])

        Connections {
            target: settings
            function onAppearanceChanged() {
                themePicker.currentIndex
                    = Qt.binding(() => Math.max(
                                     0, themePicker.values.indexOf(
                                         settings.theme)))
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Both take effect at once, in a call as well as out of one.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 8
        text: qsTr("Icons")
        font.pixelSize: 15
        font.bold: true
    }

    LabelledPicker {
        id: glyphsPicker

        readonly property var values: [SettingsViewModel.Glyphs.Solid,
                                       SettingsViewModel.Glyphs.Outline]

        Layout.fillWidth: true

        placeholderText: qsTr("Weight")
        model: values
        displayText: page.glyphsLabel(model[currentIndex])

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: glyphsPicker.width
            text: page.glyphsLabel(modelData)
            highlighted: glyphsPicker.highlightedIndex === index
        }

        currentIndex: Math.max(0, values.indexOf(settings.glyphs))
        onActivated: (chosen) => settings.chooseGlyphs(
                         glyphsPicker.values[chosen])

        Connections {
            target: settings
            function onAppearanceChanged() {
                glyphsPicker.currentIndex
                    = Qt.binding(() => Math.max(
                                     0, glyphsPicker.values.indexOf(
                                         settings.glyphs)))
            }
        }
    }

    LabelledPicker {
        id: iconScalePicker

        readonly property var values: [SettingsViewModel.IconScale.Normal,
                                       SettingsViewModel.IconScale.Large]

        Layout.fillWidth: true

        placeholderText: qsTr("Size")
        model: values
        displayText: page.iconScaleLabel(model[currentIndex])

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: iconScalePicker.width
            text: page.iconScaleLabel(modelData)
            highlighted: iconScalePicker.highlightedIndex === index
        }

        currentIndex: Math.max(0, values.indexOf(settings.iconScale))
        onActivated: (chosen) => settings.chooseIconScale(
                         iconScalePicker.values[chosen])

        Connections {
            target: settings
            function onAppearanceChanged() {
                iconScalePicker.currentIndex
                    = Qt.binding(() => Math.max(
                                     0, iconScalePicker.values.indexOf(
                                         settings.iconScale)))
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("The pictures on the call buttons. A matter of taste: both weights are the same drawings, and the size changes how large they are drawn rather than which drawing is used.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    LabelledPicker {
        id: sketchPicker

        Layout.topMargin: 8

        readonly property var values: [SettingsViewModel.Sketch.Nothing,
                                       SettingsViewModel.Sketch.Uprights,
                                       SettingsViewModel.Sketch.Levels,
                                       SettingsViewModel.Sketch.Histogram,
                                       SettingsViewModel.Sketch.Wave,
                                       SettingsViewModel.Sketch.Spectrum,
                                       SettingsViewModel.Sketch.Aurora,
                                       SettingsViewModel.Sketch.Halo]

        Layout.fillWidth: true
        placeholderText: qsTr("Call card")
        model: values
        displayText: page.sketchLabel(model[currentIndex])

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: sketchPicker.width
            text: page.sketchLabel(modelData)
            highlighted: sketchPicker.highlightedIndex === index
        }

        currentIndex: Math.max(0, values.indexOf(settings.sketch))
        onActivated: (chosen) => settings.chooseSketch(
                         sketchPicker.values[chosen])

        Connections {
            target: settings
            function onAppearanceChanged() {
                sketchPicker.currentIndex
                    = Qt.binding(() => Math.max(
                                     0, sketchPicker.values.indexOf(
                                         settings.sketch)))
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("The card can show what it is hearing, in both directions.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    LabelledPicker {
        id: noticePicker

        Layout.topMargin: 8

        objectName: "incomingNotice"

        readonly property var values: [SettingsViewModel.Notice.Banner,
                                       SettingsViewModel.Notice.Raise,
                                       SettingsViewModel.Notice.Silent]

        Layout.fillWidth: true
        placeholderText: qsTr("Incoming calls")
        model: values
        displayText: page.callNoticeLabel(model[currentIndex])

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: noticePicker.width
            text: page.callNoticeLabel(modelData)
            highlighted: noticePicker.highlightedIndex === index
        }

        currentIndex: Math.max(0, values.indexOf(settings.callNotice))
        onActivated: (chosen) => settings.chooseCallNotice(
                         noticePicker.values[chosen])

        Connections {
            target: settings
            function onAppearanceChanged() {
                noticePicker.currentIndex
                    = Qt.binding(() => Math.max(
                                     0, noticePicker.values.indexOf(
                                         settings.callNotice)))
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Only when the phone is not already in front of you. The ringtone and the colour of the icon are the telephone rather than a notification, and they happen whatever this says.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 8
        text: qsTr("After a call")
        font.pixelSize: 15
        font.bold: true
    }

    Slider {
        id: lingerSlider

        Layout.fillWidth: true
        from: 0
        to: 10
        stepSize: 1
        snapMode: Slider.SnapAlways
        value: settings.endedCallSeconds

        onPressedChanged: {
            if (!pressed)
                settings.chooseEndedCallSeconds(Math.round(value))
        }

        Connections {
            target: settings
            function onAppearanceChanged() {
                lingerSlider.value = Qt.binding(
                    () => settings.endedCallSeconds)
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: settings.endedCallSeconds === 0
              ? qsTr("A finished call disappears at once and the keypad comes straight back.")
              : qsTr("A finished call stays on screen for %n second(s), long enough to read how it ended, and then the keypad comes back.", "", settings.endedCallSeconds)
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Item { Layout.fillHeight: true }
}
