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

import Qt.labs.platform as Platform
import QtQuick.Dialogs as Dialogs

import Softphone

Item {
    id: recordingsBody

    objectName: "recordingsBody"

    function squeezeTheConversation() {
        recordingsBody.tableHeight = 100000
    }

    function stretchTheConversation() {
        recordingsBody.tableHeight = 1
    }

    function shareTheWindow() {
        recordingsBody.tableHeight = 0
    }

    implicitWidth: 760

    property int leastWidth: 560

    readonly property string drawing: "library"

    property bool assembled: false

    Component.onCompleted: {
        recordingsBody.assembled = true
        Qt.callLater(transcriptPanel.gatherReadables)
    }

    property string glyphs: ""

    property bool onScreen: false

    property bool kept: false

    property bool settling: false

    property bool focused: false

    readonly property bool buildEverything:
        Qt.application.arguments.indexOf("--build-every-page") >= 0

    Timer {
        running: recordingsBody.buildEverything
        interval: 250
        repeat: true
        triggeredOnStart: true

        onTriggered: {
            let waiting = false
            for (const one of transcriptPanel.readables) {
                if (!one || one.entries.length === 0) {
                    waiting = true
                    continue
                }
                if (one.openId.length === 0) {
                    one.open(one.entries[0].id)
                    waiting = true
                }
            }
            if (!waiting) {
                running = false
            }
        }
    }

    function glyph(name) {
        return recordingsBody.glyphs === "" ? "" : recordingsBody.glyphs + "/" + name + ".svg"
    }

    property real anchorMs: 0
    property real anchorAt: 0

    readonly property int aJumpRatherThanJitter: 400

    onPlayheadChanged: {
        for (const readable of transcriptPanel.readables) {
            if (readable) {
                readable.playhead = Math.round(recordingsBody.playhead)
            }
        }
    }

    property real playhead: 0

    function movePlayhead() {
        if (!recordings.playing) {
            recordingsBody.playhead = recordings.positionMs
            return
        }

        const since = Date.now() - recordingsBody.anchorAt
        const wanted = Math.min(recordings.durationMs,
                                recordingsBody.anchorMs + since * recordings.speed)

        recordingsBody.playhead =
            Math.abs(wanted - recordingsBody.playhead) > recordingsBody.aJumpRatherThanJitter
                ? wanted
                : Math.max(recordingsBody.playhead, wanted)
    }

    FrameAnimation {
        running: recordings.playing && recordingsBody.onScreen
        onTriggered: recordingsBody.movePlayhead()
    }

    Connections {
        target: recordings

        function onPlaybackChanged() {
            recordingsBody.anchorMs = recordings.positionMs
            recordingsBody.anchorAt = Date.now()

            if (!recordings.playing) {
                recordingsBody.movePlayhead()
            }
        }

        function onSought() {
            recordingsBody.anchorMs = recordings.positionMs
            recordingsBody.anchorAt = Date.now()
            recordingsBody.playhead = recordings.positionMs
        }
    }

    readonly property bool spaceIsSomebodyElses: {
        const item = recordingsBody.activeFocusItem
        if (item === null) {
            return false
        }
        if (item instanceof TextInput || item instanceof TextEdit) {
            return true
        }
        for (let at = item; at; at = at.parent) {
            if (at === recordingsBody.Overlay.overlay) {
                return true
            }
        }
        return false
    }

    Shortcut {
        sequence: "Space"
        context: Qt.WindowShortcut
        enabled: recordingsBody.onScreen && recordings.playerShown
                 && !recordingsBody.spaceIsSomebodyElses
        onActivated: recordings.togglePlay()
    }

    readonly property int filterWidth: 200

    property bool resizing: false

    Timer {
        id: steadying

        interval: 150
        onTriggered: recordingsBody.resizing = false
    }

    function moving() {

        if (!recordingsBody.onScreen || recordingsBody.settling) {
            return
        }
        recordingsBody.resizing = true
        steadying.restart()
    }

    Connections {
        target: recordingsBody

        function onWidthChanged() { recordingsBody.moving() }
        function onHeightChanged() { recordingsBody.moving() }
    }

    readonly property string openRecordingId: recordings.openId

    readonly property int filterHeight: 44

    readonly property int controlTarget: 40
    readonly property int controlCircle: 32
    readonly property int rowGlyph: 18

    function laidOver(top, bottom) {
        const a = top.a
        return Qt.rgba(top.r * a + bottom.r * (1 - a),
                       top.g * a + bottom.g * (1 - a),
                       top.b * a + bottom.b * (1 - a),
                       1)
    }

    component RoundGlyphButton: RowButton {

        property string picture: ""
        property string hint: ""

        glyph: picture
        text: hint

        glyphs: recordingsBody.glyphs
        box: recordingsBody.controlTarget
        target: recordingsBody.controlTarget
        circle: recordingsBody.controlCircle
        pictureSize: recordingsBody.rowGlyph
    }

    component PlayerGlyph: RoundGlyphButton {
        Layout.alignment: Qt.AlignRight

        box: 30
        target: 30
        circle: 26
        pictureSize: 16

        focusPolicy: Qt.NoFocus
    }

    component PlayerChoice: Button {

        property string hint: ""

        flat: true
        topInset: 0
        bottomInset: 0
        topPadding: 2
        bottomPadding: 2
        Layout.alignment: Qt.AlignHCenter

        leftPadding: 4
        rightPadding: 4
        implicitHeight: 30
        font.pixelSize: 11

        focusPolicy: Qt.NoFocus

        Hint {
            owner: parent
            visible: parent.hovered
            text: parent.hint
        }
    }

    property real tableHeight: 0

    readonly property bool split: tableHeight > 0

    readonly property real roomForThePair: {
        let left = windowColumn.height
        let items = 0

        for (const item of windowColumn.children) {
            if (!item.visible) {
                continue
            }
            items += 1
            if (item !== libraryArea && item !== conversationPanel) {
                left -= item.height
            }
        }
        return Math.max(0, left - Math.max(0, items - 1) * windowColumn.spacing)
    }

    readonly property real libraryFloor: 3 * 56

    readonly property real forTheLibrary: SplitPanes.forTheFirst(
        recordingsBody.roomForThePair,
        recordingsBody.libraryFloor,
        conversationPanel.visible ? conversationPanel.floor : 0,
        conversationPanel.visible && conversationPanel.spread
            ? (recordingsBody.split ? recordingsBody.tableHeight : -1)
            : recordingsBody.roomForThePair)

    readonly property color faintColour: Qt.rgba(Material.foreground.r, Material.foreground.g,
                                                 Material.foreground.b, 0.14)

    readonly property var textLocale: Qt.uiLanguage ? Qt.locale(Qt.uiLanguage) : Qt.locale()

    readonly property var formatLocale: Qt.locale()

    onOnScreenChanged: if (recordingsBody.onScreen) recordings.refresh()

    function stamp(ms) {
        void Qt.uiLanguage
        return new Date(ms).toLocaleString(recordingsBody.formatLocale, Locale.ShortFormat)
    }

    function size(bytes) {
        void Qt.uiLanguage
        return qsTr("%1 MB").arg((bytes / (1024 * 1024)).toFixed(1))
    }

    ColumnLayout {
        id: windowColumn

        anchors.fill: parent

        anchors.topMargin: 12
        anchors.bottomMargin: 12
        anchors.leftMargin: 0
        anchors.rightMargin: 0

        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            Layout.leftMargin: 16
            Layout.rightMargin: 16
            spacing: 6

            PickerBox {
                id: kindPicker

                implicitHeight: recordingsBody.filterHeight

                readonly property int fromTheHistory: -1

                readonly property var values: recordings.callFilter.length > 0
                    ? [kindPicker.fromTheHistory,
                       RecordingsViewModel.Filter.All,
                       RecordingsViewModel.Filter.Incoming,
                       RecordingsViewModel.Filter.Outgoing,
                       RecordingsViewModel.Filter.Captured,
                       RecordingsViewModel.Filter.Imported,
                       RecordingsViewModel.Filter.Pinned,
                       RecordingsViewModel.Filter.Conference]
                    : [RecordingsViewModel.Filter.All,
                       RecordingsViewModel.Filter.Incoming,
                       RecordingsViewModel.Filter.Outgoing,
                       RecordingsViewModel.Filter.Captured,
                       RecordingsViewModel.Filter.Imported,
                       RecordingsViewModel.Filter.Pinned,
                       RecordingsViewModel.Filter.Conference]

                readonly property int current: recordings.callFilter.length > 0
                                               ? kindPicker.fromTheHistory
                                               : recordings.filter

                Layout.fillWidth: true
                Layout.preferredWidth: recordingsBody.filterWidth
                Layout.minimumWidth: implicitWidth

                model: kindPicker.values
                displayText: recordingsBody.filterClosed(kindPicker.current)

                delegate: ItemDelegate {
                    id: kindEntry

                    required property var modelData
                    required property int index

                    width: kindPicker.width
                    text: recordingsBody.filterLabel(kindEntry.modelData)
                    highlighted: kindPicker.highlightedIndex === kindEntry.index

                    enabled: kindEntry.modelData !== kindPicker.fromTheHistory
                }

                onActivated: (chosen) => {
                    const wanted = kindPicker.values[chosen]
                    if (wanted === kindPicker.fromTheHistory) {
                        return
                    }
                    recordings.showEverything()
                    recordings.filter = wanted
                }

                Binding {
                    target: kindPicker
                    property: "currentIndex"
                    value: kindPicker.values.indexOf(kindPicker.current)
                    restoreMode: Binding.RestoreNone
                }
            }

            PickerBox {
                id: datePicker

                implicitHeight: recordingsBody.filterHeight

                readonly property var values: [RecordingsViewModel.Period.Any,
                                               RecordingsViewModel.Period.Today,
                                               RecordingsViewModel.Period.Yesterday,
                                               RecordingsViewModel.Period.Week,
                                               RecordingsViewModel.Period.Range]

                Layout.fillWidth: true
                Layout.preferredWidth: recordingsBody.filterWidth
                Layout.minimumWidth: implicitWidth

                model: datePicker.values

                displayText: recordingsBody.periodLabel(recordings.period)

                delegate: ItemDelegate {
                    id: dateEntry

                    required property var modelData
                    required property int index

                    width: datePicker.width
                    text: recordingsBody.periodEntry(dateEntry.modelData)
                    highlighted: datePicker.highlightedIndex === dateEntry.index
                }

                onActivated: (chosen) => {
                    const wanted = datePicker.values[chosen]
                    if (wanted === RecordingsViewModel.Period.Range) {
                        calendar.open()
                        return
                    }
                    recordings.showPeriod(wanted)
                }

                Binding {
                    target: datePicker
                    property: "currentIndex"
                    value: datePicker.values.indexOf(recordings.period)
                    restoreMode: Binding.RestoreNone
                }
            }

            PickerBox {
                id: categoryPicker

                implicitHeight: recordingsBody.filterHeight

                readonly property var values: [""].concat(recordings.categories)

                visible: recordings.categories.length > 0
                Layout.fillWidth: true
                Layout.preferredWidth: recordingsBody.filterWidth
                Layout.minimumWidth: implicitWidth
                model: values
                displayText: recordingsBody.categoryClosed(model[currentIndex])

                delegate: ItemDelegate {
                    required property var modelData
                    required property int index

                    width: categoryPicker.width
                    text: recordingsBody.categoryLabel(modelData)
                    highlighted: categoryPicker.highlightedIndex === index
                }

                currentIndex: Math.max(0, values.indexOf(recordings.category))
                onActivated: (chosen) => recordings.showCategory(categoryPicker.values[chosen])
            }

            PickerBox {
                id: labelPicker

                implicitHeight: recordingsBody.filterHeight

                readonly property var values: [{kind: "", code: ""}]
                    .concat(recordings.tags.map((code) => ({kind: "tag", code: code})))
                    .concat(recordings.redFlags.map((code) => ({kind: "red_flag", code: code})))

                visible: values.length > 1
                Layout.fillWidth: true
                Layout.preferredWidth: recordingsBody.filterWidth
                Layout.minimumWidth: implicitWidth
                model: values
                displayText: recordingsBody.labelClosed(model[currentIndex])

                delegate: ItemDelegate {
                    required property var modelData
                    required property int index

                    width: labelPicker.width
                    text: recordingsBody.labelText(modelData)
                    highlighted: labelPicker.highlightedIndex === index

                    Material.foreground: modelData.kind === "red_flag"
                                         ? Ink.bad(Material.theme)
                                         : undefined
                }

                currentIndex: Math.max(0, values.findIndex(
                                           (one) => one.kind === "red_flag"
                                                    ? one.code === recordings.redFlag
                                                    : one.kind === "tag"
                                                      ? one.code === recordings.tag
                                                      : recordings.tag.length === 0
                                                        && recordings.redFlag.length === 0))
                onActivated: (chosen) => {
                    const wanted = labelPicker.values[chosen]
                    recordings.showTag(wanted.kind === "tag" ? wanted.code : "")
                    recordings.showRedFlag(wanted.kind === "red_flag" ? wanted.code : "")
                }
            }

            TextField {
                id: searchField

                TextMenu {}

                implicitHeight: recordingsBody.filterHeight

                Layout.fillWidth: true
                Layout.preferredWidth: recordingsBody.filterWidth

                Layout.minimumWidth: 120

                placeholderText: qsTr("Search")

                text: recordings.search

                rightPadding: searchClear.visible
                              ? searchClear.width + 12 : searchField.leftPadding

                onTextEdited: settling.restart()

                onAccepted: {
                    settling.stop()
                    recordings.search = searchField.text
                }

                Timer {
                    id: settling
                    interval: 200
                    onTriggered: recordings.search = searchField.text
                }

                Item {
                    id: searchClear

                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    implicitWidth: 16
                    implicitHeight: 16
                    visible: searchField.text.length > 0

                    Glyph {
                        anchors.fill: parent
                        source: recordingsBody.glyph("dismiss")
                        color: Material.foreground
                        opacity: clearing.hovered ? 0.9 : 0.45
                    }

                    HoverHandler {
                        id: clearing
                        cursorShape: Qt.ArrowCursor
                    }

                    TapHandler {
                        onTapped: {
                            settling.stop()
                            searchField.text = ""
                            recordings.search = ""
                            searchField.forceActiveFocus()
                        }
                    }

                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Clear the search")
                }

                Connections {
                    target: recordings

                    function onFilterChanged() {

                        if (settling.running) {
                            return
                        }
                        if (recordings.search !== searchField.text) {
                            searchField.text = recordings.search
                        }
                    }
                }
            }

            RoundGlyphButton {
                id: libraryActs

                Layout.alignment: Qt.AlignVCenter
                picture: "more-vertical"

                hint: qsTr("Actions", "the acts this window offers, in the menu its corner opens")
                onClicked: recordingsBody.showTheLibraryMenu()
            }
        }

        Label {
            id: importNote

            property int brought: 0
            property int refused: 0

            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            visible: text.length > 0
            wrapMode: Text.WordWrap
            font.pixelSize: 12

            color: importNote.refused > 0 ? Ink.warn(Material.theme) : Material.foreground
            opacity: importNote.refused > 0 ? 1 : 0.6

            text: ""

            Timer {
                id: importNoteFades
                interval: 8000
                onTriggered: importNote.text = ""
            }

            Connections {
                target: recordings

                function onImported(brought, refused) {
                    importNote.brought = brought
                    importNote.refused = refused

                    if (refused === 0) {

                        importNote.text = qsTr("%n conversation(s) imported.", "", brought)
                    } else if (brought === 0) {

                        importNote.text = qsTr(
                            "%n file(s) could not be read as a recording.", "", refused)
                    } else {

                        importNote.text = qsTr("%1 %2")
                            .arg(qsTr("%n conversation(s) imported.", "", brought))
                            .arg(qsTr("%n file(s) could not be read as a recording.", "", refused))
                    }

                    importNoteFades.restart()
                }
            }
        }

        ColumnLayout {
            id: libraryArea

            Layout.fillWidth: true

            Layout.fillHeight: true
            Layout.preferredHeight: recordingsBody.forTheLibrary
            Layout.maximumHeight: recordingsBody.forTheLibrary
            Layout.minimumHeight: 0

            clip: true

            spacing: 0

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.fillHeight: true
                visible: !recordings.available
                verticalAlignment: Text.AlignTop
                text: qsTr("The recordings cannot be listed: the database would not open. The phone still records, and the files are where the settings say.")
                wrapMode: Text.WordWrap
                opacity: 0.7
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.fillHeight: true
                visible: recordings.available && recordings.count === 0
                verticalAlignment: Text.AlignTop
                text: recordings.filtered
                      ? qsTr("Nothing matches what you are looking for.")
                      : qsTr("Nothing has been recorded yet. Press Record during a call, or set the phone to record every call in Settings.")
                wrapMode: Text.WordWrap
                opacity: 0.7
            }

            ListView {
                id: table

                Layout.fillWidth: true
                Layout.fillHeight: true

                Layout.preferredHeight: 0

                clip: true
                visible: recordings.count > 0

                model: recordings.rows

                spacing: 0

                footer: Item {
                    width: table.width

                    height: older.visible ? older.height + 4 : 0

                    Button {
                        id: older

                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 4
                        width: parent.width - 32
                        height: 36
                        flat: true
                        topInset: 0
                        bottomInset: 0
                        visible: recordings.more
                        enabled: !recordings.fetching
                        text: recordings.fetching ? qsTr("Fetching…")
                                                  : qsTr("Show older conversations")
                        onClicked: recordings.loadMore()
                    }
                }

                onHeightChanged: if (!recordingsBody.resizing) {
                                     Qt.callLater(recordingsBody.keepTheOpenRowVisible)
                                 }

                ScrollBar.vertical: ThinScrollBar {}

                delegate: ItemDelegate {
                    id: row

                    required property var modelData

                    RowMenu {
                        acts: acts
                        number: row.modelData.number
                    }

                    width: table.width

                    leftPadding: 16
                    rightPadding: 16

                    highlighted: recordings.openId === row.modelData.id
                    hoverEnabled: true

                    Rectangle {
                        anchors.fill: parent
                        z: -1
                        visible: !row.highlighted && recordings.openSeance.length > 0
                                 && row.modelData.seanceId === recordings.openSeance
                        color: Material.accentColor
                        opacity: 0.12
                    }

                    implicitHeight: Math.max(implicitContentHeight, recordingsBody.controlTarget)
                                    + topPadding + bottomPadding

                    readonly property bool showsControls: row.hovered

                    onClicked: {
                        if (recordingsBody.pressIsForTheTable()) {
                            recordings.open(row.modelData.id)
                        }
                    }

                    Component.onCompleted: {
                        if (recordingsBody.buildEverything && recordings.openId.length === 0) {
                            recordings.open(row.modelData.id)
                        }
                    }

                    TapHandler {
                        onDoubleTapped: {
                            if (recordingsBody.pressIsForTheTable()) {
                                recordings.play(row.modelData.id)
                            }
                        }
                    }

                    contentItem: RowLayout {
                        spacing: 10

                        Glyph {
                            Layout.alignment: Qt.AlignVCenter
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20

                            source: recordingsBody.glyph(
                                        RecordingGlyph.forRecording(row.modelData))

                            color: row.modelData.pinned ? Mark.warn(Material.theme)
                                                        : Material.foreground
                            opacity: row.modelData.pinned ? 1.0 : 0.6
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    Layout.fillWidth: true

                                    Layout.maximumWidth: Math.ceil(implicitWidth)
                                    elide: Text.ElideRight
                                    text: {
                                        const called = row.modelData.name.length > 0
                                                       ? row.modelData.name : row.modelData.number
                                        return row.modelData.title.length > 0
                                               ? qsTr("%1 · %2").arg(called).arg(row.modelData.title)
                                               : called
                                    }
                                    font.weight: Font.Medium
                                }

                                Label {
                                    visible: row.modelData.parties > 1
                                    text: qsTr("%1 people").arg(row.modelData.parties + 1)
                                    font.pixelSize: 12
                                    opacity: 0.6
                                }

                                Item {
                                    Layout.fillWidth: true
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                text: row.modelData.summary.length > 0
                                      ? recordingsBody.stamp(row.modelData.startedAtMs)
                                        + "  ·  " + row.modelData.summary
                                      : recordingsBody.stamp(row.modelData.startedAtMs)
                                font.pixelSize: 12
                                opacity: 0.6
                            }
                        }

                        Label {
                            Layout.preferredWidth: 170
                            Layout.alignment: Qt.AlignVCenter
                            visible: text.length > 0
                            text: recordingsBody.filedAs(row.modelData,
                                                           Ink.bad(Material.theme))
                            textFormat: Text.StyledText
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            font.pixelSize: 12
                            opacity: 0.85
                        }

                        Label {
                            visible: !row.modelData.hasAudio
                            text: qsTr("audio deleted")
                            font.pixelSize: 12
                            opacity: 0.6
                        }

                        RowLayout {
                            Layout.fillWidth: false
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 5
                            visible: row.modelData.unfinished

                            Rectangle {
                                Layout.alignment: Qt.AlignVCenter
                                implicitWidth: 7
                                implicitHeight: 7
                                radius: width / 2
                                color: Material.accent
                            }

                            Label {
                                text: qsTr("recording")
                                font.pixelSize: 12
                                color: Material.accent
                            }
                        }

                        Label {
                            visible: !row.modelData.unfinished
                            text: Duration.text(row.modelData.durationMs)
                            font.pixelSize: 12
                            opacity: 0.7
                        }
                    }

                    Item {
                        id: acts

                        function offered() {
                            const out = []
                            for (const one of actLine.children) {
                                if (one.visible && one.text !== undefined && one.text !== "") {
                                    out.push(one)
                                }
                            }
                            return out
                        }

                        readonly property bool canExcuse:
                            row.modelData.external && row.modelData.number.length > 0
                            && settings.captureEnabled

                        readonly property bool canDelete: !row.modelData.unfinished

                        readonly property int fade: 28

                        readonly property color coat: {
                            let under = Material.background
                            if (row.highlighted) {
                                under = recordingsBody.laidOver(Material.listHighlightColor,
                                                                 under)
                            }
                            if (row.hovered) {
                                under = recordingsBody.laidOver(Material.rippleColor, under)
                            }
                            return under
                        }

                        z: 1
                        visible: row.showsControls

                        anchors.right: row.right
                        anchors.rightMargin: row.rightPadding
                        anchors.verticalCenter: row.verticalCenter

                        width: acts.fade + recordingsBody.controlTarget
                               * (2 + (acts.canExcuse ? 1 : 0) + (acts.canDelete ? 1 : 0))
                        height: recordingsBody.controlTarget

                        Rectangle {
                            x: acts.fade
                            width: parent.width - acts.fade
                            height: parent.height
                            color: acts.coat
                        }

                        Rectangle {
                            width: acts.fade
                            height: parent.height
                            gradient: Gradient {
                                orientation: Gradient.Horizontal

                                GradientStop {
                                    position: 0.0
                                    color: Qt.rgba(acts.coat.r, acts.coat.g, acts.coat.b, 0)
                                }

                                GradientStop { position: 1.0; color: acts.coat }
                            }
                        }

                        RowLayout {
                            id: actLine

                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            spacing: 0

                            RoundGlyphButton {
                                picture: "pin"

                                tint: row.modelData.pinned ? Mark.warn(Material.theme)
                                                           : Material.foreground
                                tintOpacity: row.modelData.pinned ? 1.0 : 0.5

                                hint: row.modelData.pinned ? qsTr("Stop keeping it")
                                                           : qsTr("Keep this one")
                                onClicked: recordings.togglePin(row.modelData.id)
                            }

                            RoundGlyphButton {
                                picture: "edit"

                                hint: qsTr("Rename")
                                onClicked: {
                                    naming.recordingId = row.modelData.id
                                    naming.capture = row.modelData.external
                                    naming.imported = row.modelData.imported
                                    naming.named = row.modelData.external
                                                   || row.modelData.imported
                                    naming.unfinished = row.modelData.unfinished
                                    naming.was = naming.named
                                                 ? row.modelData.remoteName
                                                 : row.modelData.title
                                    naming.open()
                                }
                            }

                            RoundGlyphButton {

                                visible: acts.canExcuse
                                picture: "call-prohibited"
                                hint: qsTr("Not a conversation")
                                onClicked: {
                                    notAConversation.recordingId = row.modelData.id

                                    notAConversation.what = row.modelData.number
                                    notAConversation.open()
                                }
                            }

                            RoundGlyphButton {

                                visible: acts.canDelete
                                picture: "delete"
                                hint: qsTr("Delete this recording")
                                onClicked: {
                                    deleting.recordingId = row.modelData.id
                                    deleting.open()
                                }
                            }
                        }
                    }
                }
            }
        }

        SplitGrip {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            visible: conversationPanel.visible && conversationPanel.spread

            sizeNow: libraryArea.height
            hairline: recordingsBody.faintColour

            onMoved: (size) => { recordingsBody.tableHeight = size }
        }

        Item {
            id: conversationPanel

            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            readonly property bool spread: transcriptPanel.visible

            readonly property real floor: (player.drawing ? player.fixedHeight
                                                          : player.implicitHeight)
                                          + (transcriptPanel.visible
                                             ? transcriptPanel.floor + conversationBody.spacing * 2
                                               + 1
                                             : 0)

            Layout.fillHeight: true
            Layout.preferredHeight: 0
            Layout.minimumHeight: 0

            clip: true

            visible: recordings.openId.length > 0
                     && (recordings.playerShown || recordings.readingShown)

            ColumnLayout {
                id: conversationBody

                anchors.fill: parent
                spacing: 8

                RowLayout {
                    id: player

                    Layout.fillWidth: true

                    readonly property int fixedHeight: 106

                    readonly property bool drawing: recordings.openHasAudio
                                                    && !recordings.openUnreadable
                                                    && !recordings.openUnfinished

                    Layout.preferredHeight: player.drawing ? player.fixedHeight : -1
                    Layout.minimumHeight: player.drawing ? player.fixedHeight : 0
                    Layout.maximumHeight: player.drawing ? player.fixedHeight
                                                         : Number.POSITIVE_INFINITY

                    Layout.fillHeight: false

                    spacing: 8

                    readonly property real fullMs: Math.max(1, recordings.durationMs)
                    readonly property real windowMs: Math.min(60000, player.fullMs)
                    readonly property real maxOffsetMs: Math.max(0, player.fullMs - player.windowMs)
                    readonly property bool zoomed: player.windowMs < player.fullMs

                    readonly property int sides: recordings.nearSide && recordings.farSide
                                                 ? 0 : (recordings.nearSide ? 1 : 2)

                    property real offsetMs: 0

                    function scrollTo(ms) {
                        player.offsetMs = Math.max(0, Math.min(player.maxOffsetMs, ms))
                    }

                    readonly property real head: recordingsBody.playhead

                    property bool following: true

                    function nudged() {
                        player.following = false
                    }

                    onHeadChanged: {
                        if (!recordings.playing) {
                            return
                        }
                        if (player.following) {
                            player.scrollTo(player.head - player.windowMs / 2)
                            return
                        }
                        if (player.head >= player.offsetMs + player.windowMs / 2
                                && player.head <= player.offsetMs + player.windowMs) {
                            player.following = true
                        }
                    }

                    Connections {
                        target: recordings

                        function onPlaybackChanged() {
                            if (!recordings.playing) {
                                player.following = true
                            }
                        }

                        function onSought() {
                            player.following = true
                        }
                    }

                    Connections {
                        target: recordings

                        function onOpenChanged() {
                            player.offsetMs = 0
                        }
                    }

                    ColumnLayout {
                        id: transport

                        Layout.fillHeight: true
                        visible: player.drawing

                        spacing: 6

                        Item { Layout.fillHeight: true }

                        Label {
                            Layout.alignment: Qt.AlignHCenter

                            text: Duration.text(recordingsBody.playhead)
                            font.pixelSize: 12
                            opacity: 0.7
                        }

                        Button {
                            Layout.alignment: Qt.AlignHCenter

                            readonly property bool sounding: recordings.playing
                                                             || recordings.starting

                            display: AbstractButton.IconOnly
                            icon.source: recordingsBody.glyph(sounding ? "hold" : "resume")
                            icon.width: 20
                            icon.height: 20
                            text: sounding ? qsTr("Pause") : qsTr("Play")
                            Accessible.name: text
                            onClicked: recordings.togglePlay()

                            Hint {
                                owner: parent
                                visible: parent.hovered
                                text: parent.text
                            }
                        }

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: Duration.text(recordings.durationMs)
                            font.pixelSize: 12
                            opacity: 0.7
                        }

                        Item { Layout.fillHeight: true }
                    }

                    Item {
                        id: waveStrip

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: player.drawing

                        clip: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            visible: !recordingsBody.resizing

                            Repeater {
                                model: 2

                                delegate: Item {
                                    id: channel

                                    required property int index

                                    property var values: []

                                    function readTheWave() {
                                        values = index === 0 ? recordings.nearWaveNow()
                                                             : recordings.farWaveNow()

                                        channel.paintedAs = ""
                                        picture.requestPaint()
                                    }

                                    Component.onCompleted: channel.readTheWave()

                                    readonly property bool audible: index === 0
                                                                    ? recordings.nearSide
                                                                    : recordings.farSide

                                    readonly property int barStep: 3
                                    readonly property int barWidth: 2

                                    function originPx(canvas) {
                                        return Math.round(player.offsetMs * canvas.width
                                                          / player.windowMs)
                                    }

                                    function pictureAs(canvas) {
                                        if (!canvas || canvas.width <= 0) {
                                            return ""
                                        }
                                        const perBar = channel.barStep * player.windowMs
                                                       / canvas.width
                                        return [canvas.width, canvas.height,
                                                channel.originPx(canvas),
                                                Math.floor(recordingsBody.playhead / perBar),
                                                Material.accent].join("/")
                                    }

                                    property string paintedAs: ""

                                    function repaintIfItMoved() {
                                        if (channel.pictureAs(picture) !== channel.paintedAs) {
                                            picture.requestPaint()
                                        }
                                    }

                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.minimumHeight: 40
                                    implicitHeight: 56

                                    opacity: channel.audible ? 1.0 : 0.3

                                    readonly property real headHere: (recordingsBody.playhead
                                                                      - player.offsetMs) / player.windowMs

                                    function drawInto(canvas) {
                                        channel.paintedAs = channel.pictureAs(canvas)

                                        const context = canvas.getContext("2d")
                                        context.reset()

                                        const middle = canvas.height / 2

                                        const values = channel.values
                                        if (!values || values.length === 0) {

                                            context.strokeStyle = Material.foreground
                                            context.globalAlpha = 0.15
                                            context.beginPath()
                                            context.moveTo(0, middle)
                                            context.lineTo(canvas.width, middle)
                                            context.stroke()
                                            return
                                        }

                                        const head = recordingsBody.playhead

                                        const perBar = channel.barStep * player.windowMs / canvas.width
                                        const first = Math.floor(player.offsetMs / perBar)
                                        const last = Math.ceil((player.offsetMs + player.windowMs) / perBar)

                                        const originPx = channel.originPx(canvas)

                                        const perBucket = player.fullMs / values.length

                                        for (let bar = first; bar <= last; ++bar) {
                                            const at = bar * perBar
                                            if (at < 0 || at > player.fullMs) {
                                                continue
                                            }

                                            const from = Math.max(0, Math.min(values.length - 1,
                                                                  Math.floor(at / perBucket)))
                                            const to = Math.max(from + 1, Math.min(values.length,
                                                                Math.ceil((at + perBar) / perBucket)))
                                            let loudest = 0
                                            for (let bucket = from; bucket < to; ++bucket) {
                                                if (values[bucket] > loudest) {
                                                    loudest = values[bucket]
                                                }
                                            }

                                            const half = Math.max(1, loudest * middle)
                                            const x = bar * channel.barStep - originPx

                                            if (at <= head) {
                                                context.fillStyle = Material.accent
                                                context.globalAlpha = 0.9
                                            } else {
                                                context.fillStyle = Material.foreground
                                                context.globalAlpha = 0.30
                                            }
                                            context.fillRect(x, middle - half, channel.barWidth, half * 2)
                                        }
                                    }

                                    Canvas {
                                        id: picture

                                        anchors.fill: parent

                                        renderStrategy: Canvas.Immediate

                                        onPaint: channel.drawInto(picture)

                                        Connections {
                                            target: recordings
                                            function onWaveChanged() { channel.readTheWave() }
                                        }

                                        Connections {
                                            target: player
                                            function onOffsetMsChanged() { channel.repaintIfItMoved() }
                                            function onWindowMsChanged() { channel.repaintIfItMoved() }
                                            function onHeadChanged() { channel.repaintIfItMoved() }
                                        }

                                        Connections {
                                            target: recordingsBody
                                            function onResizingChanged() {
                                                if (!recordingsBody.resizing) {
                                                    picture.requestPaint()
                                                }
                                            }
                                        }
                                    }

                                    Rectangle {

                                        x: Math.round(Math.min(channel.width - width,
                                                     channel.width * Math.max(0, Math.min(1, channel.headHere))))
                                        width: 2
                                        height: parent.height
                                        color: Material.accent

                                        visible: channel.headHere >= 0 && channel.headHere <= 1
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onPressed: (mouse) => recordingsBody.seekTo(mouse.x, width)
                                        onPositionChanged: (mouse) => {
                                            if (pressed) {
                                                recordingsBody.seekTo(mouse.x, width)
                                            }
                                        }
                                    }

                                    WheelHandler {
                                        enabled: player.zoomed
                                        onWheel: (event) => {
                                            const steps = event.angleDelta.y / 120
                                            player.nudged()
                                            player.scrollTo(player.offsetMs
                                                            - steps * player.windowMs * 0.25)
                                        }
                                    }
                                }
                            }

                            ThinScrollBar {
                                id: waveScroll

                                Layout.fillWidth: true

                                visible: player.zoomed
                                orientation: Qt.Horizontal
                                always: true
                                size: player.windowMs / player.fullMs
                                position: player.offsetMs / player.fullMs

                                onPositionChanged: {
                                    const wanted = waveScroll.position * player.fullMs
                                    if (Math.abs(wanted - player.offsetMs) > 1) {
                                        if (waveScroll.pressed) {
                                            player.nudged()
                                        }
                                        player.scrollTo(wanted)
                                    }
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        visible: !player.drawing
                        spacing: 4

                        Label {
                            Layout.fillWidth: true
                            visible: !recordings.openHasAudio
                            text: qsTr("The audio is no longer kept: a retention rule deleted it.")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 12
                            opacity: 0.7
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: recordings.openHasAudio && recordings.openUnreadable
                            text: qsTr("This recording could not be opened: the file is not where the library says it is, or something else has written over it.")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 12
                            color: Ink.bad(Material.theme)
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: recordings.openUnfinished
                            text: qsTr("This conversation is being recorded now. It can be played once the recording stops.")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 12
                            opacity: 0.7
                        }
                    }

                    ColumnLayout {
                        Layout.fillHeight: true
                        spacing: 4

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 0

                            PlayerGlyph {
                                visible: player.drawing
                                picture: "save"
                                hint: qsTr("Save a copy of this recording · %1")
                                      .arg(recordingsBody.size(recordings.openBytes))
                                onClicked: {
                                    saving.selectedFile = "file:///" + recordings.exportName
                                    saving.open()
                                }
                            }

                            PlayerGlyph {
                                picture: "dismiss"
                                hint: qsTr("Close this conversation")
                                onClicked: recordings.stop()
                            }
                        }

                        PlayerChoice {
                            id: speedChoice

                            Layout.preferredWidth: Math.max(speedChoice.implicitWidth,
                                                            sidesChoice.implicitWidth)

                            readonly property var values: [0.5, 0.75, 1.0, 1.25, 1.5, 2.0]

                            visible: player.drawing
                            text: recordingsBody.speedLabel(recordings.speed)
                            hint: qsTr("How fast to play it")
                            onClicked: speedMenu.popup(speedChoice, 0, speedChoice.height)

                            Menu {
                                id: speedMenu

                                Repeater {
                                    model: speedChoice.values

                                    delegate: MenuItem {
                                        required property var modelData

                                        text: recordingsBody.speedLabel(modelData)
                                        checkable: true
                                        checked: recordings.speed === modelData
                                        onTriggered: recordings.speed = modelData
                                    }
                                }
                            }
                        }

                        PlayerChoice {
                            id: sidesChoice

                            Layout.preferredWidth: Math.max(sidesChoice.implicitWidth,
                                                            speedChoice.implicitWidth)

                            visible: player.drawing && !recordings.openMixed
                            text: recordingsBody.sidesLabel(player.sides)
                            hint: qsTr("Which voice to listen to")
                            onClicked: sidesMenu.popup(sidesChoice, 0, sidesChoice.height)

                            Menu {
                                id: sidesMenu

                                Repeater {
                                    model: [0, 1, 2]

                                    delegate: MenuItem {
                                        required property var modelData

                                        text: recordingsBody.sidesLabel(modelData)
                                        checkable: true
                                        checked: player.sides === modelData
                                        onTriggered: recordings.listenTo(modelData !== 2,
                                                                         modelData !== 1)
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    visible: transcriptPanel.visible
                    color: recordingsBody.faintColour
                }

            Item {
                id: transcriptPanel

                Layout.fillWidth: true

                Layout.fillHeight: true

                readonly property real floor: readingTools.implicitHeight + 8 + 2 * 22

                Layout.minimumHeight: transcriptPanel.visible ? transcriptPanel.floor : 0

                visible: recordings.openId.length > 0 && recordings.readingShown
                         && transcriptPanel.readables.length > 0

                    readonly property bool anythingToRead: {
                        void transcriptPanel.readableRevision
                        for (const one of transcriptPanel.readables) {
                            if (one && one.entries.length > 0) {
                                return true
                            }
                        }
                        return false
                    }

                    property var readables: []

                    readonly property var chosenReadable: {
                        void transcriptPanel.readableRevision
                        for (let i = transcriptPanel.readables.length - 1; i >= 0; --i) {
                            const one = transcriptPanel.readables[i]
                            if (one && one.openId.length > 0) {
                                return one
                            }
                        }
                        return transcriptPanel.readables.length > 0
                               ? transcriptPanel.readables[0]
                               : null
                    }

                    function gatherReadables() {

                        if (!recordingsBody.assembled) {
                            Qt.callLater(transcriptPanel.gatherReadables)
                            return
                        }

                        const found = []
                        for (let i = 0; i < readablePanels.count; ++i) {
                            const loader = readablePanels.itemAt(i)

                            if (loader && loader.item) {
                                found.push(loader.item)
                            }
                        }
                        transcriptPanel.readables = found
                        transcriptPanel.readableRevision += 1

                        if (found.length === 0 && recordings.readingShown) {
                            recordings.closeReading()
                        }

                        if (found.length > 0 && !transcriptPanel.hadReadables) {
                            recordings.showReading()
                        }
                        transcriptPanel.hadReadables = found.length > 0
                    }

                    property bool hadReadables: false

                    property int readableRevision: 0

                    readonly property bool anyJobs: {
                        void transcriptPanel.readableRevision

                        for (const one of transcriptPanel.readables) {
                            if (one && one.jobsTitle !== undefined && one.jobsTitle !== "") {
                                return true
                            }
                        }
                        return false
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        RowLayout {
                            id: readingTools

                            Layout.fillWidth: true
                            spacing: 4

                            PickerBox {
                                id: whatPicker

                implicitHeight: recordingsBody.filterHeight

                                readonly property var values: {
                                    void transcriptPanel.readableRevision

                                    const rows = []
                                    for (const readable of transcriptPanel.readables) {
                                        if (!readable) {
                                            continue
                                        }
                                        for (const one of readable.entries) {
                                            rows.push({panel: readable,
                                                       id: one.id,
                                                       label: one.label,
                                                       glyph: one.glyph || "",
                                                       working: one.working,
                                                       failed: one.failed})
                                        }
                                    }
                                    return rows
                                }

                                visible: transcriptPanel.anythingToRead
                                Layout.fillWidth: true
                                model: values

                                displayText: whatPicker.values.length > 0
                                                 && whatPicker.currentIndex >= 0
                                             ? recordingsBody.readableName(
                                                   whatPicker.values[whatPicker.currentIndex])
                                             : ""

                                picture: whatPicker.values.length > 0
                                             && whatPicker.currentIndex >= 0
                                         ? recordingsBody.glyph(
                                               whatPicker.values[whatPicker.currentIndex].glyph
                                               || "")
                                         : ""

                                delegate: ItemDelegate {
                                    required property var modelData
                                    required property int index

                                    width: whatPicker.width
                                    text: recordingsBody.readableName(modelData)
                                    highlighted: whatPicker.highlightedIndex === index

                                    icon.source: recordingsBody.glyph(modelData.glyph || "")
                                    icon.width: 16
                                    icon.height: 16

                                    spacing: 8

                                    topPadding: 6
                                    bottomPadding: 6
                                    implicitHeight: 30

                                    Material.foreground: modelData.failed
                                                         ? Ink.bad(Material.theme)
                                                         : undefined
                                }

                                readonly property int wantedIndex:
                                    whatPicker.values.findIndex(
                                        (one) => one.panel === transcriptPanel.chosenReadable
                                                 && one.id === one.panel.openId)

                                function markTheOpenOne() {
                                    if (whatPicker.wantedIndex >= 0
                                        && whatPicker.currentIndex !== whatPicker.wantedIndex) {
                                        whatPicker.currentIndex = whatPicker.wantedIndex
                                    }
                                }

                                onCountChanged: Qt.callLater(whatPicker.markTheOpenOne)
                                onModelChanged: Qt.callLater(whatPicker.markTheOpenOne)
                                onWantedIndexChanged: Qt.callLater(whatPicker.markTheOpenOne)

                                onActivated: (chosen) => {
                                    const wanted = whatPicker.values[chosen]
                                    for (const readable of transcriptPanel.readables) {
                                        if (readable && readable !== wanted.panel) {
                                            readable.open("")
                                        }
                                    }
                                    wanted.panel.open(wanted.id)
                                }

                                Hint {
                                    owner: parent
                                    visible: parent.hovered
                                             && transcriptPanel.chosenReadable !== null
                                             && transcriptPanel.chosenReadable.provenance.length > 0
                                    text: transcriptPanel.chosenReadable
                                          ? transcriptPanel.chosenReadable.provenance
                                          : ""
                                }
                            }

                            Loader {
                                id: whenNothingToRead

                                visible: whenNothingToRead.sourceComponent !== null

                                sourceComponent: {
                                    void transcriptPanel.readableRevision

                                    if (transcriptPanel.anythingToRead) {
                                        return null
                                    }
                                    for (const one of transcriptPanel.readables) {
                                        if (one && one.whenEmpty) {
                                            return one.whenEmpty
                                        }
                                    }
                                    return null
                                }
                            }

                            Item { Layout.fillWidth: true; visible: !whatPicker.visible }

                            PanelAct {
                                id: aiAct

                                glyphs: recordingsBody.glyphs

                                picture: "sparkle"
                                shown: transcriptPanel.anyJobs

                                text: qsTr("Transcribe or ask a model…")
                                onClicked: recordingsBody.showTheAiMenu(aiAct)
                            }

                            Row {
                                spacing: 0

                                Repeater {
                                    model: {
                                        void transcriptPanel.readableRevision

                                        const ordered = []
                                        for (const one of transcriptPanel.readables) {
                                            if (one) {
                                                ordered.push(one)
                                            }
                                        }

                                        return ordered.sort((a, b) => a.actRank - b.actRank)
                                    }

                                    Loader {
                                        required property var modelData

                                        sourceComponent: modelData ? modelData.acts : null
                                    }
                                }

                            }

                        }

                        Repeater {

                            model: {
                                void transcriptPanel.readableRevision

                                const rows = []
                                for (const readable of transcriptPanel.readables) {
                                    if (!readable) {
                                        continue
                                    }
                                    for (const one of readable.notices) {
                                        if (one.text.length > 0) {
                                            rows.push(one)
                                        }
                                    }
                                }
                                return rows
                            }

                            Label {
                                required property var modelData

                                Layout.fillWidth: true
                                text: modelData.text
                                wrapMode: Text.WordWrap
                                font.pixelSize: 12
                                color: modelData.kind === "failure"
                                       ? Ink.bad(Material.theme)
                                       : Ink.warn(Material.theme)
                            }
                        }

                        Repeater {
                            id: readablePanels

                            model: {
                                void uiRegistry.slotRevision
                                return uiRegistry.itemsIn("recordings.reading")
                            }

                            onCountChanged: transcriptPanel.gatherReadables()

                            Loader {
                                id: readableLoader

                                required property string modelData

                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                visible: readableLoader.item !== null
                                         && transcriptPanel.chosenReadable === readableLoader.item

                                source: {
                                    void uiRegistry.slotRevision
                                    return uiRegistry.componentOfItem(readableLoader.modelData)
                                }

                                onLoaded: {
                                    readableLoader.item.glyphs =
                                        Qt.binding(() => recordingsBody.glyphs)
                                    readableLoader.item.chosen =
                                        Qt.binding(() => transcriptPanel.chosenReadable
                                                         === readableLoader.item)
                                    transcriptPanel.gatherReadables()
                                }

                                onStatusChanged: {
                                    if (readableLoader.status === Loader.Error) {
                                        console.warn("the readable", readableLoader.modelData,
                                                     "did not load:", readableLoader.source)
                                    }
                                }
                            }
                        }

                    }
                }
            }
        }

        Frame {
            id: queueStrip

            property bool opened: false

            property int shownCount: 0
            property int shownWorking: 0
            property int shownFailed: 0
            property string shownSummary: ""
            property double shownSince: 0

            property double now: 0

            function readTheQueue() {
                if (jobs.count > 0) {
                    linger.stop()
                    queueStrip.shownCount = jobs.count
                    queueStrip.shownWorking = jobs.working
                    queueStrip.shownFailed = jobs.failed
                    queueStrip.shownSummary = jobs.summary
                    queueStrip.shownSince = jobs.workingSinceMs
                    queueStrip.now = Date.now()
                } else if (queueStrip.shownCount > 0 && !linger.running) {
                    linger.restart()
                }
            }

            Timer {
                interval: 1000
                repeat: true
                triggeredOnStart: true
                running: queueStrip.visible && queueStrip.shownWorking > 0
                onTriggered: queueStrip.now = Date.now()
            }

            Component.onCompleted: queueStrip.readTheQueue()

            Connections {
                target: jobs

                function onRowsChanged() {
                    queueStrip.readTheQueue()
                }
            }

            Timer {
                id: linger

                interval: 2000
                onTriggered: {
                    queueStrip.shownCount = 0
                    queueStrip.shownWorking = 0
                    queueStrip.shownFailed = 0
                    queueStrip.shownSummary = ""
                    queueStrip.shownSince = 0
                }
            }

            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            visible: queueStrip.shownCount > 0
            padding: 8

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    BusyIndicator {
                        visible: queueStrip.shownWorking > 0
                        running: visible
                        padding: 0
                        implicitWidth: 30
                        implicitHeight: 30
                    }

                    Label {
                        Layout.fillWidth: true
                        text: {
                            void Qt.uiLanguage
                            if (queueStrip.shownWorking === 0 || queueStrip.shownSince <= 0) {
                                return queueStrip.shownSummary
                            }

                            return qsTr("%1 · %2").arg(queueStrip.shownSummary)
                                   .arg(Duration.text(queueStrip.now - queueStrip.shownSince))
                        }
                        elide: Text.ElideRight
                        font.pixelSize: 12
                        color: queueStrip.shownWorking === 0 && queueStrip.shownFailed > 0
                               ? Ink.bad(Material.theme)
                               : Material.foreground
                    }

                    Button {
                        flat: true
                        topInset: 0
                        bottomInset: 0
                        text: queueStrip.opened ? qsTr("Hide") : qsTr("Show")
                        onClicked: queueStrip.opened = !queueStrip.opened
                    }

                    Button {
                        flat: true
                        topInset: 0
                        bottomInset: 0
                        visible: queueStrip.shownFailed > 0
                        text: qsTr("Clear failures")
                        onClicked: jobs.dismissAll()
                    }
                }

                ListView {
                    id: queueList

                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(160, contentHeight)
                    visible: queueStrip.opened
                    clip: true
                    spacing: 2
                    model: jobs.rows

                    ScrollBar.vertical: ThinScrollBar {}

                    delegate: ItemDelegate {
                        id: queueRow

                        required property var modelData

                        width: queueList.width
                        onClicked: recordings.open(modelData.recordingId)

                        RowMenu {
                            extra: queueRow.modelData.error.length > 0
                                   ? [{

                                       label: qsTr("Copy the error"),
                                       picture: settings.glyphSet + "/copy.svg",
                                       act: () => settings.copyToClipboard(
                                                queueRow.modelData.error)
                                   }]
                                   : []
                        }

                        contentItem: RowLayout {
                            spacing: 8

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.party.length > 0
                                          ? qsTr("%1 · %2").arg(modelData.party)
                                                .arg(recordingsBody.stamp(modelData.startedAtMs))
                                          : recordingsBody.stamp(modelData.startedAtMs)
                                    elide: Text.ElideRight
                                    font.pixelSize: 12
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.kindLabel
                                    elide: Text.ElideRight
                                    font.pixelSize: 11
                                    opacity: 0.6
                                }

                                Label {
                                    Layout.fillWidth: true
                                    visible: modelData.error.length > 0
                                    text: modelData.error
                                    elide: Text.ElideRight
                                    font.pixelSize: 11
                                    opacity: 0.7
                                    color: Ink.bad(Material.theme)
                                }
                            }

                            Label {
                                text: recordingsBody.jobStateLabel(modelData)
                                font.pixelSize: 11
                                opacity: 0.7
                            }

                            Label {
                                visible: !modelData.failed && modelData.sinceMs > 0
                                text: Duration.text(queueStrip.now - modelData.sinceMs)
                                font.pixelSize: 11
                                opacity: 0.7
                            }

                            Button {
                                flat: true
                                topInset: 0
                                bottomInset: 0
                                text: modelData.failed ? qsTr("Dismiss") : qsTr("Stop")
                                onClicked: modelData.failed ? jobs.dismiss(modelData.id)
                                                            : jobs.cancel(modelData.id)
                            }
                        }
                    }
                }
            }
        }

    }

    function filterLabel(value) {
        void Qt.uiLanguage

        if (value === kindPicker.fromTheHistory) {
            return qsTr("One call")
        }

        switch (value) {
        case RecordingsViewModel.Filter.All:
            return qsTr("All calls")
        case RecordingsViewModel.Filter.Incoming:
            return qsTr("Incoming")
        case RecordingsViewModel.Filter.Outgoing:
            return qsTr("Outgoing")
        case RecordingsViewModel.Filter.Captured:
            return qsTr("Captured")
        case RecordingsViewModel.Filter.Imported:

            return qsTr("Imported")
        case RecordingsViewModel.Filter.Pinned:
            return qsTr("Kept")
        case RecordingsViewModel.Filter.Conference:
            return qsTr("Conferences")
        }
        return ""
    }

    function periodEntry(value) {
        void Qt.uiLanguage

        switch (value) {
        case RecordingsViewModel.Period.Any:
            return qsTr("Any time")
        case RecordingsViewModel.Period.Today:
            return qsTr("Today")
        case RecordingsViewModel.Period.Yesterday:
            return qsTr("Yesterday")
        case RecordingsViewModel.Period.Week:
            return qsTr("Last 7 days")
        case RecordingsViewModel.Period.Range:
            return qsTr("Choose dates…")
        }
        return ""
    }

    function periodLabel(value) {
        void Qt.uiLanguage

        if (value === RecordingsViewModel.Period.Any) {
            return qsTr("Period")
        }
        if (value === RecordingsViewModel.Period.Range) {
            return recordingsBody.rangeText(recordings.fromMs, recordings.toMs)
        }
        return recordingsBody.periodEntry(value)
    }

    function rangeText(fromMs, toMs) {
        void Qt.uiLanguage

        const first = new Date(fromMs).toLocaleDateString(recordingsBody.formatLocale,
                                                          Locale.ShortFormat)
        if (fromMs === toMs) {
            return first
        }

        const last = new Date(toMs).toLocaleDateString(recordingsBody.formatLocale,
                                                       Locale.ShortFormat)
        return qsTr("%1 – %2").arg(first).arg(last)
    }

    function monthName(year, month) {
        void Qt.uiLanguage
        return qsTr("%1 %2").arg(recordingsBody.textLocale.standaloneMonthName(month))
                            .arg(String(year))
    }

    function filedAs(entry, flagColour) {
        void Qt.uiLanguage

        void taxonomy.count

        const escape = (text) => String(text).replace(/&/g, "&amp;")
                                             .replace(/</g, "&lt;")
                                             .replace(/>/g, "&gt;")

        const parts = []

        if (entry.category.length > 0) {
            parts.push("<b>" + escape(taxonomy.titleOf("category", entry.category)) + "</b>")
        }

        if (entry.score > 0) {
            parts.push(escape(String(entry.score)))
        }

        for (const code of entry.redFlags) {
            parts.push("<font color=\"" + flagColour + "\">"
                       + escape(taxonomy.titleOf("red_flag", code)) + "</font>")
        }
        for (const code of entry.tags) {
            parts.push(escape(taxonomy.titleOf("tag", code)))
        }
        return parts.join(" · ")
    }

    function keepTheOpenRowVisible() {
        if (recordings.openId.length === 0 || !table.visible) {
            return
        }
        const at = recordings.indexOf(recordings.openId)
        if (at >= 0) {
            table.positionViewAtIndex(at, ListView.Contain)
        }
    }

    function readableName(entry) {
        void Qt.uiLanguage

        if (!entry) {
            return ""
        }
        return entry.working ? entry.label + " …" : entry.label
    }

    function filterClosed(value) {
        void Qt.uiLanguage

        return value === RecordingsViewModel.Filter.All
                ? qsTr("Type") : recordingsBody.filterLabel(value)
    }

    function categoryClosed(value) {
        void Qt.uiLanguage
        return value.length === 0 ? qsTr("Category")
                                  : recordingsBody.categoryLabel(value)
    }

    function labelClosed(entry) {
        void Qt.uiLanguage
        return (!entry || entry.kind.length === 0) ? qsTr("Mark")
                                                   : recordingsBody.labelText(entry)
    }

    function categoryLabel(value) {
        void Qt.uiLanguage
        return value.length === 0 ? qsTr("Any category") : taxonomy.titleOf("category", value)
    }

    function tagLabel(value) {
        void Qt.uiLanguage
        return value.length === 0 ? qsTr("Any tag") : taxonomy.titleOf("tag", value)
    }

    function labelText(entry) {
        void Qt.uiLanguage

        if (!entry || entry.kind.length === 0) {

            return qsTr("Any tag or flag")
        }
        return taxonomy.titleOf(entry.kind, entry.code)
    }

    function jobStateLabel(row) {
        void Qt.uiLanguage

        if (row.failed) {
            return row.attempts > 1

                   ? qsTr("gave up after %n attempt(s)", "", row.attempts)
                   : qsTr("gave up")
        }
        if (row.running) {
            return qsTr("listening")
        }
        return qsTr("waiting")
    }

    function speedLabel(value) {
        void Qt.uiLanguage
        return qsTr("%1×").arg(value)
    }

    function sidesLabel(value) {
        void Qt.uiLanguage

        switch (value) {
        case 1:
            return qsTr("Me")
        case 2:
            return qsTr("Them")
        }

        return qsTr("Both", "both sides of a conversation")
    }

    property real coveredUntil: 0

    function pressIsForTheTable() {
        return !calendar.opened && Date.now() > recordingsBody.coveredUntil
    }

    function seekTo(x, width) {
        if (width > 0 && recordings.durationMs > 0) {

            const across = Math.max(0, Math.min(1, x / width))

            recordings.seek(player.offsetMs + across * player.windowMs)
        }
    }

    Popup {
        id: calendar

        property int shownYear: 0
        property int shownMonth: 0
        property real firstMs: 0
        property real lastMs: 0

        readonly property real lowMs: calendar.lastMs === 0
                                      ? calendar.firstMs
                                      : Math.min(calendar.firstMs, calendar.lastMs)
        readonly property real highMs: calendar.lastMs === 0
                                       ? calendar.firstMs
                                       : Math.max(calendar.firstMs, calendar.lastMs)

        parent: datePicker
        y: datePicker.height + 4
        width: 276

        modal: true
        dim: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        padding: 0

        function step(months) {
            const page = new Date(calendar.shownYear, calendar.shownMonth + months, 1)
            calendar.shownYear = page.getFullYear()
            calendar.shownMonth = page.getMonth()
        }

        function pick(day) {
            if (calendar.firstMs === 0 || calendar.lastMs !== 0) {
                calendar.firstMs = day
                calendar.lastMs = 0
                return
            }

            calendar.lastMs = day
            recordings.showDates(calendar.firstMs, calendar.lastMs)

            recordingsBody.coveredUntil = Date.now() + 400
            Qt.callLater(calendar.close)
        }

        onAboutToHide: recordingsBody.coveredUntil = Date.now() + 400
        onClosed: recordingsBody.coveredUntil = Date.now() + 400

        onAboutToShow: {
            const known = recordings.period === RecordingsViewModel.Period.Range
                          && recordings.fromMs > 0

            calendar.firstMs = known ? recordings.fromMs : 0
            calendar.lastMs = known ? recordings.toMs : 0

            const page = known ? new Date(recordings.fromMs) : new Date()
            calendar.shownYear = page.getFullYear()
            calendar.shownMonth = page.getMonth()
        }

        contentItem: Item {
            implicitWidth: calendarBody.implicitWidth + 24
            implicitHeight: calendarBody.implicitHeight + 24

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.AllButtons
            }

            ColumnLayout {
                id: calendarBody

                anchors.fill: parent
                anchors.margins: 12
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    RoundGlyphButton {
                        picture: "chevron-left"
                        hint: qsTr("The month before")
                        onClicked: calendar.step(-1)
                    }

                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        font.weight: Font.Medium
                        text: recordingsBody.monthName(calendar.shownYear, calendar.shownMonth)
                    }

                    RoundGlyphButton {
                        picture: "chevron-right"
                        hint: qsTr("The month after")
                        onClicked: calendar.step(1)
                    }
                }

                DayOfWeekRow {
                    Layout.fillWidth: true
                    locale: recordingsBody.textLocale
                    font.pixelSize: 11
                    spacing: 0
                    padding: 0

                    delegate: Label {
                        required property string shortName

                        width: 36
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 11
                        opacity: 0.6
                        text: shortName
                    }
                }

                MonthGrid {
                    id: grid

                    Layout.fillWidth: true
                    locale: recordingsBody.textLocale
                    month: calendar.shownMonth
                    year: calendar.shownYear
                    spacing: 0

                    delegate: Item {
                        id: day

                        required property var model

                        readonly property real dayMs: new Date(day.model.year, day.model.month,
                                                               day.model.day).getTime()

                        readonly property bool thisMonth: day.model.month === grid.month
                        readonly property bool inRange: calendar.firstMs > 0
                                                        && day.dayMs >= calendar.lowMs
                                                        && day.dayMs <= calendar.highMs
                        readonly property bool anEnd: day.dayMs === calendar.lowMs
                                                      || day.dayMs === calendar.highMs

                        implicitWidth: 36
                        implicitHeight: 32

                        Rectangle {
                            anchors.fill: parent
                            anchors.topMargin: 1
                            anchors.bottomMargin: 1
                            visible: day.thisMonth && day.inRange && !day.anEnd
                            color: Qt.rgba(Material.accent.r, Material.accent.g,
                                           Material.accent.b, 0.20)
                        }

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 1
                            visible: day.thisMonth && day.anEnd
                            radius: height / 2
                            color: Material.accent
                        }

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 1
                            visible: day.thisMonth && day.model.today && !day.anEnd
                            radius: height / 2
                            color: "transparent"
                            border.width: 1
                            border.color: Qt.rgba(Material.foreground.r, Material.foreground.g,
                                                  Material.foreground.b, 0.35)
                        }

                        Label {
                            anchors.centerIn: parent
                            font.pixelSize: 12
                            text: day.model.day
                            opacity: day.thisMonth ? 1.0 : 0.25
                            color: day.thisMonth && day.anEnd ? Material.background
                                                              : Material.foreground
                        }

                        HoverHandler {
                            enabled: day.thisMonth
                            cursorShape: Qt.PointingHandCursor
                        }

                        TapHandler {
                            enabled: day.thisMonth
                            onTapped: calendar.pick(day.dayMs)
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    font.pixelSize: 11
                    opacity: 0.6
                    wrapMode: Text.WordWrap
                    text: calendar.firstMs > 0 && calendar.lastMs === 0
                          ? qsTr("Now the last day. The same one again is a single day.")
                          : qsTr("Press the first day, then the last.")
                }
            }
        }
    }

    function showTheLibraryMenu() {
        const rows = [{

            label: qsTr("Import from file(s)"),
            offered: true,
            button: null,
            act: () => recordingsBody.offerToImport(),
            picture: settings.glyphSet + "/arrow-import.svg"
        }, {

            label: qsTr("Export to CSV"),
            offered: recordings.count > 0,
            button: null,
            act: () => recordingsBody.offerToSaveTheList(),
            picture: settings.glyphSet + "/save.svg"
        }]

        for (const one of contributedActs.offered()) {
            rows.push({
                label: one.text,
                offered: one.enabled,
                button: one,
                act: null,

                picture: one.icon.source
            })
        }

        libraryEntries.model = rows
        libraryMenu.present(libraryActs)
    }

    function showTheAiMenu(under) {
        const rows = []

        const ordered = []
        for (const one of transcriptPanel.readables) {
            if (one && one.jobsTitle !== undefined && one.jobsTitle !== "") {
                ordered.push(one)
            }
        }

        ordered.sort((a, b) => a.jobRank - b.jobRank)

        for (const one of ordered) {
            const jobs = one.jobsNow()
            const note = one.jobsNote
            if (jobs.length === 0 && note === "") {
                continue
            }
            rows.push({kind: "heading", label: one.jobsTitle, offered: false,
                       picture: "", act: null})
            if (jobs.length === 0) {
                rows.push({kind: "note", label: note, offered: false,
                           picture: "", act: null})
                continue
            }
            for (const job of jobs) {
                rows.push({kind: "job", label: job.label, offered: job.offered,
                           picture: "", act: job.act})
            }
        }

        aiEntries.model = rows
        aiMenu.present(under)
    }

    RightMenu {
        id: aiMenu

        host: aiAct

        height: Math.min(aiMenu.implicitHeight, 12 * 32 + 8)

        Repeater {
            id: aiEntries

            model: []

            JobEntry {}
        }
    }

    component JobEntry: MenuItem {
        id: jobEntry

        required property var modelData

        readonly property bool heading: jobEntry.modelData.kind === "heading"
        readonly property bool note: jobEntry.modelData.kind === "note"

        horizontalPadding: 12
        verticalPadding: 4
        implicitHeight: jobEntry.note
                        ? noteWords.implicitHeight + 12
                        : (jobEntry.heading ? 26 : 32)

        enabled: jobEntry.modelData.kind === "job" && jobEntry.modelData.offered

        onTriggered: {
            if (jobEntry.modelData.act) {
                jobEntry.modelData.act()
            }
        }

        contentItem: Item {
            implicitWidth: jobEntry.note ? noteWords.width : words.implicitWidth
            implicitHeight: jobEntry.note ? noteWords.implicitHeight
                                          : words.implicitHeight

            Label {
                id: words

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: !jobEntry.note
                text: jobEntry.note ? "" : jobEntry.modelData.label
                elide: Text.ElideRight
                font.bold: jobEntry.heading
                font.pixelSize: jobEntry.heading ? 11 : 13

                color: jobEntry.heading
                       ? Material.accent
                       : (jobEntry.enabled ? Material.foreground
                                           : Material.hintTextColor)
            }

            Label {
                id: noteWords

                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                visible: jobEntry.note

                width: 260
                text: jobEntry.note ? jobEntry.modelData.label : ""
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Material.hintTextColor
            }
        }
    }

    function offerToImport() {
        if (importing.currentFolder.toString().length === 0) {
            importing.currentFolder =
                Platform.StandardPaths.writableLocation(Platform.StandardPaths.MusicLocation)
        }
        importing.open()
    }

    function offerToSaveTheList() {
        if (savingList.currentFolder.toString().length === 0) {
            savingList.currentFolder =
                Platform.StandardPaths.writableLocation(Platform.StandardPaths.DocumentsLocation)
        }
        savingList.selectedFile = savingList.currentFolder + "/" + recordings.listName()
        savingList.open()
    }

    Item {
        id: contributedActs

        visible: false

        function offered() {
            const out = []
            for (let at = 0; at < acts.count; ++at) {
                const loaded = acts.itemAt(at)
                const act = loaded ? loaded.item : null
                if (act && act.offered !== false) {
                    out.push(act)
                }
            }
            return out
        }

        Repeater {
            id: acts

            model: {
                void uiRegistry.slotRevision
                return uiRegistry.itemsIn("recordings.acts")
            }

            SlotLoader {
                id: actLoader

                required property string modelData

                what: actLoader.modelData
                page: recordingsBody
                noun: "the act"
            }
        }
    }

    RightMenu {
        id: libraryMenu

        host: libraryActs

        Repeater {
            id: libraryEntries

            model: []

            TextMenuItem {
                required property var modelData

                text: modelData.label
                enabled: modelData.offered
                picture: modelData.picture

                onTriggered: {
                    if (modelData.button) {
                        modelData.button.clicked()
                    } else {
                        modelData.act()
                    }
                }
            }
        }
    }

    Dialogs.FileDialog {
        id: savingList

        title: qsTr("Save the list")
        fileMode: Dialogs.FileDialog.SaveFile
        nameFilters: [qsTr("Spreadsheets (*.csv)"), qsTr("All files (*)")]
        onAccepted: recordings.saveListTo(savingList.selectedFile)
    }

    Dialogs.FileDialog {
        id: importing

        title: qsTr("Import conversations")
        fileMode: Dialogs.FileDialog.OpenFiles

        nameFilters: [qsTr("Sound files (*.mp3 *.wav)")]
        onAccepted: recordings.importFiles(importing.selectedFiles)
    }

    Dialogs.FileDialog {
        id: saving

        title: qsTr("Save the recording")
        fileMode: Dialogs.FileDialog.SaveFile
        nameFilters: [qsTr("Sound files (*.mp3)"), qsTr("All files (*)")]
        onAccepted: recordings.saveAs(saving.selectedFile)
    }

    ConfirmDialog {
        id: deleting

        property string recordingId: ""
        title: qsTr("Delete this recording?")
        actionText: qsTr("Delete")
        onAccepted: {
            recordings.remove(deleting.recordingId)
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("The audio goes also. Nothing about the conversation is kept. Deleting cannot be undone.")
        }
    }

    ConfirmDialog {
        id: notAConversation

        property string recordingId: ""
        property string what: ""

        title: qsTr("Not a conversation?")
        actionText: qsTr("Delete")
        onAccepted: {
            settings.notACall(notAConversation.recordingId)
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap

            text: qsTr("The recording goes. %1 stops being recorded from now on; "
                       + "Settings → Capture lists what is excluded and takes it back "
                       + "off the list. Deleting cannot be undone.")
                  .arg(notAConversation.what)
        }
    }

    ConfirmDialog {
        id: naming

        property string recordingId: ""
        property string was: ""

        property bool unfinished: false

        property bool capture: false

        property bool named: false

        property bool imported: false

        title: qsTr("Name this conversation")
        actionText: qsTr("Save")
        destructive: false

        actionEnabled: !naming.named || chosenName.text.trim().length > 0
        onAccepted: {
            recordings.rename(naming.recordingId, chosenName.text)

            if (naming.capture && naming.unfinished) {
                settings.renameCapture(chosenName.text)
            }
        }
        onOpened: {
            chosenName.text = naming.was
            chosenName.forceActiveFocus()
            chosenName.selectAll()
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: naming.imported
                  ? qsTr("What this conversation is called. It arrived named after the "
                         + "file it was brought in from.")
                  : naming.capture
                    ? qsTr("What this conversation is called. It arrived named after the "
                           + "application it was heard in.")
                    : qsTr("Shown beside the name this conversation already has. "
                           + "Leave it empty for none.")
        }

        TextField {
            id: chosenName

            TextMenu {}

            Layout.fillWidth: true
            placeholderText: qsTr("Name")
            onAccepted: naming.accept()
        }
    }
}
