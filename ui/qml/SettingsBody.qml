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

Item {
    id: settingsBody

    implicitWidth: 620

    property int leastWidth: 520

    readonly property string windowTitle: qsTr("Settings")

    property string glyphs: ""

    property bool onScreen: false

    property bool kept: false

    property bool settling: false

    property bool focused: false

    readonly property color linkColour: Material.color(
        Material.Teal, Material.theme === Material.Dark ? Material.Shade200
                                                        : Material.Shade700)

    function actionLabel(action) {
        void Qt.uiLanguage

        switch (action) {
        case SettingsViewModel.Action.Answer:
            return qsTr("Answer a call")
        case SettingsViewModel.Action.HangUp:
            return qsTr("Hang up")
        case SettingsViewModel.Action.HoldResume:
            return qsTr("Hold or resume")
        case SettingsViewModel.Action.Mute:
            return qsTr("Mute the microphone")
        case SettingsViewModel.Action.ShowWindow:
            return qsTr("Show or hide the phone")
        default:
            return ""
        }
    }

    function callNoticeLabel(notice) {
        void Qt.uiLanguage

        switch (notice) {
        case SettingsViewModel.Notice.Raise:
            return qsTr("Bring the phone to the front")
        case SettingsViewModel.Notice.Silent:
            return qsTr("Nothing")
        default:
            return qsTr("A banner by the notification area")
        }
    }

    function ringtoneLabel(entry) {
        void Qt.uiLanguage

        if (entry === undefined || entry === null)
            return ""
        if (entry.own)
            return entry.name

        switch (entry.id) {
        case "classic":
            return qsTr("Classic")
        case "bell":
            return qsTr("Bell")
        case "soft":
            return qsTr("Soft")
        case "digital":
            return qsTr("Digital")
        case "insistent":
            return qsTr("Insistent")
        default:
            return entry.id
        }
    }

    function ringtoneTrouble(problem, name) {
        void Qt.uiLanguage

        switch (problem) {
        case SettingsViewModel.RingProblem.NoSuchFile:
            return qsTr("%1 is not there any more, so the phone rings with its own sound. Choose it again, or pick one of the five.").arg(name)
        case SettingsViewModel.RingProblem.NotAFile:
            return qsTr("%1 is a folder rather than a sound.").arg(name)
        case SettingsViewModel.RingProblem.Locked:
            return qsTr("%1 cannot be read — check who is allowed to open it.").arg(name)
        case SettingsViewModel.RingProblem.NotAWave:
            return qsTr("%1 is not a WAV file. The phone plays WAV; an MP3 or an M4A has to be converted first.").arg(name)
        case SettingsViewModel.RingProblem.WrongFormat:
            return qsTr("%1 is a WAV the phone cannot play. It plays 16-bit or telephone-quality WAV, mono or stereo, at any sample rate.").arg(name)
        case SettingsViewModel.RingProblem.NoAudio:
            return qsTr("%1 has no sound in it.").arg(name)
        default:
            return ""
        }
    }

    function grabText(grab) {
        void Qt.uiLanguage

        switch (grab) {
        case SettingsViewModel.Grab.Unsupported:
            return qsTr("This desktop session does not let an application take a key from the rest of it, so this works only while the phone has the focus.")
        case SettingsViewModel.Grab.Taken:
            return qsTr("Another application already holds this combination. Choose a different one.")
        case SettingsViewModel.Grab.NoSuchKey:
            return qsTr("This keyboard has no such key.")
        default:
            return ""
        }
    }

    function keyProblemText(problem) {
        void Qt.uiLanguage

        switch (problem) {
        case SettingsViewModel.KeyProblem.NoModifier:
            return qsTr("Hold Ctrl, Alt or Super as well — a key on its own is a key you can no longer type.")
        case SettingsViewModel.KeyProblem.Unreadable:
            return qsTr("That is not a combination this keyboard can produce.")
        default:
            return ""
        }
    }

    property bool trayAvailable: false

    property bool trayShown: false

    property string registrationProblem: ""

    property string registrationSummary: ""

    signal diagnosticsRequested()

    signal apiPageRequested(string address)

    signal windowRequested(string id, string control)

    signal phoneRequested()

    function reveal(section, control) {
        settingsBody.section = section
        spotlight.reveal(control)
    }

    function secretNote() {
        void Qt.uiLanguage

        switch (accounts.keyring) {
        case AccountsViewModel.Working:
            return qsTr("It is kept in this computer's keyring, never in a settings file.")
        case AccountsViewModel.Sealed:

            return qsTr("This desktop offers no keyring, so it is kept in an encrypted file on this computer. It cannot be read by another machine or by another installation — and a keyring would still be safer, because a keyring can be locked.")
        case AccountsViewModel.Locked:
            return qsTr("Your system keyring is locked, so it cannot be saved. Unlock it and try again.")
        case AccountsViewModel.Unreadable:

            return qsTr("What this computer has kept cannot be read: it was written by another installation. Type it again to store it afresh.")
        }
        return qsTr("There is nowhere on this computer to keep it, so it cannot be saved. You will have to type it each time.")
    }

    function secretNoteIsPlain() {
        return accounts.keyring === AccountsViewModel.Working
               || accounts.keyring === AccountsViewModel.Sealed
    }

    Spotlight {
        id: spotlight

        anchors.fill: parent
        z: 10

        inside: settingsScroller.contentItem
        tracks: settingsScroller.contentItem.contentY

        onFound: (what) => settingsBody.scrollTo(what)
    }

    function scrollTo(what) {
        const flick = settingsScroller.contentItem
        if (flick === null || what === null) {
            return
        }

        const where = what.mapToItem(flick.contentItem, 0, 0)
        const room = Math.max(0, flick.contentHeight - flick.height)
        flick.contentY = Math.max(0, Math.min(room, where.y - (flick.height - what.height) / 2))
    }

    readonly property alias scroller: settingsScroller
    readonly property alias restoreDictionaries: restoringDictionaries
    readonly property alias restoreEverything: restoringEverything
    readonly property alias clearLog: clearLogDialog

    function confirmDisabling(id, name, going, doingLess) {
        disablePluginDialog.pluginId = id
        disablePluginDialog.pluginName = name
        disablePluginDialog.going = going
        disablePluginDialog.doingLess = doingLess
        disablePluginDialog.open()
    }

    readonly property var sectionKeys: uiRegistry.settingsSections

    property string section: settingsBody.sectionKeys.length > 0
                             ? settingsBody.sectionKeys[0] : ""

    onSectionChanged: settingsScroller.contentItem.contentY = 0

    readonly property bool everyPage: Qt.application.arguments.indexOf("--build-every-page") >= 0

    property Component providerCard: Item {
        id: card

        property Item firstField: providerCardFirstField

        readonly property var page: parent && parent.about ? parent.about : providers

        implicitHeight: providerCardLayout.implicitHeight + 24

        ColumnLayout {
            id: providerCardLayout

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 12
            spacing: 12

            GridLayout {
                id: providerCardGrid

                Layout.fillWidth: true
                columns: card.page.canStream && card.width >= 520 ? 2 : 1
                columnSpacing: 12
                rowSpacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    spacing: 4

                    TextField {
                        id: providerCardFirstField

                        TextMenu {}

                        Layout.fillWidth: true
                        placeholderText: qsTr("Name")
                        text: card.page.name
                        onTextEdited: card.page.setName(text)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    spacing: 4

                    LabelledPicker {
                        id: providerKindPicker

                        readonly property var kinds: card.page.types

                        Layout.fillWidth: true
                        placeholderText: qsTr("Kind")
                        model: kinds.map((one) => one.id)
                        displayText: {
                            const found = kinds.find((one) => one.id === card.page.type)
                            return found ? found.label : ""
                        }
                        currentIndex: Math.max(0, kinds.findIndex(
                                                   (one) => one.id === card.page.type))
                        onActivated: (chosen) => card.page.setType(
                                         providerKindPicker.kinds[chosen].id)

                        delegate: ItemDelegate {
                            id: kindRow

                            required property var modelData
                            required property int index

                            readonly property var about:
                                providerKindPicker.kinds.find(
                                    (one) => one.id === kindRow.modelData)

                            width: providerKindPicker.width
                            highlighted: providerKindPicker.highlightedIndex === kindRow.index

                            contentItem: RowLayout {
                                spacing: 6

                                Label {
                                    Layout.fillWidth: true
                                    text: kindRow.about ? kindRow.about.label
                                                        : kindRow.modelData
                                    elide: Text.ElideRight
                                }

                                Glyph {
                                    width: 14
                                    height: 14
                                    visible: kindRow.about !== undefined
                                             && kindRow.about.transcribes
                                    source: settings.glyphSet + "/clock.svg"
                                    opacity: 0.7
                                }

                                Glyph {
                                    width: 14
                                    height: 14
                                    visible: kindRow.about !== undefined
                                             && kindRow.about.listens
                                    source: settings.glyphSet + "/flash.svg"
                                    opacity: 0.7
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    spacing: 4

                    TextField {

                        TextMenu {}

                        Layout.fillWidth: true
                        placeholderText: card.page.canStream

                                         ? qsTr("Address for transcripts")
                                         : qsTr("Address")
                        text: card.page.endpoint
                        onTextEdited: card.page.setEndpoint(text)
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("For example: %1").arg(card.page.addressExample())
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                        opacity: 0.6
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    visible: card.page.canStream
                    spacing: 4

                    TextField {

                        TextMenu {}

                        Layout.fillWidth: true

                        placeholderText: qsTr("Address for the prompter")
                        text: card.page.realtimeEndpoint
                        onTextEdited: card.page.setRealtimeEndpoint(text)
                    }

                    Label {
                        readonly property string vendor: card.page.liveAddressExample()

                        Layout.fillWidth: true
                        text: card.page.runsHere

                              ? qsTr("Empty is worked out from the address beside it.")
                              : vendor.length > 0

                                ? qsTr("For example: %1").arg(vendor)

                                : qsTr("A prompter needs this filled in.")
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                        opacity: 0.6
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    spacing: 4

                    TextField {
                        id: providerKeyField

                        TextMenu {}

                        Layout.fillWidth: true
                        placeholderText: qsTr("Key")
                        echoMode: TextInput.Password
                        text: card.page.token
                        onTextEdited: card.page.setToken(text)
                    }

                    Label {
                        Layout.fillWidth: true
                        text: settingsBody.secretNote()
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                        color: settingsBody.secretNoteIsPlain()
                               ? Material.foreground : Ink.warn(Material.theme)
                        opacity: settingsBody.secretNoteIsPlain() ? 0.6 : 1.0
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    spacing: 4

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: providerKeyField.implicitHeight
                        topInset: 0
                        bottomInset: 0
                        text: qsTr("Test")
                        enabled: card.page.endpoint.length > 0
                                 && card.page.testState !== "trying"
                        onClicked: card.page.test()
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: card.page.testDetail.length > 0
                        text: card.page.testDetail
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                        color: card.page.testState === "refused"
                               ? Ink.bad(Material.theme)
                               : card.page.testState === "reached"
                                 ? Ink.good(Material.theme)
                                 : Material.foreground
                        opacity: card.page.testState === "trying" ? 0.6 : 1.0
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    spacing: 4

                    LabelledPicker {
                        id: modelPicker

                        Layout.fillWidth: true

                        visible: card.page.needsModel
                        placeholderText: card.page.canStream

                                         ? qsTr("Model for transcripts")
                                         : qsTr("Model")
                        editable: true

                        model: card.page.offeredModels

                        function showTheDraft() {
                            if (modelPicker.editText !== card.page.model) {
                                modelPicker.editText = card.page.model
                            }
                        }

                        Component.onCompleted: modelPicker.editText = card.page.model
                        onModelChanged: Qt.callLater(modelPicker.showTheDraft)

                        onActivated: (chosen) => {
                            card.page.setModel(card.page.offeredModels[chosen])
                            modelPicker.editText = card.page.offeredModels[chosen]
                        }
                        onAccepted: card.page.setModel(modelPicker.editText)

                        Connections {
                            target: modelPicker.contentItem

                            function onTextEdited() {
                                card.page.setModel(modelPicker.contentItem.text)
                            }
                        }

                        Connections {
                            target: card.page

                            function onDraftChanged() {
                                modelPicker.showTheDraft()
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    visible: card.page.canStream
                    spacing: 4

                    LabelledPicker {
                        id: realtimeModelPicker

                        Layout.fillWidth: true

                        placeholderText: qsTr("Model for the prompter")
                        editable: true
                        model: card.page.offeredRealtimeModels

                        function showTheDraft() {
                            if (realtimeModelPicker.editText !== card.page.realtimeModel) {
                                realtimeModelPicker.editText = card.page.realtimeModel
                            }
                        }

                        Component.onCompleted:
                            realtimeModelPicker.editText = card.page.realtimeModel
                        onModelChanged: Qt.callLater(realtimeModelPicker.showTheDraft)

                        onActivated: (chosen) => {
                            card.page.setRealtimeModel(card.page.offeredRealtimeModels[chosen])
                            realtimeModelPicker.editText =
                                card.page.offeredRealtimeModels[chosen]
                        }
                        onAccepted: card.page.setRealtimeModel(realtimeModelPicker.editText)

                        Connections {
                            target: realtimeModelPicker.contentItem

                            function onTextEdited() {
                                card.page.setRealtimeModel(realtimeModelPicker.contentItem.text)
                            }
                        }

                        Connections {
                            target: card.page

                            function onDraftChanged() {
                                realtimeModelPicker.showTheDraft()
                            }
                        }
                    }

                    Label {
                        readonly property string example: card.page.liveModelExample()

                        Layout.fillWidth: true
                        text: example.length > 0

                              ? qsTr("Empty sends no model name. This vendor publishes %1.").arg(example)
                              : qsTr("Empty sends no model name.")
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                        opacity: 0.6
                    }
                }
            }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            ListCheckBox {
                                Layout.fillWidth: true
                                visible: !page.listens

                                text: qsTr("Use as default")
                                checked: page.isDefault
                                onToggled: page.setIsDefault(checked)
                            }

                            ListCheckBox {
                                Layout.fillWidth: true
                                visible: page.listens && page.canTranscribe

                                text: qsTr("Default for transcripts")
                                checked: page.isDefault
                                onToggled: page.setIsDefault(checked)
                            }

                            ListCheckBox {
                                Layout.fillWidth: true
                                visible: page.listens && page.canStream

                                text: qsTr("Default for the prompter")
                                checked: page.isRealtimeDefault
                                onToggled: page.setIsRealtimeDefault(checked)
                            }

                            ListCheckBox {
                                Layout.fillWidth: true

                                text: qsTr("Enabled")
                                checked: page.switchedOn
                                onToggled: page.setSwitchedOn(checked)
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            flat: true
                            text: settingsBody.providerExtrasShown

                                  ? qsTr("Hide advanced settings")

                                  : qsTr("Advanced settings")
                            onClicked: settingsBody.providerExtrasShown =
                                       !settingsBody.providerExtrasShown
                        }

                        TextField {

                            TextMenu {}

                            Layout.fillWidth: true
                            visible: settingsBody.providerExtrasShown
                            placeholderText: qsTr("Region")
                            text: page.region
                            onTextEdited: page.setRegion(text)
                        }

                        Repeater {
                            model: page.knobs

                            delegate: ColumnLayout {
                                id: knobRow

                                required property var modelData

                                visible: settingsBody.providerExtrasShown

                                readonly property string knobName: modelData.name
                                readonly property bool isFlag: modelData.kind === "flag"

                                readonly property bool accepted: modelData.accepted !== false

                                readonly property bool isTemperature:
                                    knobRow.knobName === "temperature"
                                readonly property real ceiling:
                                    knobRow.modelData.ceiling !== undefined
                                    ? knobRow.modelData.ceiling : 2.0

                                Layout.fillWidth: true

                                Layout.fillHeight: false
                                spacing: 2

                                readonly property string title: knobRow.titleOf(knobRow.knobName)
                                readonly property string hint: knobRow.hintOf(knobRow.knobName)

                                function titleOf(name) {

                                    void Qt.uiLanguage
                                    switch (name) {

                                    case "temperature": return qsTr("Temperature")

                                    case "max_output": return qsTr("Answer limit")

                                    case "context_tokens": return qsTr("Context limit")

                                    case "max_concurrent": return qsTr("Requests at once")

                                    case "max_bytes": return qsTr("Upload limit")

                                    case "max_duration": return qsTr("Length limit")

                                    case "turn_gap_ms": return qsTr("Turn gap")

                                    case "operating_point": return qsTr("Operating point")

                                    case "json_mode": return qsTr("Ask for JSON as a parameter")

                                    case "word_timestamps": return qsTr("Ask when each word was said")

                                    case "multichannel": return qsTr("Send the two sides separately")

                                    case "diarisation": return qsTr("Ask who is speaking")

                                    case "format": return qsTr("Write numbers as figures")

                                    case "folder_id": return qsTr("Folder")

                                    case "text_normalization": return qsTr("Write numbers as figures")

                                    case "operations_url": return qsTr("Operations address")

                                    case "turn_idle_ms": return qsTr("End a reply after")

                                    case "turn_join_ms": return qsTr("Join short replies within")
                                    }
                                    return name
                                }

                                function hintOf(name) {

                                    void Qt.uiLanguage
                                    switch (name) {
                                    case "turn_idle_ms":

                                        return qsTr("After this long with no new words, the prompter ends the reply: the only boundary there is on a recogniser that marks none of its own, and a backstop on one that marks them and then stops. Empty uses the prompter's own shorter default.")
                                    case "turn_join_ms":

                                        return qsTr("Some recognisers cut one sentence into several short replies. A short reply waits this long to see whether the next one belongs with it, and the two are shown as one. Empty shows each as it arrives.")
                                    case "temperature":
                                        return qsTr("How much the model may vary its wording. Nought is the same answer to the same conversation every time, which is what a summary somebody may check wants. Leave it empty for a model that will not take one.")
                                    case "max_output":
                                        return knobRow.modelData.clearable

                                            ? qsTr("The most the model may write in one answer. Leave it empty for a model that should decide for itself.")
                                            : qsTr("The most the model may write in one answer. This provider requires it, so it cannot be left empty.")
                                    case "context_tokens":
                                        return qsTr("How much this phone believes the model can read at once. Nothing is told it: this is what a long conversation is folded to fit.")
                                    case "max_concurrent":
                                        return qsTr("How many requests this phone will have in flight at the same time.")
                                    case "max_bytes":
                                        return qsTr("The largest file this phone will send, in bytes. A recording over it is refused before the upload rather than after it.")
                                    case "max_duration":
                                        return qsTr("The longest recording this phone will send, in seconds. Nought is no limit of ours, and whatever the provider allows is what applies.")
                                    case "turn_gap_ms":
                                        return qsTr("This one is the phone's own. A recogniser that hands back words with a speaker on each and no turns leaves somebody to decide how long a silence ends one, and this is that number, in milliseconds.")
                                    case "operating_point":
                                        return qsTr("The provider's own word for how hard to work at it. Leave it empty to use whatever the account is set to.")
                                    case "word_timestamps":
                                        return qsTr("The transcript follows the sound with these. Without them a line can still be read, but nothing lights up as it is played.")
                                    case "multichannel":
                                        return qsTr("A call recorded here has the two people on two channels, and keeping them apart is what tells the recogniser who said what. Turn it off for a server that says it can do this and cannot.")
                                    case "diarisation":
                                        return qsTr("Who said which line. Where the two sides already arrive as two channels, this is what names the people inside one of them.")
                                    case "format":
                                    case "text_normalization":
                                        return qsTr("Sums, dates and phone numbers come back as they are written rather than spelt out as they were said. Turn it off to keep every word as it was spoken.")
                                    case "folder_id":
                                        return qsTr("Only for an IAM token of a personal account, which belongs to no folder. A service account's API key already carries its own, so this can stay empty. Where it is set, a model name without a folder is completed into it.")
                                    case "operations_url":
                                        return qsTr("Where a recognition that has been handed over is asked about until it is finished. Yandex's own address unless your deployment has another.")
                                    }
                                    return ""
                                }

                                Label {
                                    Layout.fillWidth: true
                                    Layout.topMargin: 4
                                    visible: knobRow.isTemperature && knobRow.accepted
                                    text: qsTr("%1: %2").arg(knobRow.title)
                                              .arg(Number(temperatureSlider.value)
                                                       .toLocaleString(Qt.locale(), 'f', 1))
                                    font.pixelSize: 12
                                    opacity: 0.6
                                }

                                Slider {
                                    id: temperatureSlider

                                    Layout.fillWidth: true
                                    visible: knobRow.isTemperature && knobRow.accepted
                                    from: 0
                                    to: knobRow.ceiling
                                    stepSize: 0.1
                                    snapMode: Slider.SnapAlways
                                    value: Number(knobRow.modelData.value)

                                    onPressedChanged: {
                                        if (!pressed) {
                                            page.setKnob(knobRow.knobName,
                                                         value.toFixed(1))
                                        }
                                    }
                                }

                                TextField {

                                    TextMenu {}

                                    Layout.fillWidth: true
                                    visible: !knobRow.isFlag && !knobRow.isTemperature
                                    enabled: knobRow.accepted

                                    placeholderText: knobRow.title
                                    inputMethodHints: knobRow.modelData.kind === "number"
                                                      ? Qt.ImhDigitsOnly
                                                      : Qt.ImhNone
                                    text: knobRow.modelData.value
                                    onTextEdited: page.setKnob(knobRow.knobName, text)
                                }

                                CheckBox {
                                    id: knobTick

                                    Layout.fillWidth: true
                                    visible: knobRow.isFlag
                                    padding: 0
                                    text: knobRow.title
                                    checked: knobRow.modelData.value === "true"
                                    onToggled: page.setKnob(knobRow.knobName,
                                                            checked ? "true" : "false")

                                    contentItem: Label {
                                        text: knobTick.text
                                        wrapMode: Text.WordWrap
                                        verticalAlignment: Text.AlignVCenter
                                        leftPadding: knobTick.indicator.width
                                                     + knobTick.spacing
                                    }
                                    implicitHeight: Math.max(30, contentItem.implicitHeight)
                                }

                                Label {
                                    Layout.fillWidth: true
                                    visible: knobRow.hint !== "" && knobRow.accepted
                                    text: knobRow.hint
                                    wrapMode: Text.WordWrap
                                    font.pixelSize: 12
                                    opacity: 0.6
                                }

                                Label {
                                    Layout.fillWidth: true
                                    visible: !knobRow.accepted

                                    text: qsTr("%1 does not take this one, so nothing is sent.")
                                          .arg(page.model)
                                    wrapMode: Text.WordWrap
                                    font.pixelSize: 12
                                    opacity: 0.6
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: 4
                            visible: card.page.kind !== "llm" && settingsBody.providerExtrasShown
                            spacing: 2

                            TextField {

                                TextMenu {}

                                Layout.fillWidth: true

                                placeholderText: qsTr("Language")
                                text: card.page.language
                                onTextEdited: card.page.setLanguage(text)
                            }

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("A two-letter language code, as in ISO 639-1. Leave it empty and the recogniser decides, which is right unless your calls are in a language it keeps mishearing.")
                                wrapMode: Text.WordWrap
                                font.pixelSize: 12
                                opacity: 0.6
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: 4
                            visible: settingsBody.providerExtrasShown
                            spacing: 2

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Extras")
                                font.pixelSize: 12
                                opacity: 0.6
                            }

                            MemoField {
                                shortest: 88
                                text: card.page.params
                                wrapMode: TextEdit.NoWrap
                                onEdited: (value) => card.page.setParams(value)
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: settingsBody.providerExtrasShown

                            text: card.page.kind === "llm"
                                  ? qsTr("One name = value a line — top_p = 0.9, say — passed to the model as it is. Leave it empty unless the server you are using documents something.")
                                  : qsTr("One name = value a line — vad_filter = true, say — passed to the recogniser as it is. Leave it empty unless the server you are using documents something.")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 12
                            opacity: 0.6
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: 4
                            visible: card.page.listens && settingsBody.providerExtrasShown
                            spacing: 2

                        TextField {

                            TextMenu {}

                            Layout.fillWidth: true

                            placeholderText: qsTr("Wait, minutes")
                            text: card.page.waitMinutes
                            inputMethodHints: Qt.ImhDigitsOnly
                            onTextEdited: card.page.setWaitMinutes(text)
                        }

                        Label {
                            Layout.fillWidth: true

                            text: qsTr("Leave this empty and the wait is worked out from the length of each recording, which is what almost every installation should do. Set it where you know your own machine is slower than that.")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 12
                            opacity: 0.6
                        }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: 4
                            visible: card.page.canStream && settingsBody.providerExtrasShown
                            spacing: 2

                            TextField {

                                TextMenu {}

                                Layout.fillWidth: true

                                placeholderText: qsTr("Price a minute")
                                text: card.page.realtimePrice
                                onTextEdited: card.page.setRealtimePrice(text)
                            }

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("What this recogniser charges for a minute of live audio, from its own price list. The prompter shows what a session has cost and stops at the monthly ceiling you set under Prompter; with nothing here it says the figure is an estimate rather than showing one.")
                                wrapMode: Text.WordWrap
                                font.pixelSize: 12
                                opacity: 0.6
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: 4
                            visible: card.page.priced && settingsBody.providerExtrasShown
                            spacing: 2

                            TextField {

                                TextMenu {}

                                Layout.fillWidth: true

                                placeholderText: qsTr("Price in, per million")
                                text: card.page.inPrice
                                onTextEdited: card.page.setInPrice(text)
                            }
                        }

                        TextField {

                            TextMenu {}

                            Layout.fillWidth: true
                            visible: card.page.priced && settingsBody.providerExtrasShown

                            placeholderText: qsTr("Price out, per million")
                            text: card.page.outPrice
                            onTextEdited: card.page.setOutPrice(text)
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: card.page.priced && settingsBody.providerExtrasShown
                            text: qsTr("Leave these empty for a model on your own machine: it costs nothing to run, and what this month cost will say so. The prices are the provider's own — copy them from its price list.")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 12
                            opacity: 0.6
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Button {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                topInset: 0
                                bottomInset: 0
                                highlighted: true
                                enabled: page.complete
                                text: qsTr("Save")
                                onClicked: page.save()
                            }

                            Button {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                topInset: 0
                                bottomInset: 0
                                text: qsTr("Cancel")
                                onClicked: page.cancel()
                            }

                            Button {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                topInset: 0
                                bottomInset: 0
                                visible: page.editingId.length > 0
                                text: qsTr("Delete")

                                onClicked: removeProviderDialog.ask(page)
                            }
                        }
        }
    }

    property Component automationCard: Item {

        property Item firstField: automationCardFirstField
        implicitHeight: automationCardLayout.implicitHeight + 24

        ColumnLayout {
            id: automationCardLayout

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 12
            spacing: 12

            Label {
                Layout.fillWidth: true
                text: automations.sentence
                wrapMode: Text.WordWrap
                font.italic: true
                opacity: 0.8
            }

            TextField {
                id: automationCardFirstField

                TextMenu {}

                Layout.fillWidth: true
                placeholderText: qsTr("Name")
                text: automations.name
                onTextEdited: automations.setName(text)
            }

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 4

                text: qsTr("Only if all of these are true")
                font.pixelSize: 12
                opacity: 0.6
            }

            Repeater {
                model: automations.conditions

                delegate: RowLayout {
                    id: clause

                    required property var modelData
                    required property int index

                    readonly property var operators: automations.operatorsFor(modelData.field)
                    readonly property var values: automations.valuesFor(modelData.field)
                    readonly property string kind: automations.valueKindFor(modelData.field,
                                                                            modelData.op)

                    Layout.fillWidth: true
                    spacing: 6

                    LabelledPicker {
                        id: fieldPicker

                        readonly property var choices: automations.fields

                        Layout.fillWidth: true
                        Layout.preferredWidth: 2
                        popupWidth: clause.width
                        placeholderText: qsTr("What")
                        model: choices.map((one) => one.id)
                        displayText: settingsBody.fieldLabel(model[currentIndex])

                        delegate: ItemDelegate {
                            required property var modelData
                            required property int index

                            width: fieldPicker.popup.width
                            text: settingsBody.fieldLabel(modelData)
                            highlighted: fieldPicker.highlightedIndex === index
                        }

                        currentIndex: Math.max(0, choices.findIndex(
                                                   (one) => one.id === clause.modelData.field))
                        onActivated: (chosen) => automations.setConditionField(
                                         clause.index, fieldPicker.choices[chosen].id)
                    }

                    LabelledPicker {
                        id: operatorPicker

                        readonly property var choices: clause.operators

                        Layout.fillWidth: true
                        Layout.preferredWidth: 2
                        popupWidth: clause.width
                        placeholderText: qsTr("How")
                        model: choices.map((one) => one.id)
                        displayText: settingsBody.operatorLabel(model[currentIndex])

                        delegate: ItemDelegate {
                            required property var modelData
                            required property int index

                            width: operatorPicker.popup.width
                            text: settingsBody.operatorLabel(modelData)
                            highlighted: operatorPicker.highlightedIndex === index
                        }

                        currentIndex: Math.max(0, choices.findIndex(
                                                   (one) => one.id === clause.modelData.op))
                        onActivated: (chosen) => automations.setConditionOperator(
                                         clause.index, operatorPicker.choices[chosen].id)
                    }

                    LabelledPicker {
                        id: valuePicker

                        readonly property var choices: clause.kind !== "choice"
                            ? []
                            : (clause.modelData.op === "present"
                               || clause.modelData.op === "absent")
                              ? [{id: "", label: qsTr("any of them")}].concat(clause.values)
                              : clause.values

                        Layout.fillWidth: true
                        Layout.preferredWidth: 2
                        popupWidth: clause.width
                        visible: clause.kind === "choice"
                        placeholderText: qsTr("Which")
                        model: choices.map((one) => one.id)
                        displayText: valuePicker.currentIndex >= 0
                                     && valuePicker.currentIndex < choices.length
                                     ? choices[valuePicker.currentIndex].label
                                     : ""

                        delegate: ItemDelegate {
                            required property var modelData
                            required property int index

                            width: valuePicker.popup.width
                            text: valuePicker.choices[index] !== undefined
                                  ? valuePicker.choices[index].label : ""
                            highlighted: valuePicker.highlightedIndex === index
                        }

                        currentIndex: Math.max(0, choices.findIndex(
                                                   (one) => one.id === clause.modelData.value))
                        onActivated: (chosen) => automations.setConditionValue(
                                         clause.index, valuePicker.choices[chosen].id)
                    }

                    TextField {

                        TextMenu {}

                        Layout.fillWidth: true
                        Layout.preferredWidth: 2
                        visible: clause.kind === "number" || clause.kind === "text"

                        placeholderText: clause.kind === "number" ? qsTr("How many")
                                                                  : qsTr("Value")
                        inputMethodHints: clause.kind === "number" ? Qt.ImhDigitsOnly
                                                                   : Qt.ImhNone
                        validator: clause.kind === "number" ? wholeNumbers : null

                        IntValidator {
                            id: wholeNumbers

                            bottom: 0
                            top: 1000000
                        }
                        text: clause.modelData.value
                        onTextEdited: automations.setConditionValue(clause.index, text)
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 2
                        visible: clause.kind === "none"
                    }

                    ToolButton {
                        text: "✕"
                        onClicked: automations.removeCondition(clause.index)
                        Accessible.name: qsTr("Delete this condition")
                    }
                }
            }

            Button {
                Layout.fillWidth: true
                topInset: 0
                bottomInset: 0
                text: qsTr("Add a condition")
                onClicked: automations.addCondition()
            }

            RowLayout {
                id: actionRow

                Layout.fillWidth: true
                Layout.topMargin: 4
                spacing: 6

                LabelledPicker {
                    id: actionPicker

                    readonly property var choices: automations.actions

                    Layout.fillWidth: true
                    popupWidth: actionRow.width
                    placeholderText: qsTr("Action")
                    model: choices.map((one) => one.id)
                    displayText: settingsBody.ruleActionLabel(model[currentIndex])

                    delegate: ItemDelegate {
                        required property var modelData
                        required property int index

                        width: actionPicker.popup.width
                        text: settingsBody.ruleActionLabel(modelData)
                        highlighted: actionPicker.highlightedIndex === index
                    }

                    currentIndex: Math.max(0, choices.findIndex(
                                               (one) => one.id === automations.action))
                    onActivated: (chosen) => automations.setAction(actionPicker.choices[chosen].id)
                }

                LabelledPicker {
                    id: promptPicker

                    readonly property var choices: automations.prompts

                    Layout.fillWidth: true
                    popupWidth: actionRow.width
                    visible: automations.action === "generate"
                    placeholderText: qsTr("Prompt")
                    model: choices.map((one) => one.id)
                    displayText: settingsBody.promptLabel(model[currentIndex])

                    delegate: ItemDelegate {
                        required property var modelData
                        required property int index

                        width: promptPicker.popup.width
                        text: settingsBody.promptLabel(modelData)
                        highlighted: promptPicker.highlightedIndex === index
                    }

                    currentIndex: Math.max(0, choices.findIndex(
                                               (one) => one.id === automations.templateCode))
                    onActivated: (chosen) => automations.setTemplateCode(
                                     promptPicker.choices[chosen].id)
                }

                LabelledPicker {
                    id: rulesTagPicker

                    readonly property var choices: automations.valuesFor("tag")

                    Layout.fillWidth: true
                    popupWidth: actionRow.width
                    visible: automations.action === "tag"
                    placeholderText: qsTr("Tag")
                    model: choices.map((one) => one.id)
                    displayText: rulesTagPicker.currentIndex >= 0
                                 && rulesTagPicker.currentIndex < choices.length
                                 ? choices[rulesTagPicker.currentIndex].label
                                 : ""

                    delegate: ItemDelegate {
                        required property var modelData
                        required property int index

                        width: rulesTagPicker.popup.width
                        text: rulesTagPicker.choices[index] !== undefined
                              ? rulesTagPicker.choices[index].label : ""
                        highlighted: rulesTagPicker.highlightedIndex === index
                    }

                    currentIndex: Math.max(0, choices.findIndex(
                                               (one) => one.id === automations.tagCode))
                    onActivated: (chosen) => automations.setTagCode(
                                     rulesTagPicker.choices[chosen].id)
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                ListCheckBox {
                    Layout.fillWidth: true

                    text: qsTr("Enabled")
                    checked: automations.switchedOn
                    onToggled: automations.setSwitchedOn(checked)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    topInset: 0
                    bottomInset: 0
                    highlighted: true
                    enabled: automations.complete
                    text: qsTr("Save")
                    onClicked: automations.save()
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    topInset: 0
                    bottomInset: 0
                    text: qsTr("Cancel")
                    onClicked: automations.cancel()
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    topInset: 0
                    bottomInset: 0
                    visible: automations.editingId.length > 0
                    text: qsTr("Delete")
                    onClicked: removeAutomationDialog.ask(automations.editingId)
                }
            }
        }
    }

    property Component templateCard: Item {

        property Item firstField: templateCardFirstField
        implicitHeight: templateCardLayout.implicitHeight + 24

        ColumnLayout {
            id: templateCardLayout

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 12
            spacing: 12

            TextField {
                id: templateCardFirstField

                TextMenu {}

                Layout.fillWidth: true
                placeholderText: qsTr("Name")
                text: templates.name
                onTextEdited: templates.setName(text)
            }

            LabelledPicker {
                id: templateKindPicker

                readonly property var choices: templates.kinds

                Layout.fillWidth: true
                visible: !templates.realtime
                placeholderText: qsTr("Produces")
                model: choices.map((one) => one.id)
                displayText: settingsBody.templateKindLabel(model[currentIndex])

                delegate: ItemDelegate {
                    required property var modelData
                    required property int index

                    width: templateKindPicker.width
                    text: settingsBody.templateKindLabel(modelData)
                    highlighted: templateKindPicker.highlightedIndex === index
                }

                currentIndex: Math.max(0, choices.findIndex((one) => one.id === templates.kind))
                onActivated: (chosen) => templates.setKind(templateKindPicker.choices[chosen].id)
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("What the answer becomes. A one-line summary goes in the conversation list beside the caller; a category files the conversation; tags and red flags go into their dictionaries and into the filters.")
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6
            }

            LabelledPicker {
                id: templateShapePicker

                readonly property var choices: templates.shapes

                Layout.fillWidth: true
                placeholderText: qsTr("Answer shape")
                model: choices.map((one) => one.id)
                displayText: settingsBody.templateShapeLabel(model[currentIndex])

                delegate: ItemDelegate {
                    required property var modelData
                    required property int index

                    width: templateShapePicker.width
                    text: settingsBody.templateShapeLabel(modelData)
                    highlighted: templateShapePicker.highlightedIndex === index
                }

                currentIndex: Math.max(0, choices.findIndex((one) => one.id === templates.shape))
                onActivated: (chosen) => templates.setShape(templateShapePicker.choices[chosen].id)
            }

            Label {
                Layout.fillWidth: true

                text: (void templates.shape, templates.shapeInstructionTitle().length > 0)
                      ? qsTr("The shape decides what the model is asked for and how the answer is drawn. The words it is asked in are the “%1” row under Dictionaries → Instructions.").arg(templates.shapeInstructionTitle())
                      : qsTr("The shape decides what the model is asked for and how the answer is drawn. Change it and the same prompt comes back as a paragraph, a list or a table.")
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
                visible: templates.instructions.length > 0
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Also send")
                    font.pixelSize: 12
                    opacity: 0.6
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Repeater {
                        model: templates.instructions

                        delegate: ListCheckBox {
                            required property var modelData

                            Layout.fillWidth: true
                            text: modelData.label
                            checked: modelData.chosen
                            onToggled: templates.toggleInstruction(modelData.id, checked)
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                visible: templates.instructions.length > 0
                text: qsTr("Added after the shape's own words and the sentence about the language, in the order they are listed. Write your own under Dictionaries → Instructions — “answer briefly”, “never quote a price” — and every prompt you tick it on says it.")
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6
            }

            GridLayout {
                Layout.fillWidth: true
                visible: templates.realtime
                columns: templateCardLayout.width >= 520 ? 2 : 1
                columnSpacing: 12
                rowSpacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    spacing: 4

                    LabelledPicker {
                        id: templateRecogniserPicker

                        readonly property var choices: templates.recognisers

                        Layout.fillWidth: true
                        placeholderText: qsTr("Recogniser")
                        model: choices.map((one) => one.id)
                        displayText: {
                            const found = choices.find((one) => one.id === model[currentIndex]);
                            return found ? found.label : "";
                        }

                        delegate: ItemDelegate {
                            required property var modelData
                            required property int index

                            width: templateRecogniserPicker.width
                            text: {
                                const found = templateRecogniserPicker.choices.find(
                                    (one) => one.id === modelData);
                                return found ? found.label : modelData;
                            }
                            highlighted: templateRecogniserPicker.highlightedIndex === index
                        }

                        currentIndex: Math.max(
                            0, choices.findIndex((one) => one.id === templates.asrProviderId))
                        onActivated: (chosen) => templates.setAsrProviderId(
                                         templateRecogniserPicker.choices[chosen].id)
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: templates.recognisers.length <= 1
                        text: qsTr("No recogniser here can listen as a conversation happens. Add one under Transcription and press Test — none of them works until it has an address or a key.")
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                        opacity: 0.6
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    spacing: 4

                    LabelledPicker {
                        id: templateTurnPicker

                        readonly property var choices: templates.turnPolicies

                        Layout.fillWidth: true
                        placeholderText: qsTr("When a reply has ended")
                        model: choices.map((one) => one.id)
                        displayText: {
                            const found = choices.find((one) => one.id === model[currentIndex]);
                            return found ? found.label : "";
                        }

                        delegate: ItemDelegate {
                            required property var modelData
                            required property int index

                            width: templateTurnPicker.width
                            text: {
                                const found = templateTurnPicker.choices.find(
                                    (one) => one.id === modelData);
                                return found ? found.label : modelData;
                            }
                            highlighted: templateTurnPicker.highlightedIndex === index
                        }

                        currentIndex: Math.max(
                            0, choices.findIndex((one) => one.id === templates.turnPolicy))
                        onActivated: (chosen) => templates.setTurnPolicy(
                                         templateTurnPicker.choices[chosen].id)
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Six of the recognisers say where a reply ends and four do not. The recogniser's own answer falls back to a pause where it has none, which is why it is the one to leave this on.")
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                        opacity: 0.6
                    }
                }
            }

            ListCheckBox {
                Layout.fillWidth: true
                visible: templates.realtime
                text: qsTr("Recognise my side too")
                checked: templates.realtimeNear
                onToggled: templates.setRealtimeNear(checked)
            }

            Label {
                Layout.fillWidth: true
                visible: templates.realtime

                text: qsTr("A second session on the same recogniser, at twice the price. Your words go into the column and into what the model is told, and are never what it is asked about.")
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Role — what the model is")
                    font.pixelSize: 12
                    opacity: 0.6
                }

                MemoField {
                    shortest: 72
                    text: templates.system
                    onEdited: (value) => templates.setSystem(value)
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: qsTr("The prompt")
                    font.pixelSize: 12
                    opacity: 0.6
                }

                MemoField {
                    shortest: 160
                    tallest: 420
                    text: templates.body
                    onEdited: (value) => templates.setBody(value)
                }

                Label {
                    Layout.fillWidth: true
                    visible: templates.realtime && !templates.asksAModel
                    text: qsTr("Leave it empty and nothing is asked of a model — the words are shown as they arrive, and the only thing being paid for is the recogniser.")
                    wrapMode: Text.WordWrap
                    font.pixelSize: 12
                    opacity: 0.6
                }
            }

            Label {
                Layout.fillWidth: true
                visible: templates.refusal.length > 0
                text: templates.refusal
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Ink.bad(Material.theme)
            }

            Label {
                Layout.fillWidth: true

                text: qsTr("You can use: %1").arg(templates.placeholders.join("  "))
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6
            }

            GridLayout {
                Layout.fillWidth: true
                columns: templateCardLayout.width >= 520 ? 2 : 1
                columnSpacing: 12
                rowSpacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    spacing: 12

                    LabelledPicker {
                        id: templatePolicyPicker

                        readonly property var choices: templates.policies

                        Layout.fillWidth: true
                        placeholderText: qsTr("Answer in")
                        model: choices.map((one) => one.id)
                        displayText: settingsBody.templatePolicyLabel(model[currentIndex])

                        delegate: ItemDelegate {
                            required property var modelData
                            required property int index

                            width: templatePolicyPicker.width
                            text: settingsBody.templatePolicyLabel(modelData)
                            highlighted: templatePolicyPicker.highlightedIndex === index
                        }

                        currentIndex: Math.max(
                            0, choices.findIndex((one) => one.id === templates.policy))
                        onActivated: (chosen) => templates.setPolicy(
                                         templatePolicyPicker.choices[chosen].id)
                    }

                    TextField {

                        TextMenu {}

                        Layout.fillWidth: true
                        visible: templates.policy === "fixed"
                        placeholderText: qsTr("Language code")
                        text: templates.language
                        onTextEdited: templates.setLanguage(text)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    spacing: 4

                    LabelledPicker {
                        id: templateModelPicker

                        readonly property var choices: templates.models

                        Layout.fillWidth: true
                        visible: choices.length > 1
                        placeholderText: qsTr("Model")
                        model: choices.map((one) => one.label)

                        delegate: ItemDelegate {
                            required property var modelData
                            required property int index

                            width: templateModelPicker.width
                            text: modelData
                            highlighted: templateModelPicker.highlightedIndex === index
                        }

                        currentIndex: Math.max(0, choices.findIndex(
                                                   (one) => one.id === templates.providerId))
                        onActivated: (chosen) => templates.setProviderId(
                                         templateModelPicker.choices[chosen].id)
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                ListCheckBox {
                    Layout.fillWidth: true

                    text: qsTr("Enabled")
                    checked: templates.switchedOn
                    onToggled: templates.setSwitchedOn(checked)
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Extras")
                    font.pixelSize: 12
                    opacity: 0.6
                }

                MemoField {
                    shortest: 60
                    text: templates.params
                    wrapMode: TextEdit.NoWrap
                    onEdited: (value) => templates.setParams(value)
                }

                Label {
                    Layout.fillWidth: true

                    text: qsTr("One name = value a line — temperature = 0.7, say. These win over the model's own for this prompt alone.")
                    wrapMode: Text.WordWrap
                    font.pixelSize: 12
                    opacity: 0.6
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    topInset: 0
                    bottomInset: 0
                    text: qsTr("Preview")
                    enabled: templates.refusal.length === 0
                    onClicked: templates.askForPreview()
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    topInset: 0
                    bottomInset: 0
                    visible: templates.restorable
                    text: qsTr("Restore")
                    onClicked: templates.restore()
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    topInset: 0
                    bottomInset: 0
                    visible: templates.editingId.length > 0
                    text: qsTr("Duplicate")
                    onClicked: templates.duplicate()
                }
            }

            Label {
                Layout.fillWidth: true
                visible: templates.previewProblem.length > 0
                text: templates.previewProblem
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6
            }

            Label {
                Layout.fillWidth: true
                visible: templates.previewSource.length > 0 && templates.preview.length > 0
                text: templates.previewSource
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6
            }

            MemoField {
                shortest: 200
                tallest: 460
                visible: templates.preview.length > 0
                text: templates.preview
                readOnly: true
                textFont.family: "monospace"
                textFont.pixelSize: 12
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    topInset: 0
                    bottomInset: 0
                    highlighted: true
                    enabled: templates.complete
                    text: qsTr("Save")
                    onClicked: templates.save()
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    topInset: 0
                    bottomInset: 0
                    text: qsTr("Cancel")
                    onClicked: templates.cancel()
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    topInset: 0
                    bottomInset: 0
                    visible: templates.editingId.length > 0
                    text: qsTr("Delete")
                    onClicked: removeTemplateDialog.ask(templates.editingId)
                }
            }
        }
    }

    property Component taxonomyCard: Item {

        property Item firstField: taxonomyCardFirstField
        implicitHeight: taxonomyCardLayout.implicitHeight + 24

        ColumnLayout {
            id: taxonomyCardLayout

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 12
            spacing: 12

            TextField {
                id: taxonomyCardFirstField

                TextMenu {}

                Layout.fillWidth: true
                placeholderText: qsTr("Name")
                text: taxonomy.title
                onTextEdited: taxonomy.setTitle(text)
            }

            TextField {

                TextMenu {}

                Layout.fillWidth: true
                visible: !taxonomy.codeLocked
                placeholderText: qsTr("Code")
                text: taxonomy.code
                onTextEdited: taxonomy.setCode(text)
            }

            Label {
                Layout.fillWidth: true
                visible: !taxonomy.codeLocked
                text: qsTr("Lowercase letters, digits and hyphens. This is what everything else stores, so it stops being free to change the moment a conversation refers to it.")
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6
            }

            Label {
                Layout.fillWidth: true
                visible: taxonomy.codeLocked
                text: qsTr("Code: %1 — fixed, because conversations already refer to it. The name above is free to change.").arg(taxonomy.code)
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6
            }

            Label {
                Layout.fillWidth: true
                visible: taxonomy.refusal.length > 0
                text: taxonomy.refusal
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Ink.bad(Material.theme)
            }

            MemoField {
                shortest: 88
                placeholderText: taxonomy.editingKind === "instruction"
                                 ? qsTr("What the model is told")
                                 : qsTr("What this means")
                text: taxonomy.description
                onEdited: (value) => taxonomy.setDescription(value)
            }

            Label {
                Layout.fillWidth: true
                text: taxonomy.editingKind === "instruction"
                      ? qsTr("Sent word for word, in front of the prompt. Empty it and nothing of the kind is sent; delete the entry and what the program came with is sent instead.")
                      : qsTr("This sentence is given to the model along with the name, so write what would make an answer right rather than what the entry is called.")
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6
            }

            LabelledPicker {
                id: severityPicker

                readonly property var values: ["low", "medium", "high"]

                Layout.fillWidth: true
                visible: taxonomy.editingKind === "red_flag"
                placeholderText: qsTr("Severity")
                model: values
                displayText: settingsBody.severityLabel(model[currentIndex])

                delegate: ItemDelegate {
                    required property var modelData
                    required property int index

                    width: severityPicker.width
                    text: settingsBody.severityLabel(modelData)
                    highlighted: severityPicker.highlightedIndex === index
                }

                currentIndex: Math.max(0, values.indexOf(taxonomy.severity))
                onActivated: (chosen) => taxonomy.setSeverity(severityPicker.values[chosen])
            }

            Label {
                Layout.fillWidth: true
                visible: taxonomy.editingKind === "red_flag"
                text: qsTr("What an occurrence gets when the model does not say otherwise.")
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: taxonomy.editingId.length > 0
                         && taxonomy.editingKind !== "instruction"
                spacing: 0

                ListCheckBox {
                    Layout.fillWidth: true

                    text: qsTr("Deleted")
                    checked: taxonomy.retired
                    onToggled: taxonomy.setRetired(checked)
                }
            }

            Label {
                Layout.fillWidth: true
                visible: taxonomy.editingId.length > 0
                         && taxonomy.editingKind !== "instruction"
                text: qsTr("A deleted entry is no longer offered and is no longer given to the models. Conversations that already carry it go on showing it.")
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6
            }

            Label {
                Layout.fillWidth: true
                visible: taxonomy.builtIn
                text: qsTr("This entry came with the program and its name is translated. Changing the words here makes them yours, and they stop following the interface language.")
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    topInset: 0
                    bottomInset: 0
                    highlighted: true
                    enabled: taxonomy.complete && taxonomy.code.trim().length > 0
                             && taxonomy.title.trim().length > 0
                    text: qsTr("Save")
                    onClicked: taxonomy.save()
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    topInset: 0
                    bottomInset: 0
                    text: qsTr("Cancel")
                    onClicked: taxonomy.cancel()
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    topInset: 0
                    bottomInset: 0
                    visible: taxonomy.editingId.length > 0
                    text: qsTr("Delete")
                    onClicked: settingsBody.askAboutRemoving(taxonomy.editingId)
                }
            }
        }
    }

    function askAboutRemoving(id) {
        const row = taxonomy.rowFor(id)
        removeEntryDialog.entryId = id
        removeEntryDialog.entryTitle = row.title
        removeEntryDialog.used = row.used
        removeEntryDialog.open()
    }

    property Component accountCard: Item {

        property Item firstField: accountCardFirstField

        implicitHeight: accountCardLayout.implicitHeight + 24

        ColumnLayout {
            id: accountCardLayout

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 12
            spacing: 12

        TextField {
            id: accountCardFirstField

            TextMenu {}

            Layout.fillWidth: true
            placeholderText: qsTr("Name (optional)")
            text: accounts.displayName
            onTextEdited: accounts.displayName = text
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {

                TextMenu {}

                Layout.fillWidth: true
                Layout.preferredWidth: 1
                placeholderText: qsTr("Username")
                text: accounts.username
                onTextEdited: accounts.username = text
            }

            TextField {

                TextMenu {}

                Layout.fillWidth: true
                Layout.preferredWidth: 1
                placeholderText: qsTr("Password")
                echoMode: TextInput.Password
                text: accounts.password
                onTextEdited: accounts.password = text
            }
        }

        TextField {

            TextMenu {}

            Layout.fillWidth: true
            placeholderText: qsTr("Server address")
            text: accounts.domain
            onTextEdited: accounts.domain = text
        }

        Label {
            Layout.fillWidth: true
            text: settingsBody.secretNote()
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            color: settingsBody.secretNoteIsPlain()
                   ? Material.foreground : Ink.warn(Material.theme)
            opacity: settingsBody.secretNoteIsPlain() ? 0.6 : 1.0
        }

        Button {
            Layout.fillWidth: true
            flat: true
            text: settingsBody.advancedShown
                  ? qsTr("Hide server settings")
                  : qsTr("Server settings")
            onClicked: settingsBody.advancedShown = !settingsBody.advancedShown
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12
            visible: settingsBody.advancedShown

            TextField {

                TextMenu {}

                Layout.fillWidth: true
                placeholderText: qsTr("Authentication user")
                text: accounts.authUser
                onTextEdited: accounts.authUser = text
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                LabelledPicker {
                    id: transportPicker

                    readonly property var values: [AccountsViewModel.Transport.Udp,
                                                   AccountsViewModel.Transport.Tcp,
                                                   AccountsViewModel.Transport.Tls]

                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    placeholderText: qsTr("Transport")
                    model: values
                    displayText: settingsBody.transportLabel(model[currentIndex])

                    delegate: ItemDelegate {
                        required property var modelData
                        required property int index

                        width: transportPicker.width
                        text: settingsBody.transportLabel(modelData)
                        highlighted: transportPicker.highlightedIndex === index
                    }

                    currentIndex: Math.max(0, values.indexOf(accounts.transport))
                    onActivated: (chosen) => accounts.transport
                                 = transportPicker.values[chosen]

                    Connections {
                        target: accounts
                        function onEditorChanged() {
                            transportPicker.currentIndex
                                = Qt.binding(() => Math.max(
                                                 0, transportPicker.values.indexOf(
                                                     accounts.transport)))
                        }
                    }
                }

                TextField {

                    TextMenu {}

                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    placeholderText: qsTr("Port")
                    text: accounts.port > 0 ? String(accounts.port) : ""
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator {
                        bottom: 1
                        top: 65535
                    }
                    onTextEdited: accounts.port = parseInt(text, 10) || 0
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 12
                visible: accounts.transport === AccountsViewModel.Transport.Tls

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    CheckBox {
                        Layout.fillWidth: true
                        text: qsTr("Verify the certificate")
                        checked: accounts.verifyCertificate
                        onToggled: accounts.verifyCertificate = checked
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Untick only for a server whose administrator made its certificate themselves. The connection stays encrypted, but anyone between you and the server could pretend to be it.")
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                        opacity: 0.6
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    LabelledPicker {
                        id: encryptionPicker

                        readonly property var values: [AccountsViewModel.Encryption.Off,
                                                       AccountsViewModel.Encryption.Offered,
                                                       AccountsViewModel.Encryption.Required]

                        Layout.fillWidth: true
                        placeholderText: qsTr("Call encryption")
                        model: values
                        displayText: settingsBody.encryptionLabel(model[currentIndex])

                        delegate: ItemDelegate {
                            required property var modelData
                            required property int index

                            width: encryptionPicker.width
                            text: settingsBody.encryptionLabel(modelData)
                            highlighted: encryptionPicker.highlightedIndex === index
                        }

                        currentIndex: Math.max(0, values.indexOf(accounts.mediaEncryption))
                        onActivated: (chosen) => accounts.mediaEncryption
                                     = encryptionPicker.values[chosen]

                        Connections {
                            target: accounts
                            function onEditorChanged() {
                                encryptionPicker.currentIndex
                                    = Qt.binding(() => Math.max(
                                                     0, encryptionPicker.values.indexOf(
                                                         accounts.mediaEncryption)))
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Encrypts the sound of calls on this account (SRTP). Required refuses a call the server will not encrypt.")
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                        opacity: 0.6
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                TextField {

                    TextMenu {}

                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    placeholderText: qsTr("Outbound proxy")
                    text: accounts.outboundProxy
                    onTextEdited: accounts.outboundProxy = text
                }

                TextField {

                    TextMenu {}

                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    placeholderText: qsTr("Registrar")
                    text: accounts.registrar
                    onTextEdited: accounts.registrar = text
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                TextField {

                    TextMenu {}

                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    placeholderText: qsTr("Re-register, seconds")
                    text: accounts.expirySeconds > 0
                          ? String(accounts.expirySeconds) : ""
                    inputMethodHints: Qt.ImhDigitsOnly
                    onTextEdited: accounts.expirySeconds = parseInt(text, 10) || 0
                }

                LabelledPicker {
                        id: dtmfPicker

                        readonly property var values: [AccountsViewModel.Dtmf.Rfc2833,
                                                       AccountsViewModel.Dtmf.SipInfo]

                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        placeholderText: qsTr("Keypad tones")
                        model: values
                        displayText: settingsBody.dtmfShort(model[currentIndex])

                        delegate: ItemDelegate {
                            required property var modelData
                            required property int index

                            width: dtmfPicker.width
                            text: settingsBody.dtmfLabel(modelData)
                            highlighted: dtmfPicker.highlightedIndex === index
                        }

                        currentIndex: Math.max(0, values.indexOf(accounts.dtmfTransport))
                        onActivated: (chosen) => accounts.dtmfTransport
                                     = dtmfPicker.values[chosen]

                        Connections {
                            target: accounts
                            function onEditorChanged() {
                                dtmfPicker.currentIndex
                                    = Qt.binding(() => Math.max(
                                                     0, dtmfPicker.values.indexOf(
                                                         accounts.dtmfTransport)))
                            }
                        }
                }
            }

        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Answering")
            font.pixelSize: 15
            font.bold: true
        }

        CheckBox {
            Layout.fillWidth: true
            text: qsTr("Answer automatically")
            checked: accounts.autoAnswer
            onToggled: accounts.autoAnswer = checked
        }

        TextField {

            TextMenu {}

            Layout.fillWidth: true
            visible: accounts.autoAnswer

            placeholderText: qsTr("Ring first, seconds")
            text: accounts.autoAnswerAfterSeconds > 0
                  ? String(accounts.autoAnswerAfterSeconds) : ""
            inputMethodHints: Qt.ImhDigitsOnly

            validator: IntValidator {
                bottom: 1
                top: accounts.maxAutoAnswerAfterSeconds
            }
            onTextEdited: accounts.autoAnswerAfterSeconds
                          = parseInt(text, 10) || 0
        }

        Label {
            Layout.fillWidth: true
            visible: accounts.autoAnswer
            text: qsTr("Between 1 and %1 seconds. Leave the delay empty to answer at once, without ringing. A call that arrives while you are already on one always rings and waits for you.").arg(accounts.maxAutoAnswerAfterSeconds)
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: 0.6
        }

        Label {
            Layout.fillWidth: true
            visible: text.length > 0
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            color: Ink.warn(Material.theme)
            text: settingsBody.faultText(accounts.fault)
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                highlighted: true
                enabled: accounts.canSave
                text: qsTr("Save")
                onClicked: accounts.save()
            }

            Button {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                text: qsTr("Cancel")
                onClicked: accounts.cancelEdit()
            }

            Button {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                visible: !accounts.addingNew
                text: qsTr("Delete")
                onClicked: removeAccountDialog.open()
            }
        }
        }
    }

    function lampColour(state) {
        switch (state) {
        case ButtonsViewModel.Idle:
            return Mark.good
        case ButtonsViewModel.Ringing:
            return Mark.warn(Material.theme)
        case ButtonsViewModel.Busy:
            return Mark.bad
        }
        return Qt.rgba(Material.foreground.r, Material.foreground.g, Material.foreground.b, 0.22)
    }

    function keyDetail(entry) {
        void Qt.uiLanguage

        return entry.lineLabel === ""
             ? entry.number
             : qsTr("%1 · %2").arg(entry.number).arg(entry.lineLabel)
    }

    property Component keyCard: Item {

        property Item firstField: keyCardFirstField

        implicitHeight: keyCardLayout.implicitHeight + 24

        ColumnLayout {
            id: keyCardLayout

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 12
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                TextField {
                    id: keyCardFirstField

                    TextMenu {}

                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    placeholderText: qsTr("Number")
                    text: buttons.draftNumber
                    onTextEdited: buttons.draftNumber = text
                }

                LabelledPicker {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    placeholderText: qsTr("Line")

                    visible: buttons.lines.length > 2
                    model: buttons.lines
                    textRole: "label"
                    valueRole: "id"

                    displayText: currentValue === undefined || currentValue === ""
                                 ? qsTr("Line")
                                 : currentText
                    currentIndex: indexOfValue(buttons.draftLine)
                    onActivated: buttons.draftLine = currentValue

                    delegate: ItemDelegate {
                        required property var modelData
                        required property int index

                        width: parent ? parent.width : 0
                        text: modelData.label === "" ? qsTr("Line") : modelData.label
                        highlighted: parent && parent.parent
                                     && parent.parent.highlightedIndex === index
                    }
                }

                TextField {

                    TextMenu {}

                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    placeholderText: qsTr("Label")
                    text: buttons.draftTitle
                    onTextEdited: buttons.draftTitle = text
                }
            }

            Switch {
                Layout.fillWidth: true
                text: qsTr("Show whether this line is busy")
                checked: buttons.draftWatched
                enabled: buttons.canWatch
                onToggled: buttons.draftWatched = checked
            }

            Label {
                Layout.fillWidth: true
                visible: !buttons.canWatch
                text: qsTr("Choose a line first: the phone asks that line's switch about this number, so it has to know which one.")
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.6
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    highlighted: true
                    enabled: buttons.canSave
                    text: qsTr("Save")
                    onClicked: buttons.save()
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    text: qsTr("Cancel")
                    onClicked: buttons.cancel()
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    visible: !buttons.addingNew
                    text: qsTr("Delete")
                    onClicked: removeKeyDialog.open()
                }
            }
        }
    }

    Shortcut {
        sequence: StandardKey.Cancel
        enabled: settingsBody.onScreen
                 && (accounts.editing || buttons.editing || providers.editing || taxonomy.editing
                     || models.editing || templates.editing || automations.editing)
        onActivated: {
            if (automations.editing) {
                automations.cancel()
            } else if (templates.editing) {
                templates.cancel()
            } else if (models.editing) {
                models.cancel()
            } else if (taxonomy.editing) {
                taxonomy.cancel()
            } else if (providers.editing) {
                providers.cancel()
            } else if (buttons.editing) {
                buttons.cancel()
            } else {
                accounts.cancelEdit()
            }
        }
    }

    readonly property int showsAtLeast: 40

    function bringIntoView(item) {
        if (!item) {
            return
        }

        Qt.callLater(function () {
            const view = settingsScroller.contentItem
            if (!view || !item.parent) {
                return
            }

            const top = item.mapToItem(view.contentItem, 0, 0).y
            const limit = Math.max(0, view.contentHeight - view.height)

            if (top < view.contentY) {
                view.contentY = Math.max(0, top - 12)
            } else if (top > view.contentY + view.height - settingsBody.showsAtLeast) {
                view.contentY = Math.min(limit, top - 12)
            }
        })
    }

    function keepForLabel(days) {
        void Qt.uiLanguage

        if (days <= 0) {
            return qsTr("Always")
        }
        if (days % 365 === 0) {
            return qsTr("%n year(s)", "", days / 365)
        }
        if (days % 30 === 0) {
            return qsTr("%n month(s)", "", days / 30)
        }
        return qsTr("%n day(s)", "", days)
    }

    function callWaitingLabel(waiting) {
        void Qt.uiLanguage

        return waiting === SettingsViewModel.CallWaiting.Busy

             ? qsTr("Answer busy")

             : qsTr("Ring the second call")
    }

    function sizeLabel(megabytes) {
        void Qt.uiLanguage

        if (megabytes <= 0) {
            return qsTr("No limit")
        }
        if (megabytes >= 1024) {
            return qsTr("%1 GB").arg((megabytes / 1024).toFixed(megabytes % 1024 === 0 ? 0 : 1))
        }
        return qsTr("%1 MB").arg(megabytes)
    }

    function recordingPolicyLabel(value) {
        void Qt.uiLanguage

        switch (value) {
        case 0:
            return qsTr("Manually")
        case 1:
            return qsTr("Ask each call")
        case 2:
            return qsTr("Every call")
        case 3:
            return qsTr("Selected lines")
        }
        return ""
    }

    function ruleActionLabel(id) {
        void Qt.uiLanguage
        return automations.actions.find((one) => one.id === id)?.label ?? id
    }

    function fieldLabel(id) {
        void Qt.uiLanguage
        return automations.fields.find((one) => one.id === id)?.label ?? id
    }

    function operatorLabel(id) {
        void Qt.uiLanguage
        return automations.operators.find((one) => one.id === id)?.label ?? id
    }

    function promptLabel(id) {
        void Qt.uiLanguage
        return automations.prompts.find((one) => one.id === id)?.label ?? id
    }

    function templateKindLabel(id) {
        void Qt.uiLanguage
        return templates.kinds.find((one) => one.id === id)?.label ?? id
    }

    function templateShapeLabel(id) {
        void Qt.uiLanguage
        return templates.shapes.find((one) => one.id === id)?.label ?? id
    }

    function templatePolicyLabel(id) {
        void Qt.uiLanguage
        return templates.policies.find((one) => one.id === id)?.label ?? id
    }

    function severityLabel(code) {
        void Qt.uiLanguage

        switch (code) {
        case "low":
            return qsTr("Low")
        case "medium":
            return qsTr("Medium")
        case "high":
            return qsTr("High")
        }
        return code
    }

    function recordingConsentLabel(value) {
        void Qt.uiLanguage

        switch (value) {
        case 0:
            return qsTr("An announcement")
        case 1:
            return qsTr("A tone every few seconds")
        case 2:

            return qsTr("I have their consent")
        case 3:
            return qsTr("Nothing at all")
        }
        return ""
    }

    function glyphsLabel(value) {
        void Qt.uiLanguage

        switch (value) {
        case SettingsViewModel.Glyphs.Solid:
            return qsTr("Solid")
        case SettingsViewModel.Glyphs.Outline:
            return qsTr("Outline")
        }
        return ""
    }

    function sketchLabel(value) {
        void Qt.uiLanguage

        switch (value) {
        case SettingsViewModel.Sketch.Nothing:
            return qsTr("Nothing")
        case SettingsViewModel.Sketch.Levels:
            return qsTr("Two levels")
        case SettingsViewModel.Sketch.Histogram:
            return qsTr("Sliding bars")
        case SettingsViewModel.Sketch.Wave:
            return qsTr("Wave")
        case SettingsViewModel.Sketch.Spectrum:
            return qsTr("Spectrum")
        case SettingsViewModel.Sketch.Aurora:
            return qsTr("Aurora")
        case SettingsViewModel.Sketch.Halo:
            return qsTr("Halo")
        case SettingsViewModel.Sketch.Uprights:
            return qsTr("Uprights")
        }
        return ""
    }

    function iconScaleLabel(value) {
        void Qt.uiLanguage

        switch (value) {
        case SettingsViewModel.IconScale.Normal:
            return qsTr("Normal")
        case SettingsViewModel.IconScale.Large:
            return qsTr("Large")
        }
        return ""
    }

    function verbosityLabel(code) {
        void Qt.uiLanguage

        switch (code) {
        case SettingsViewModel.Verbosity.Error:
            return qsTr("Errors only")
        case SettingsViewModel.Verbosity.Warning:
            return qsTr("Errors and warnings")
        case SettingsViewModel.Verbosity.Info:
            return qsTr("The usual")
        case SettingsViewModel.Verbosity.Debug:
            return qsTr("Detailed")
        case SettingsViewModel.Verbosity.Trace:
            return qsTr("Everything, with SIP messages")
        }
        return ""
    }

    readonly property string templateExample:
        "http://crm.example/pop?number=[number]&who=[name]"

    function logSizeText(bytes) {
        void Qt.uiLanguage

        const locale = Qt.locale(Qt.uiLanguage)
        if (bytes <= 0)
            return qsTr("empty")
        if (bytes < 1024)
            return qsTr("%1 bytes").arg(Number(bytes).toLocaleString(locale, 'f', 0))
        if (bytes < 1024 * 1024)
            return qsTr("%1 kB").arg(Number(bytes / 1024).toLocaleString(locale, 'f', 0))
        return qsTr("%1 MB").arg(Number(bytes / (1024 * 1024)).toLocaleString(locale, 'f', 1))
    }

    function languageLabel(entry) {

        void Qt.uiLanguage

        if (!entry) {
            return ""
        }

        return entry.code === "" ? qsTr("Same as this computer") : entry.name
    }

    function indexOfLanguage(entries, code) {
        for (let index = 0; index < entries.length; ++index) {
            if (entries[index].code === code) {
                return index
            }
        }

        return 0
    }

    function layoutLabel(value) {
        return shapeWords.labelFor(value)
    }

    function layoutWord(value) {
        return shapeWords.wordFor(value)
    }

    ShapeWords {
        id: shapeWords
    }

    function themeLabel(value) {

        void Qt.uiLanguage

        switch (value) {
        case SettingsViewModel.Theme.Light:
            return qsTr("Light")
        case SettingsViewModel.Theme.Dark:
            return qsTr("Dark")
        default:
            return qsTr("Same as this computer")
        }
    }

    property bool advancedShown: false

    property bool providerExtrasShown: false

    function transportLabel(value) {

        switch (value) {
        case AccountsViewModel.Transport.Tcp:
            return "TCP"
        case AccountsViewModel.Transport.Tls:
            return "TLS"
        default:
            return "UDP"
        }
    }

    function encryptionLabel(value) {

        void Qt.uiLanguage

        switch (value) {
        case AccountsViewModel.Encryption.Off:

            return qsTr("Off", "call encryption")
        case AccountsViewModel.Encryption.Required:
            return qsTr("Required")
        default:
            return qsTr("If the server supports it")
        }
    }

    function faultText(value) {

        void Qt.uiLanguage

        switch (value) {
        case AccountsViewModel.MissingUsername:
            return qsTr("This account needs a username.")
        case AccountsViewModel.MissingDomain:
            return qsTr("This account needs a server address.")
        case AccountsViewModel.InvalidPort:
            return qsTr("A port is a number from 1 to 65535.")
        case AccountsViewModel.InvalidExpiry:
            return qsTr("Re-registering happens between 30 seconds and 24 hours apart.")
        default:
            return ""
        }
    }

    function selectedLabel() {
        const rows = accounts.accounts
        for (let index = 0; index < rows.length; ++index) {
            if (rows[index].id === accounts.selectedId) {
                return rows[index].label
            }
        }
        return ""
    }

    function stateIs(entry, state) {
        return entry !== undefined && entry.state === state
    }

    function stateColor(entry) {
        if (entry === undefined || !entry.enabled) {
            return Mark.muted
        }

        switch (entry.state) {
        case AccountsViewModel.Registered:
            return Mark.good
        case AccountsViewModel.Failed:
            return Mark.bad
        case AccountsViewModel.Registering:
        case AccountsViewModel.Unregistering:
            return Mark.warn(Material.theme)
        default:
            return Mark.muted
        }
    }

    function rowNote(entry) {

        void Qt.uiLanguage

        if (entry === undefined) {
            return ""
        }

        if (entry.leaving) {
            return qsTr("Leaving when the call on it ends")
        }
        if (!entry.enabled) {
            return qsTr("Not used")
        }
        if (entry.state === AccountsViewModel.Failed) {
            return settingsBody.registrationProblem.length > 0
                ? settingsBody.registrationProblem
                : qsTr("Not connected")
        }

        return entry.label === entry.uri ? "" : entry.uri
    }

    function dtmfShort(value) {
        void Qt.uiLanguage

        return value === AccountsViewModel.Dtmf.SipInfo ? qsTr("SIP INFO")
                                                        : qsTr("Audio stream")
    }

    function dtmfLabel(value) {

        void Qt.uiLanguage

        return value === AccountsViewModel.Dtmf.SipInfo
            ? qsTr("As SIP INFO messages")
            : qsTr("In the audio stream (RFC 2833)")
    }

    function codecLabel(entry) {

        void Qt.uiLanguage

        if (!entry) {
            return ""
        }
        if (entry.clockRate <= 0) {
            return entry.name
        }

        const rate = Math.round(entry.clockRate / 1000)

        if (entry.channels === 2) {
            return qsTr("%1 · %2 kHz · stereo").arg(entry.name).arg(rate)
        }
        if (entry.channels > 2) {
            return qsTr("%1 · %2 kHz · %n channels", "", entry.channels)
                .arg(entry.name).arg(rate)
        }
        return qsTr("%1 · %2 kHz").arg(entry.name).arg(rate)
    }

    function deviceLabel(entry, emptyChoice) {

        void Qt.uiLanguage

        if (!entry) {
            return ""
        }
        if (entry.name === "") {
            return emptyChoice !== undefined ? emptyChoice : qsTr("System default")
        }

        const shown = entry.description !== "" ? entry.description : entry.name
        return entry.present ? shown : qsTr("%1 (not connected)").arg(shown)
    }

    onFocusedChanged: if (settingsBody.focused) settings.refreshDevices()

    function pendingText(count, schedule) {

        void Qt.uiLanguage

        switch (schedule) {
        case SettingsViewModel.WhenCallsEnd:
            return qsTr("%n change(s) will apply when your calls end.", "", count)
        case SettingsViewModel.NextCall:
            return qsTr("%n change(s) will apply to your next call.", "", count)
        case SettingsViewModel.NextStart:
            return qsTr("%n change(s) will apply the next time the app starts.", "", count)
        }
        return ""
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        focus: true

        Keys.onEscapePressed: (event) => {
            if (accounts.editing) {
                accounts.cancelEdit()
            }
            event.accepted = true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Pane {
                Layout.preferredWidth: 160
                Layout.fillHeight: true
                padding: 0

                background: Rectangle {
                    color: Material.foreground
                    opacity: 0.04
                }

                ScrollView {
                    id: sectionScroller

                    anchors.fill: parent
                    clip: true

                    contentWidth: availableWidth

                    ScrollBar.vertical: ThinScrollBar {
                        parent: sectionScroller
                        x: sectionScroller.mirrored ? 0 : sectionScroller.width - width
                        y: sectionScroller.topPadding
                        height: sectionScroller.availableHeight
                    }

                    ColumnLayout {
                        width: sectionScroller.availableWidth
                        spacing: 0

                        Repeater {
                            model: settingsBody.sectionKeys

                            ColumnLayout {
                                id: sectionRow

                                required property string modelData
                                required property int index

                                readonly property bool opensABand: {
                                    const keys = settingsBody.sectionKeys
                                    if (sectionRow.index <= 0 || sectionRow.index >= keys.length) {
                                        return false
                                    }
                                    return uiRegistry.groupOf(keys[sectionRow.index])
                                           !== uiRegistry.groupOf(keys[sectionRow.index - 1])
                                }

                                Layout.fillWidth: true
                                spacing: 0

                                Item {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 17
                                    visible: sectionRow.opensABand

                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.leftMargin: 16
                                        anchors.rightMargin: 12
                                        anchors.verticalCenter: parent.verticalCenter
                                        height: 1
                                        color: Material.foreground
                                        opacity: 0.15
                                    }
                                }

                                ItemDelegate {
                                    id: sectionItem

                                    readonly property string modelData: sectionRow.modelData

                                    readonly property bool current: settingsBody.section === modelData

                                    Layout.fillWidth: true
                                    leftPadding: 16
                                    rightPadding: 12

                                    topPadding: 12
                                    bottomPadding: 12
                                    onClicked: settingsBody.section = modelData

                                    contentItem: Label {

                                        text: {
                                            void uiRegistry.revision
                                            return uiRegistry.titleOf(sectionItem.modelData)
                                        }
                                        elide: Text.ElideRight
                                        verticalAlignment: Text.AlignVCenter
                                        color: sectionItem.current ? Material.accentColor
                                                                   : Material.foreground
                                        opacity: sectionItem.current ? 1.0 : 0.85
                                    }

                                    background: Rectangle {
                                        color: sectionItem.current
                                               ? Qt.rgba(Material.accentColor.r, Material.accentColor.g,
                                                         Material.accentColor.b, 0.12)
                                               : (sectionItem.hovered
                                                  ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                                                            Material.foreground.b, 0.08)
                                                  : "transparent")

                                        Rectangle {
                                            width: 3
                                            height: parent.height
                                            anchors.right: parent.right
                                            color: Material.accentColor
                                            visible: sectionItem.current
                                        }
                                    }
                                }
                            }
                        }

                    }
                }
            }

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                color: Material.foreground
                opacity: 0.15
            }

            ScrollView {
                id: settingsScroller

                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                contentWidth: availableWidth

                ScrollBar.vertical: ThinScrollBar {
                    parent: settingsScroller
                    x: settingsScroller.mirrored ? 0 : settingsScroller.width - width
                    y: settingsScroller.topPadding
                    height: settingsScroller.availableHeight
                }

                ColumnLayout {
                    width: parent.width
                    spacing: 12

                    Repeater {
                        model: uiRegistry.settingsSections

                        SlotLoader {
                            id: sectionLoader

                            required property string modelData

                            property bool everWanted: settingsBody.everyPage
                                                      || sectionLoader.chosen

                            readonly property bool chosen:
                                settingsBody.section === sectionLoader.modelData

                            what: sectionLoader.modelData
                            page: settingsBody
                            noun: "the settings section"
                            aSection: true

                            onChosenChanged: {
                                if (sectionLoader.chosen) {
                                    sectionLoader.everWanted = true
                                }
                            }

                            active: sectionLoader.everWanted

                            Layout.fillWidth: true

                            Layout.margins: 16

                            visible: sectionLoader.chosen

                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            visible: settings.pendingCount > 0
            color: Material.foreground
            opacity: 0.15
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            visible: settings.pendingCount > 0
            spacing: 12

            Label {
                Layout.fillWidth: true
                text: settingsBody.pendingText(settings.pendingCount, settings.pendingSchedule)
                wrapMode: Text.WordWrap
                font.pixelSize: 13
            }

            Button {
                flat: true
                text: qsTr("Undo it")
                onClicked: settings.withdrawPending()
            }
        }
    }

    ConfirmDialog {
        id: disablePluginDialog

        destructive: false

        property string pluginId: ""
        property string pluginName: ""
        property var going: []

        property var doingLess: []
        title: qsTr("Turn off %1?").arg(disablePluginDialog.pluginName)
        actionText: qsTr("Turn off")
        onAccepted: {
            plugins.setEnabled(disablePluginDialog.pluginId, false)
        }

        Label {
            Layout.fillWidth: true
            visible: disablePluginDialog.going.length > 0
            wrapMode: Text.WordWrap
            text: qsTr("These need it and go off with it: %1.")
                      .arg(disablePluginDialog.going.join(", "))
        }
        Label {
            Layout.fillWidth: true
            visible: disablePluginDialog.doingLess.length > 0
            wrapMode: Text.WordWrap
            text: qsTr("These go on running and do less without it: %1.")
                      .arg(disablePluginDialog.doingLess.join(", "))
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            opacity: 0.75
            text: qsTr("What they have already stored stays where it is. Turning them back on brings it back.")
        }
    }

    ConfirmDialog {
        id: removeKeyDialog

        title: qsTr("Delete this button?")
        actionText: qsTr("Delete")
        onAccepted: {
            buttons.removeButton(buttons.selectedId)
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("The button and what it watches will be deleted. Deleting cannot be undone.")
        }
    }

    ConfirmDialog {
        id: removeAutomationDialog

        property string entryId: ""
        property string entryName: ""
        property bool shipped: false
        function ask(id) {
            const row = automations.rowFor(id)
            removeAutomationDialog.entryId = id
            removeAutomationDialog.entryName = row.name
            removeAutomationDialog.shipped = row.builtIn === true
            removeAutomationDialog.open()
        }
        title: qsTr("Delete this rule?")
        actionText: qsTr("Delete")
        onAccepted: {
            automations.remove(removeAutomationDialog.entryId)
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: removeAutomationDialog.shipped
                  ? qsTr("%1 will be deleted. It came with the program and will not come back — if you only want it to stop, switch it off instead. Conversations it has already acted on keep what it did. Deleting cannot be undone.").arg(removeAutomationDialog.entryName)
                  : qsTr("%1 will be deleted. Conversations it has already acted on keep what it did. Deleting cannot be undone.").arg(removeAutomationDialog.entryName)
        }
    }

    ConfirmDialog {
        id: removeTemplateDialog

        property string entryId: ""
        property string entryName: ""
        property bool shipped: false
        function ask(id) {
            const row = templates.rowFor(id)
            removeTemplateDialog.entryId = id
            removeTemplateDialog.entryName = row.name
            removeTemplateDialog.shipped = row.builtIn === true
            removeTemplateDialog.open()
        }
        title: qsTr("Delete this prompt?")
        actionText: qsTr("Delete")
        onAccepted: {
            templates.remove(removeTemplateDialog.entryId)
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap

            text: removeTemplateDialog.shipped
                  ? qsTr("%1 will be deleted. It came with the program and will not come back — if you only want it out of the way, switch it off instead. What it has already written is kept. Deleting cannot be undone.").arg(removeTemplateDialog.entryName)
                  : qsTr("%1 will be deleted. What it has already written is kept. Deleting cannot be undone.").arg(removeTemplateDialog.entryName)
        }
    }

    ConfirmDialog {
        id: removeEntryDialog

        property string entryId: ""
        property string entryTitle: ""

        property bool used: false

        title: qsTr("Delete this entry?")
        actionText: qsTr("Delete")

        destructive: !removeEntryDialog.used
        onAccepted: {
            if (removeEntryDialog.used) {
                taxonomy.setRetired(true)
                taxonomy.save()
            } else {
                taxonomy.remove(removeEntryDialog.entryId)
            }
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: removeEntryDialog.used
                  ? qsTr("Conversations already carry %1, so it is hidden rather than thrown away: it is no longer offered and no longer given to the models, and the conversations that have it go on showing it. Turn on Show deleted to find it again.").arg(removeEntryDialog.entryTitle)
                  : qsTr("%1 will be deleted. Nothing refers to it yet, so nothing else changes. Deleting cannot be undone.").arg(removeEntryDialog.entryTitle)
        }
    }

    ConfirmDialog {
        id: removeProviderDialog

        property var about: providers

        readonly property bool recogniser: about !== undefined && about.listens === true

        function ask(which) {
            removeProviderDialog.about = which
            removeProviderDialog.open()
        }

        title: recogniser
               ? qsTr("Delete this recogniser?")

               : qsTr("Delete this model?")
        actionText: qsTr("Delete")
        onAccepted: {
            removeProviderDialog.about.remove(removeProviderDialog.about.editingId)
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: removeProviderDialog.recogniser
                  ? qsTr("%1 will be deleted and its key taken out of wherever this phone kept it. Transcripts it made are kept. Deleting cannot be undone.").arg(removeProviderDialog.about.name)
                  : qsTr("%1 will be deleted and its key taken out of wherever this phone kept it. What it has already written is kept. Deleting cannot be undone.").arg(removeProviderDialog.about.name)
        }
    }

    ConfirmDialog {
        id: removeAccountDialog

        title: qsTr("Delete this account?")
        actionText: qsTr("Delete")
        onAccepted: accounts.removeAccount(accounts.selectedId)

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("%1 will be deleted and its password taken out of wherever this phone kept it. Deleting cannot be undone.").arg(settingsBody.selectedLabel())
        }
    }

    ConfirmDialog {
        id: clearLogDialog

        title: qsTr("Clear the log?")
        actionText: qsTr("Clear")
        onAccepted: {
            settings.clearLog()
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Everything written down so far goes, and it cannot be brought back. Clearing it before you reproduce a problem is what makes the file readable; clearing it afterwards throws the evidence away.")
        }
    }

    ConfirmDialog {
        id: restoringDictionaries

        title: qsTr("Start again?")
        actionText: qsTr("Restore")
        onAccepted: {
            taxonomy.restoreEverything()
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Every category, tag, red flag and instruction goes — the ones you wrote as well as the ones you changed — and the ones the program came with take their place. What your conversations are already filed under, tagged with and flagged for is untouched, but a code you had added will no longer be in the lists. Restoring cannot be undone.")
        }
    }

    ConfirmDialog {
        id: restoringEverything

        title: qsTr("Start again?")
        actionText: qsTr("Restore")
        onAccepted: {
            templates.restoreEverything()
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Every prompt and every rule goes — the ones you wrote as well as the ones you changed — and the ones the program came with take their place. Your language models, your dictionaries and everything the models have already written are left alone. Restoring cannot be undone.")
        }
    }
}
