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
import QtQml
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import Qt.labs.platform as Platform

import QtQuick.Dialogs as DeployedForThePlugins

import Softphone

ApplicationWindow {
    id: root

    readonly property int columnWidth: shape.column

    readonly property int wantedWidth:
        root.columnWidth + (root.keyColumns === 0 ? sideSlot.implicitWidth : 0)

    readonly property int keyColumns: shape.keyColumns

    readonly property bool oneChipRow: shape.oneChipRow

    function fitTheWidth() {

        if (root.zonesInside || root.isTheBar) {
            return
        }

        const to = root.wantedWidth
        if (root.width === to && root.minimumWidth === to && root.maximumWidth === to) {
            return
        }
        if (to > root.width) {
            root.maximumWidth = to
            root.width = to
            root.minimumWidth = to
        } else {
            root.minimumWidth = to
            root.width = to
            root.maximumWidth = to
        }
    }

    onWantedWidthChanged: Qt.callLater(root.fitTheWidth)

    readonly property bool waysInAsRow:
        !root.isTheBar && !root.zonesInside && !shape.toolbarIsAMenu

    readonly property bool waysInAsMenu:
        !root.isTheBar && !root.zonesInside && shape.toolbarIsAMenu

    readonly property bool waysInMarked: {
        void uiRegistry.markRevision
        return uiRegistry.anythingMarked()
    }

    readonly property int waysInExtra:
        root.waysInAsRow
        ? Math.max(0, toolbarRow.rows - 1) * (toolbarRow.rowHeight + toolbarRow.rowSpacing)
        : 0

    property bool pageHoldsTheMenu: false

    property int primaryLeast: shape.leastHeight

    readonly property int leastMiddle: 560

    width: root.zonesInside ? root.columnWidth + 900 : root.columnWidth

    height: root.zonesInside ? 780 : root.minimumHeight

    readonly property int noCeiling: 16777215

    minimumWidth: root.isTheBar ? 320
                                : (root.zonesInside ? root.columnEnds + root.leastMiddle
                                                    : root.columnWidth)
    maximumWidth: root.isTheBar || root.zonesInside ? root.noCeiling : root.columnWidth

    minimumHeight: root.isTheBar
                   ? root.barHeight
                   : root.primaryLeast + root.primaryExtra + root.footerExtra + root.waysInExtra

    visibility: Window.Hidden

    property string dialRefusal: ""

    Timer {
        id: refusalFades

        interval: 8000
        onTriggered: root.dialRefusal = ""
    }

    Connections {
        target: buttons

        function onDialRefused(reason) {
            void Qt.uiLanguage

            switch (reason) {
            case Dialling.NotConnected:
                root.dialRefusal = qsTr("Not connected yet, so that call could not be placed.")
                break
            case Dialling.NoLine:
                root.dialRefusal = qsTr("No line is switched on to call from.")
                break
            case Dialling.NothingToDial:
                root.dialRefusal = qsTr("That key has no number on it.")
                break
            case Dialling.TooManyCalls:
                root.dialRefusal = qsTr("As many calls are up as this phone will carry.")
                break
            default:
                root.dialRefusal = qsTr("That call could not be placed.")
                break
            }

            refusalFades.restart()
        }
    }

    property string recordingRefusal: ""

    Timer {
        id: recordingRefusalFades

        interval: 8000
        onTriggered: root.recordingRefusal = ""
    }

    Connections {
        target: calls

        function onRecordingRefused(why) {
            void Qt.uiLanguage

            switch (why) {
            case CallViewModel.Refusal.NoRoom:
                root.recordingRefusal =
                    qsTr("Not recording: the disk is down to the space. Free some, or change the limit in Settings.")
                break
            default:
                root.recordingRefusal = qsTr("The recording would not start.")
                break
            }

            recordingRefusalFades.restart()
        }
    }

    readonly property bool ringingOut: calls.hasCall && !calls.ended
                                       && calls.direction === CallViewModel.Outgoing
                                       && (calls.state === CallViewModel.RingingOut
                                           || calls.state === CallViewModel.Dialing)
                                       && calls.startedAtMs > 0

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

    function keyHint(entry) {
        void Qt.uiLanguage

        const parts = [entry.number]
        if (entry.lineLabel !== "") {
            parts.push(entry.lineLabel)
        }
        if (entry.watched) {
            parts.push(root.lampText(entry.state, entry.trouble))
        }
        return parts.join(" · ")
    }

    function lampText(state, trouble) {
        void Qt.uiLanguage

        switch (state) {
        case ButtonsViewModel.Idle:
            return qsTr("free")
        case ButtonsViewModel.Ringing:
            return qsTr("ringing")
        case ButtonsViewModel.Busy:
            return qsTr("on a call")
        }

        switch (trouble) {
        case ButtonsViewModel.NoLine:
            return qsTr("the line is not registered")
        case ButtonsViewModel.Asking:
            return qsTr("asking the switch")
        case ButtonsViewModel.NoSuchLine:
            return qsTr("the switch has no such number")
        case ButtonsViewModel.Refused:
            return qsTr("the switch will not report this line")
        }
        return qsTr("no answer from the switch")
    }

    onVisibleChanged: sound.watchWindow(visible)

    title: qsTr("AI Softphone")

    Component.onCompleted: {

        if (root.isTheBar) {
            Qt.callLater(root.placeTheBar)
        }

        if (root.strandedWithoutATray) {
            console.warn("this desktop has no notification area, so the phone is showing its",
                         "window: the arrangement it was set to has no other way in")
            Qt.callLater(root.showTheWindow)
        }

        if (root.buildEveryWindow) {
            root.buildPluginWindows()
        }

        root.fitTheWidth()

        const placed = windows.restore("main", root.minimumWidth, root.minimumHeight)
        if (placed.width > 0 && placed.height > 0) {
            root.height = placed.height
            root.x = placed.x
            root.y = placed.y

            root.wanted = Qt.point(placed.x, placed.y)
            root.placeKnown = true
        }

        if (!startsHidden) {
            root.visibility = Window.Windowed
            return
        }

        if (!root.trayShown) {

            root.visibility = Window.Minimized
            outOfTheWay.start()
        }
    }

    Timer {
        id: outOfTheWay

        interval: 200
        onTriggered: if (root.visibility !== Window.Minimized) root.showMinimized()
    }

    property point wanted: Qt.point(0, 0)

    property bool placeKnown: false

    function rememberPlace() {

        if (root.visibility !== Window.Windowed) {
            return
        }

        root.wanted = Qt.point(root.x, root.y)
        root.placeKnown = true
        rememberGeometry.restart()
    }

    function takeThePlaceBack() {
        if (root.placeKnown) {
            desktop.keepPlace(root, root.wanted.x, root.wanted.y)
        }
    }

    Timer {
        id: rememberGeometry

        interval: 500

        onTriggered: if (root.placeKnown) {
            windows.remember("main", root.wanted.x, root.wanted.y, root.width, root.height)
        }
    }

    onXChanged: root.rememberPlace()
    onYChanged: root.rememberPlace()
    onWidthChanged: rememberGeometry.restart()
    onHeightChanged: rememberGeometry.restart()

    flags: root.isTheBar
           ? (Qt.FramelessWindowHint
              | (desktop.edgesReservable ? Qt.WindowStaysOnTopHint : 0))
           : (Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint
              | Qt.WindowMinimizeButtonHint | Qt.WindowCloseButtonHint

              | (root.zonesInside ? Qt.WindowMaximizeButtonHint : 0))

    onTrayShownChanged: {
        if (!root.trayShown && !root.visible) {
            root.showMinimized()
        }
    }

    readonly property bool trayShown: tray.available

    readonly property bool onScreen: root.visible && root.visibility !== Window.Minimized

    readonly property bool darkWanted: settings.darkWanted

    Material.theme: root.darkWanted ? Material.Dark : Material.Light
    Material.accent: Material.Teal

    readonly property color nearColour: Material.accentColor
    readonly property color farColour: root.darkWanted ? "#7986CB" : "#5C6BC0"

    readonly property color faintColour: Qt.rgba(Material.foreground.r, Material.foreground.g,
                                                 Material.foreground.b, 0.14)

    function overGround(ink, alpha) {
        const ground = Material.background
        return Qt.rgba(ink.r * alpha + ground.r * (1 - alpha),
                       ink.g * alpha + ground.g * (1 - alpha),
                       ink.b * alpha + ground.b * (1 - alpha), 1)
    }

    readonly property color idleNearColour:
        root.overGround(Material.foreground, root.darkWanted ? 0.27 : 0.35)
    readonly property color idleFarColour:
        root.overGround(Material.foreground, root.darkWanted ? 0.20 : 0.26)

    function statusText(state) {

        void Qt.uiLanguage

        switch (state) {
        case AccountsViewModel.Registered:
            return qsTr("Connected")
        case AccountsViewModel.Registering:
            return qsTr("Connecting…")
        case AccountsViewModel.Unregistering:
            return qsTr("Disconnecting…")
        case AccountsViewModel.Failed:
            return qsTr("Not connected")
        default:
            return qsTr("Offline")
        }
    }

    function statusColor(state) {
        switch (state) {
        case AccountsViewModel.Registered:
            return Mark.good
        case AccountsViewModel.Registering:
        case AccountsViewModel.Unregistering:
            return Mark.warn(Material.theme)
        case AccountsViewModel.Failed:
            return Mark.bad
        default:
            return Mark.muted
        }
    }

    function lineColor(entry) {
        if (entry === undefined || !entry.enabled) {
            return Mark.muted
        }
        return root.statusColor(entry.state)
    }

    function callColor(entry) {
        if (entry === undefined) {
            return Mark.muted
        }
        if (entry.state === CallViewModel.Ended) {
            return Mark.muted
        }
        if (entry.state === CallViewModel.RingingIn) {
            return Mark.info
        }
        if (entry.held || entry.heldByThem) {
            return Mark.warn(Material.theme)
        }
        if (entry.state === CallViewModel.Active) {

            return Mark.good
        }
        return Material.accentColor
    }

    readonly property string lineInTheCard: calls.hasCall ? calls.accountId : ""

    function spotlightSentence() {

        void Qt.uiLanguage

        const problem = root.problemText(accounts.spotlightProblem)
        if (problem.length === 0) {
            return ""
        }
        if (!accounts.severalLines) {
            return problem
        }

        const others = accounts.failedCount - 1
        if (others > 0) {
            return qsTr("%1: %2 (and %n other account(s))", "", others)
                .arg(accounts.spotlightLabel).arg(problem)
        }
        return qsTr("%1: %2").arg(accounts.spotlightLabel).arg(problem)
    }

    function lineNote(accountId) {

        void Qt.uiLanguage

        const label = accounts.labelOf(accountId)
        return label.length > 0 ? qsTr("Line %1").arg(label) : ""
    }

    function rowStateText(entry) {

        void Qt.uiLanguage

        const state = root.callStateText(entry.state, entry.held, entry.heldByThem,
                                         entry.ending, entry.inConference)
        if (!accounts.severalLines) {
            return state
        }

        const label = accounts.labelOf(entry.accountId)
        return label.length > 0 ? qsTr("%1 · %2").arg(state).arg(label) : state
    }

    function problemText(problem) {

        void Qt.uiLanguage

        switch (problem) {
        case AccountsViewModel.AuthenticationRejected:
            return qsTr("The server rejected this username or password.")
        case AccountsViewModel.Forbidden:
            return qsTr("The server refused this account. Check the password, and whether the account is allowed to register from here.")
        case AccountsViewModel.AccountNotFound:
            return qsTr("The server does not know this account.")
        case AccountsViewModel.RegistrarUnreachable:
            return qsTr("Could not reach the server. Check the address, the port, and your network.")
        case AccountsViewModel.DomainNotResolved:
            return qsTr("Could not find that server address. Check it for typing mistakes.")
        case AccountsViewModel.ServerUnavailable:
            return qsTr("The server is temporarily unavailable.")
        case AccountsViewModel.CertificateNotTrusted:
            return qsTr("The server's certificate could not be verified. If the server's administrator made it themselves, untick Verify the certificate in Server settings.")
        case AccountsViewModel.InvalidConfiguration:
            return qsTr("Something in these settings is not valid.")
        case AccountsViewModel.PasswordUnavailable:
            return qsTr("The password for this account could not be read from the keychain. Unlock it and connect again.")
        case AccountsViewModel.Unknown:
            return qsTr("The server refused the connection for an unknown reason.")
        default:
            return ""
        }
    }

    function elapsed(milliseconds) {
        if (milliseconds < 0)
            return ""

        const total = Math.floor(milliseconds / 1000)
        const seconds = String(total % 60).padStart(2, "0")
        const minutes = Math.floor(total / 60) % 60
        const hours = Math.floor(total / 3600)

        return hours > 0
            ? hours + ":" + String(minutes).padStart(2, "0") + ":" + seconds
            : minutes + ":" + seconds
    }

    function duration(connectedAtMs, endedAtMs) {
        return root.elapsed((endedAtMs > 0 ? endedAtMs : calls.nowMs) - connectedAtMs)
    }

    function redialProgress() {
        void Qt.uiLanguage

        const waiting = calls.redialWaiting
                      ? qsTr("waiting for the line")
                      : qsTr("next try in %n second(s)", "",
                             Math.max(0, Math.ceil((calls.redialNextAtMs - calls.nowMs) / 1000)))

        return qsTr("Attempt %1 · %2").arg(calls.redialAttempts).arg(waiting)
    }

    function callStateText(state, held, heldByThem, ending, inConference) {

        void Qt.uiLanguage

        switch (state) {
        case CallViewModel.Dialing:
            return qsTr("Calling…")
        case CallViewModel.RingingOut:
            return qsTr("Ringing…")
        case CallViewModel.RingingIn:
            return qsTr("Incoming call")
        case CallViewModel.Active:
            if (held)
                return qsTr("On hold")
            if (heldByThem)

                return qsTr("Held by them")

            if (inConference)
                return qsTr("In conference")
            return qsTr("On a call")
        case CallViewModel.Ended:
            return root.endingText(ending)
        default:
            return ""
        }
    }

    function endingText(ending) {

        void Qt.uiLanguage

        switch (ending) {
        case CallViewModel.Transferred:

            return qsTr("Transferred")
        case CallViewModel.Busy:
            return qsTr("Busy")
        case CallViewModel.Declined:
            return qsTr("Declined")
        case CallViewModel.PickedUpElsewhere:

            return qsTr("Answered elsewhere")
        case CallViewModel.NoAnswer:
            return qsTr("No answer")
        case CallViewModel.Cancelled:
            return qsTr("Missed call")
        case CallViewModel.Unavailable:
            return qsTr("Not available")
        case CallViewModel.NumberNotFound:
            return qsTr("No such number here")
        case CallViewModel.NotAllowed:
            return qsTr("Not allowed")
        case CallViewModel.MediaMismatch:
            return qsTr("No audio format in common")
        case CallViewModel.ServerError:
            return qsTr("The server could not connect it")
        case CallViewModel.Unreachable:
            return qsTr("Could not be reached")
        case CallViewModel.NoAudioDevice:

            return qsTr("No microphone or speakers")
        case CallViewModel.Failed:
            return qsTr("Did not go through")
        default:

            return qsTr("Call ended")
        }
    }

    function transferFailureText(status, who) {

        void Qt.uiLanguage

        switch (status) {
        case 486:
        case 600:
            return qsTr("%1 is busy. You are back on the call.").arg(who)
        case 404:
        case 604:
            return qsTr("There is no %1. You are back on the call.").arg(who)
        case 480:
        case 408:
            return qsTr("%1 did not answer. You are back on the call.").arg(who)
        case 0:

            return qsTr("Your server never said what happened to this transfer, which is a fault at its end. The call is back with you, and the transfer may still go through.")
        default:
            return qsTr("The transfer did not go through. You are back on the call.")
        }
    }

    Item {
        id: focusSink
    }

    function askWhereToTransfer() {
        if (transferApart.item !== null) {
            transferApart.item.ask()
            return
        }

        transferDialog.open()
    }

    function dropFocus() {
        focusSink.forceActiveFocus()
    }

    Dialog {
        id: transferDialog

        width: Math.min(root.width - 48, 360)
        modal: true
        title: qsTr("Transfer %1").arg(calls.remoteLabel)
        anchors.centerIn: Overlay.overlay

        header: Label {
            text: transferDialog.title
            visible: transferDialog.title !== ""
            elide: Label.ElideRight
            padding: root.dialogPad
            bottomPadding: 0
            font.pixelSize: root.dialogTitleSize
        }
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        onAboutToShow: transferShell.begin()

        onOpened: {
            calls.beginTransfer()

            transferShell.takeTheKeyboard()
        }

        onClosed: {
            if (!transferShell.arranged) {
                calls.cancelTransfer()
            }
        }

        Connections {
            target: calls
            function onCallChanged() {
                if (transferDialog.opened && !calls.canTransfer) {
                    transferDialog.close()
                }
            }
        }

        TransferBody {
            id: transferShell

            anchors.fill: parent

            page: root
            onFinished: transferDialog.close()
        }
    }

    Loader {
        id: transferApart

        active: root.isTheBar || root.hasNoWindow

        sourceComponent: TransferWindow {
            phone: root
        }
    }

    function onDark(colour) {
        return Material.color(colour, root.darkWanted ? Material.Shade700 : Material.Shade500)
    }

    readonly property string glyphs: settings.glyphSet

    readonly property bool largeGlyphs: settings.iconScale === SettingsViewModel.IconScale.Large

    readonly property int cardGlyphSize: root.largeGlyphs ? shape.cardGlyphLarge : shape.cardGlyph

    readonly property int cardButtonHeight:
        root.largeGlyphs ? shape.cardButtonLarge : shape.cardButton

    readonly property real keyGlyphShare: shape.keyGlyph / 100

    function glyph(name) {
        return root.glyphs + "/" + name + ".svg"
    }

    function shapeOf(drawing) {
        switch (drawing) {
        case SoundViewModel.Histogram:
            return SignalDrawing.Histogram
        case SoundViewModel.Wave:
            return SignalDrawing.Wave
        case SoundViewModel.Spectrum:
            return SignalDrawing.Spectrum
        case SoundViewModel.Aurora:
            return SignalDrawing.Aurora
        case SoundViewModel.Halo:
            return SignalDrawing.Halo
        }
        return SignalDrawing.Bar
    }

    readonly property var textLocale: Qt.uiLanguage ? Qt.locale(Qt.uiLanguage) : Qt.locale()

    function haloMark() {
        void Qt.uiLanguage

        if (calls.inConference) {
            return Number(calls.conferenceList.length).toLocaleString(root.textLocale, 'f', 0)
        }
        return root.initialsOf(calls.remoteLabel)
    }

    function initialsOf(name) {
        const words = name.trim().split(/\s+/).filter((word) => word.length > 0)
        if (words.length === 0) {
            return ""
        }
        if (words.length === 1) {
            return words[0].substring(0, 2).toLocaleUpperCase()
        }
        return (words[0].charAt(0) + words[1].charAt(0)).toLocaleUpperCase()
    }

    Item {
        anchors.fill: parent

        TapHandler {
            onTapped: root.dropFocus()
        }
    }

    Row {
        id: sideSlot

        visible: !root.isTheBar

        anchors.left: parent.left
        anchors.leftMargin: root.keyColumns === 0 ? root.columnWidth : 0
        width: root.keyColumns === 0 ? implicitWidth : root.columnWidth
        anchors.top: root.keyColumns === 0 ? dndBand.bottom : undefined
        anchors.bottom: root.keyColumns === 0
                        ? parent.bottom
                        : (root.zonesInside ? sectionsZone.top
                              : (root.waysInAsRow ? toolbarRow.top : parent.bottom))

        height: root.keyColumns === 0 ? undefined : root.keyBandNow

        Repeater {
            model: {
                void uiRegistry.slotRevision
                return uiRegistry.itemsIn("side")
            }

            SlotLoader {
                id: sideItem

                required property string modelData

                what: sideItem.modelData
                page: root
                noun: "the panel"

                height: root.keyColumns === 0 ? sideSlot.height : implicitHeight
                width: root.keyColumns === 0 ? implicitWidth : sideSlot.width

                enabled: startup.ready
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        z: 10
        visible: !startup.ready

        color: Qt.rgba(Material.background.r, Material.background.g,
                       Material.background.b, 0.82)

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
        }

        ColumnLayout {
            anchors.centerIn: parent
            width: parent.width - 64
            spacing: 16

            BusyIndicator {
                Layout.alignment: Qt.AlignHCenter

                running: startup.problem === ""
                visible: running
            }

            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap

                text: startup.problem === ""
                      ? qsTr("Starting…")
                      : qsTr("The sound and SIP stack would not start, so this phone cannot place calls: %1").arg(startup.problem)
                color: startup.problem === "" ? Material.foreground
                                              : Ink.bad(Material.theme)
            }
        }
    }

    Rectangle {
        id: dndBand

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        height: calls.doNotDisturb && !root.isTheBar ? 30 : 0
        visible: height > 0

        color: Mark.bad

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 6
            spacing: 8

            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                color: "white"
                font.pixelSize: 13
                font.bold: true

                text: qsTr("Do not disturb — incoming calls are refused")
            }

            ToolButton {
                id: dndOff

                Layout.alignment: Qt.AlignVCenter
                implicitHeight: 24
                topInset: 0
                bottomInset: 0
                topPadding: 0
                bottomPadding: 0
                leftPadding: 10
                rightPadding: 10
                font.pixelSize: 12
                focusPolicy: Qt.NoFocus
                hoverEnabled: true
                text: qsTr("Turn off")
                Material.foreground: "white"

                background: Rectangle {
                    radius: height / 2
                    color: dndOff.hovered ? Qt.rgba(1, 1, 1, 0.18) : Qt.rgba(1, 1, 1, 0.10)
                }

                onClicked: calls.toggleDoNotDisturb()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        z: -1
        visible: calls.doNotDisturb
        readonly property real cast: root.isTheBar ? 0.18 : 0.08
        color: Qt.rgba(Material.background.r * (1 - cast) + Mark.bad.r * cast,
                       Material.background.g * (1 - cast) + Mark.bad.g * cast,
                       Material.background.b * (1 - cast) + Mark.bad.b * cast, 1)
    }

    Repeater {
        model: {
            void uiRegistry.slotRevision
            return uiRegistry.itemsIn("primary").slice(0, 1)
        }

        SlotLoader {
            id: primaryLoader

            required property string modelData

            what: primaryLoader.modelData
            page: root
            noun: "the primary page"

            visible: !root.isTheBar
            anchors.left: parent.left
            width: root.columnWidth
            anchors.top: dndBand.bottom

            anchors.bottom: footerRow.top

            anchors.bottomMargin: 6

            onLoaded: {
                root.primaryExtra = Qt.binding(() => primaryLoader.item.extraHeight ?? 0)
                root.pageHoldsTheMenu =
                    Qt.binding(() => primaryLoader.item.holdsTheWaysIn ?? false)
                root.primaryLeast =
                    Qt.binding(() => primaryLoader.item.leastHeight ?? shape.leastHeight)
            }
        }
    }

    ColumnLayout {
        id: nothingHere

        anchors.centerIn: parent
        width: parent.width - 64
        spacing: 16

        visible: !root.isTheBar && (function () {
            void uiRegistry.slotRevision
            return uiRegistry.itemsIn("primary").length === 0
        })()

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            opacity: 0.75

            text: qsTr("There is no dialler in this window. The module that draws one is turned off.")
        }
    }

    RowLayout {
        id: footerRow

        visible: !root.isTheBar
        anchors.left: parent.left

        anchors.bottom: root.keyColumns === 0
                        ? (root.zonesInside ? sectionsZone.top
                              : (root.waysInAsRow ? toolbarRow.top : parent.bottom))
                        : sideSlot.top

        anchors.bottomMargin: root.keyColumns === 0 ? 6 : 0

        anchors.leftMargin: root.pageAir
        width: root.columnWidth - 2 * root.pageAir
        spacing: 4

        ColumnLayout {
            id: footerItems

            Layout.fillWidth: true

            spacing: 0

            enabled: startup.ready

            Repeater {
                model: {
                    void uiRegistry.slotRevision
                    return uiRegistry.itemsIn("footer")
                }

                SlotLoader {
                    id: footerItem

                    required property string modelData

                    what: footerItem.modelData
                    page: root
                    noun: "the footer item"

                    Layout.fillWidth: true

                    visible: footerItem.item !== null && footerItem.item.offered
                }
            }
        }

        ToolButton {
            id: waysInHere

            visible: root.waysInAsMenu && !root.pageHoldsTheMenu

            Layout.alignment: Qt.AlignVCenter
            implicitHeight: 28
            implicitWidth: 28
            padding: 0

            icon.source: settings.glyphSet + "/more-vertical.svg"
            icon.width: 18
            icon.height: 18

            Accessible.name: qsTr("More")

            onClicked: root.showTheWaysIn(waysInHere)

            WaitingDot {
                shown: root.waysInMarked
            }

            background: Rectangle {
                radius: height / 2
                color: waysInHere.down
                       ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                                 Material.foreground.b, 0.16)
                       : waysInHere.hovered
                         ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                                   Material.foreground.b, 0.08)
                         : "transparent"
            }
        }
    }

    Item {
        id: toolbarRow

        visible: root.waysInAsRow

        anchors.left: parent.left
        anchors.leftMargin: root.pageAir
        width: root.columnWidth - 2 * root.pageAir
        anchors.bottom: parent.bottom

        anchors.bottomMargin: 6

        readonly property int rowHeight: 34
        readonly property int rowSpacing: 4
        readonly property int actSpacing: 4

        property var places: []
        property int rows: 0

        height: toolbarRow.rows * toolbarRow.rowHeight
                + Math.max(0, toolbarRow.rows - 1) * toolbarRow.rowSpacing

        enabled: startup.ready

        function measure() {

            if (toolbarRow.width <= 0) {
                toolbarRow.places = []
                toolbarRow.rows = 0
                return
            }

            const widths = []
            for (let at = 0; at < toolbarActs.count; ++at) {
                const held = toolbarActs.itemAt(at)
                widths.push(held !== null && held.wanted ? held.implicitWidth : 0)
            }

            widths.push(settingsAct.implicitWidth)

            const laid = shape.placeInRows(widths, toolbarRow.width, toolbarRow.actSpacing,
                                           toolbarRow.rowHeight, toolbarRow.rowSpacing)
            toolbarRow.places = laid.places
            toolbarRow.rows = laid.rows
        }

        function placeOf(index) {
            return index >= 0 && index < toolbarRow.places.length
                   ? toolbarRow.places[index]
                   : { x: 0, y: 0 }
        }

        Component.onCompleted: toolbarRow.measure()
        onWidthChanged: Qt.callLater(toolbarRow.measure)

        Repeater {
            id: toolbarActs

            model: {
                void uiRegistry.slotRevision
                return uiRegistry.itemsIn("toolbar")
            }

            onCountChanged: Qt.callLater(toolbarRow.measure)

            SlotLoader {
                id: toolbarItem

                required property string modelData
                required property int index

                what: toolbarItem.modelData
                page: root
                noun: "the toolbar act"

                readonly property bool wanted:
                    toolbarItem.item !== null && toolbarItem.item.offered

                visible: toolbarItem.wanted

                x: toolbarRow.placeOf(toolbarItem.index).x
                y: toolbarRow.placeOf(toolbarItem.index).y

                onWantedChanged: Qt.callLater(toolbarRow.measure)
                onImplicitWidthChanged: Qt.callLater(toolbarRow.measure)
            }
        }

        ToolbarAct {
            id: settingsAct

            text: qsTr("Settings")

            icon.source: settings.glyphSet + "/settings.svg"

            x: toolbarRow.placeOf(toolbarActs.count).x
            y: toolbarRow.placeOf(toolbarActs.count).y

            onImplicitWidthChanged: Qt.callLater(toolbarRow.measure)
            onClicked: root.showSettings()
        }
    }

    RowLayout {
        id: toolbarMenuRow

        visible: false

        anchors.left: parent.left
        width: root.columnWidth
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 6

        spacing: 4

        enabled: startup.ready

        Item {
            Layout.fillWidth: true
        }

        ToolButton {
            id: toolbarMenuButton

            implicitHeight: 34
            implicitWidth: 34
            padding: 0

            icon.source: settings.glyphSet + "/more-vertical.svg"
            icon.width: 20
            icon.height: 20

            Accessible.name: qsTr("More")

            onClicked: root.showTheWaysIn(toolbarMenuButton)

            WaitingDot {
                shown: root.waysInMarked
            }

            background: Rectangle {
                radius: height / 2
                color: toolbarMenuButton.down
                       ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                                 Material.foreground.b, 0.16)
                       : toolbarMenuButton.hovered
                         ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                                   Material.foreground.b, 0.08)
                         : "transparent"
            }
        }

        Item {
            Layout.fillWidth: true
        }
    }

    function showTheWaysIn(under) {
        const rows = []

        for (const loaded of toolbarRow.children) {
            const act = loaded.item !== undefined ? loaded.item : loaded
            if (act === null || act.text === undefined || act.text === "" || !act.offered) {
                continue
            }
            rows.push({
                label: act.text,
                offered: act.enabled,
                button: act,

                picture: act.icon.source,

                marked: act.marked === true
            })
        }

        waysInEntries.model = rows

        waysInMenu.present(under)
    }

    RightMenu {
        id: waysInMenu

        host: toolbarMenuButton

        Repeater {
            id: waysInEntries

            model: []

            TextMenuItem {
                required property var modelData

                text: modelData.label
                enabled: modelData.offered
                picture: modelData.picture
                marked: modelData.marked

                onTriggered: modelData.button.clicked()
            }
        }
    }
    property bool keypadShown: false

    readonly property bool keysOffered: calls.talking && !calls.inConference

    readonly property bool keysReachable: root.keysOffered && !calls.held

    readonly property bool trayAvailable: tray.available

    readonly property string settingsProblem: root.problemText(accounts.spotlightProblem)
    readonly property string settingsSummary: root.spotlightSentence()

    Loader {
        id: settingsFrame

        property bool everWanted: false

        active: settingsFrame.everWanted && !shape.inAWorkspace

        sourceComponent: SettingsWindow {

        transientParent: root

        glyphs: root.glyphs

        height: root.height

        registrationProblem: root.settingsProblem
        registrationSummary: root.settingsSummary

        trayAvailable: tray.available
        trayShown: root.trayShown

        phoneX: root.placeKnown ? root.wanted.x : root.x
        phoneY: root.placeKnown ? root.wanted.y : root.y

        takenRight: root.roomTaken(settingsFrame.item, "right")
        takenLeft: root.roomTaken(settingsFrame.item, "left")

        onDiagnosticsRequested: root.showPanelNamed("diagnostics")
        onApiPageRequested: (address) => Qt.openUrlExternally(address)
        onWindowRequested: (id, control) => root.showPanelNamed(id, control)
        onPhoneRequested: root.showTheWindow()
        }
    }

    Item {
        id: sectionsZone

        visible: root.zonesInside

        anchors.left: parent.left
        width: root.columnWidth

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 6
        height: root.sectionsHeight

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Qt.rgba(Material.foreground.r, Material.foreground.g,
                           Material.foreground.b, 0.1)
        }

        ListView {
            id: sectionsList

            anchors.fill: parent
            anchors.topMargin: 5
            anchors.leftMargin: root.pageAir
            anchors.rightMargin: root.pageAir
            clip: true

            model: root.sectionsOffered

            ScrollBar.vertical: ThinScrollBar {}

            delegate: ItemDelegate {
                id: sectionRow

                required property var modelData

                width: sectionsList.width
                height: 34

                readonly property bool open: zonesHere.showing === sectionRow.modelData.key

                text: sectionRow.modelData.label
                font.pixelSize: 13
                font.bold: sectionRow.open

                icon.source: sectionRow.modelData.glyph === ""
                             ? "" : root.glyph(sectionRow.modelData.glyph)
                icon.width: 18
                icon.height: 18
                display: AbstractButton.TextBesideIcon
                spacing: 10
                leftPadding: 8

                Rectangle {
                    readonly property bool shown: {
                        void uiRegistry.markRevision
                        return uiRegistry.marked(sectionRow.modelData.key)
                    }

                    visible: shown
                    width: 7
                    height: 7
                    radius: width / 2
                    color: Mark.bad

                    x: sectionRow.leftPadding + sectionRow.icon.width - 3
                    y: (sectionRow.height - sectionRow.icon.height) / 2 - 2
                }

                onClicked: zonesHere.show(sectionRow.modelData.key)

                background: Rectangle {
                    radius: 4
                    color: sectionRow.open
                           ? Qt.rgba(Material.accent.r, Material.accent.g,
                                     Material.accent.b, 0.12)
                           : sectionRow.hovered
                             ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                                       Material.foreground.b, 0.06)
                             : "transparent"
                }
            }
        }
    }

    readonly property bool workspaceApart: shape.inAWorkspace && !shape.phoneInTheWorkspace

    readonly property bool zonesInside: shape.inAWorkspace && shape.phoneInTheWorkspace

    readonly property int pageFloor: 360

    readonly property var sectionsOffered:
        zonesHere.tabsOffered.filter(one => one.window !== "diagnostics")

    readonly property int sectionsHeight: {
        const wants = root.sectionsOffered.length * 34 + 11
        const keys = root.keyBandNow === 0 ? 0 : root.keyBandNow + 6
        const room = root.height - dndBand.height - root.footerExtra - keys
                     - root.pageFloor - 24
        return Math.max(0, Math.min(wants, room))
    }

    readonly property int keyBandRows: 4

    readonly property int keyBandNow:
        root.keyColumns === 0 ? 0 : sideSlot.implicitHeight

    readonly property int columnEnds:
        root.columnWidth + (root.keyColumns === 0 ? sideSlot.implicitWidth : 0)

    readonly property bool isTheBar: shape.name === "bar"

    readonly property bool hasNoWindow: shape.name === "no-window"

    readonly property bool strandedWithoutATray: root.hasNoWindow && !tray.available

    function entriesBefore(instantiator, index) {
        let before = 0
        for (let at = 0; at < index; ++at) {
            if (instantiator.objectAt(at) !== null) {
                ++before
            }
        }
        return before
    }

    function entriesPlaced(instantiator, index) {
        let placed = 0
        for (let at = 0; at < instantiator.count; ++at) {
            if (at !== index && instantiator.objectAt(at) !== null) {
                ++placed
            }
        }
        return placed
    }

    function itemIndexIn(menu, item) {
        for (let at = 0; at < menu.items.length; ++at) {
            if (menu.items[at] === item) {
                return at
            }
        }
        return menu.items.length
    }

    readonly property var noCall: ({ who: "", callId: "", state: CallViewModel.Ended,
                                     held: false, heldByThem: false, ending: 0,
                                     inConference: false, current: false, live: false })

    Overview {
        id: overview

        onGreenCountChanged: telemetry.setOverviewDone(overview.greenCount)
        Component.onCompleted: telemetry.setOverviewDone(overview.greenCount)
    }

    readonly property var trayWindows: {
        void uiRegistry.slotRevision

        return uiRegistry.windows.filter(id => id !== "people" && id !== "diagnostics")
    }

    readonly property var trayDialLines: {
        void accounts.count

        const out = []
        for (let index = 0; index < accounts.count; ++index) {
            const line = accounts.accountAt(index)
            if (line.enabled === true && line.state === AccountsViewModel.Registered) {
                out.push({ id: line.id, label: line.label })
            }
        }
        return out
    }

    readonly property var trayCalls: {
        const out = []
        for (const one of calls.callList) {
            if (one.state === CallViewModel.Ended) {
                continue
            }
            out.push(one)
        }
        return out
    }

    function trayCallTitle(row) {

        void Qt.uiLanguage

        const state = root.callStateText(row.state, row.held, row.heldByThem,
                                         row.ending, row.inConference)
        return state.length > 0 ? qsTr("%1 — %2").arg(row.who).arg(state) : row.who
    }

    readonly property int barHeight: Math.max(40, barRow.implicitHeight)

    property string barEdge: windows.chose("bar.edge", "top")

    onBarEdgeChanged: windows.remembers("bar.edge", root.barEdge)

    function screenForTheBar() {
        if (settings.barScreen === "") {
            return root.screen
        }
        for (const one of Qt.application.screens) {
            if (one.name === settings.barScreen) {
                return one
            }
        }
        return root.screen
    }

    function screenForOurWindows() {
        const on = root.isTheBar ? root.screenForTheBar() : root.screen
        return on === null || on === undefined ? root.screen : on
    }

    function ourScreenHolds(x, y) {
        if (!root.isTheBar) {
            return true
        }

        const on = root.screenForOurWindows()
        if (on === null || on === undefined) {
            return true
        }
        return x >= on.virtualX && x < on.virtualX + on.width
               && y >= on.virtualY && y < on.virtualY + on.height
    }

    function roomBesideTheBar() {
        const on = root.screenForOurWindows()
        if (on === null || on === undefined) {
            return null
        }

        const band = root.isTheBar ? root.barHeight : 0

        const air = 12
        const top = root.barEdge === "top" ? on.virtualY + band : on.virtualY
        const foot = root.barEdge === "top"
                     ? on.virtualY + on.height
                     : on.virtualY + on.height - band

        return Qt.rect(on.virtualX + air, top + air,
                       Math.max(320, on.width - 2 * air),
                       Math.max(320, foot - top - 2 * air))
    }

    function placeTheBar() {
        if (!root.isTheBar) {
            return
        }

        const on = root.screenForTheBar()
        if (on === null) {
            return
        }

        if (root.screen !== on) {
            root.screen = on
        }

        root.width = on.width
        root.height = root.barHeight
        root.x = on.virtualX
        root.y = root.barEdge === "top"
                 ? on.virtualY
                 : on.virtualY + on.height - root.barHeight

        desktop.reserveEdge(root, root.barEdge, root.visible ? root.barHeight : 0)
    }

    Connections {
        target: root
        enabled: root.isTheBar

        function onBarHeightChanged() { root.placeTheBar() }
        function onBarEdgeChanged() { root.placeTheBar() }
        function onScreenChanged() { root.placeTheBar() }

        function onVisibleChanged() { Qt.callLater(root.placeTheBar) }
    }

    Connections {
        target: settings
        enabled: root.isTheBar

        function onAppearanceChanged() { root.placeTheBar() }
    }

    Timer {
        id: settleTheBar

        running: root.isTheBar && root.visible
        interval: 400
        repeat: false
        onTriggered: root.placeTheBar()
    }

    RowLayout {
        id: barRow

        visible: root.isTheBar

        anchors.fill: parent
        spacing: 4

        Repeater {
            model: {
                void uiRegistry.slotRevision
                return uiRegistry.itemsIn("bar").slice(0, 1)
            }

            SlotLoader {
                id: barItem

                required property string modelData

                what: barItem.modelData
                page: root
                noun: "the bar page"

                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        Repeater {
            model: {
                void uiRegistry.slotRevision
                return uiRegistry.itemsIn("bar-extras")
            }

            SlotLoader {
                id: barExtra

                required property string modelData

                what: barExtra.modelData
                page: root
                noun: "the bar extra"

                Layout.fillHeight: true
            }
        }

        ToolButton {
            id: barMenuButton

            implicitHeight: 34
            implicitWidth: 34
            padding: 0

            icon.source: settings.glyphSet + "/more-vertical.svg"
            icon.width: 20
            icon.height: 20

            Accessible.name: qsTr("Windows")

            onClicked: root.showTheWaysIn(barMenuButton)

            WaitingDot {
                shown: root.waysInMarked
            }
        }
    }

    Loader {
        id: workspaceApartWindow

        active: root.workspaceApart

        sourceComponent: WorkspaceWindow {
            phone: root

            transientParent: root.isTheBar || root.hasNoWindow ? null : root

            glyphs: root.glyphs
            phoneX: root.placeKnown ? root.wanted.x : root.x
            phoneY: root.placeKnown ? root.wanted.y : root.y
            takenRight: root.roomTaken(workspaceApartWindow.item, "right")
            takenLeft: root.roomTaken(workspaceApartWindow.item, "left")
        }

    }

    WorkspaceZones {
        id: zonesHere

        visible: root.zonesInside

        anchors.left: parent.left
        anchors.leftMargin: root.columnEnds
        anchors.right: parent.right
        anchors.top: dndBand.bottom

        anchors.bottom: parent.bottom

        page: root
        active: root.active
    }

    readonly property bool settingsShown:
        shape.inAWorkspace
        ? (root.zonesNow() !== null && root.zonesNow().showing === "settings")
        : (settingsFrame.item !== null && settingsFrame.item.visible)

    function showSettings(section) {

        if (!shape.inAWorkspace) {
            settingsFrame.everWanted = true
        }

        if (section !== undefined && section !== "") {
            if (shape.inAWorkspace) {
                const zones = root.zonesNow()
                if (zones !== null) {
                    zones.openSettingsAt(section)
                }
            } else if (settingsFrame.item !== null) {
                settingsFrame.item.section = section
            }
        }

        if (shape.inAWorkspace) {
            root.showInTheMiddle("settings")

            root.raiseTheZoneWindow()
        } else if (settingsFrame.item !== null) {
            settingsFrame.item.openBeside(root)
        }
    }

    readonly property string settingsName: qsTr("Settings")

    function settingsSections() {
        return uiRegistry.settingsSections
    }

    function hideSettings() {
        if (shape.inAWorkspace) {
            const zones = root.zonesNow()
            if (zones !== null) {
                zones.closeTab("settings")
            }
        } else if (settingsFrame.item !== null) {
            settingsFrame.item.hide()
        }
    }

    Loader {
        active: shots.wanted.length > 0
        sourceComponent: Component {
            Screenshots {
                phone: root
                settingsPanel: settingsFrame.item
            }
        }
    }

    property int primaryExtra: 0

    readonly property int footerExtra: footerRow.implicitHeight

    property var pluginWindows: ({})

    readonly property bool buildEveryWindow:
        Qt.application.arguments.indexOf("--build-every-page") >= 0

    function buildPluginWindows() {
        for (const id of uiRegistry.windows) {
            root.buildPanel(id)
        }
    }

    function buildPanel(id) {
        if (root.pluginWindows[id]) {
            return root.pluginWindows[id]
        }
        if (!root.hasWindow(id)) {
            return null
        }

        {

            const component = Qt.createComponent("BodyPanel.qml")
            if (component.status !== Component.Ready) {

                console.warn("the window", id, "did not load:", component.errorString())
                return null
            }

            const panel = component.createObject(root, { transientParent: root, windowId: id })
            if (panel === null) {
                console.warn("the window", id, "could not be created")
                return null
            }

            panel.height = Qt.binding(() => root.height)
            panel.glyphs = Qt.binding(() => root.glyphs)
            panel.phoneX = Qt.binding(() => root.placeKnown ? root.wanted.x : root.x)
            panel.phoneY = Qt.binding(() => root.placeKnown ? root.wanted.y : root.y)
            panel.takenRight = Qt.binding(() => root.roomTaken(panel, "right"))
            panel.takenLeft = Qt.binding(() => root.roomTaken(panel, "left"))

            if (panel.recordingsRequested !== undefined) {
                panel.recordingsRequested.connect((callId) => {

                    recordings.showCallsOf(callId, true)
                    root.showPanelNamed("recordings")
                })
            }

            root.pluginWindows[id] = panel
            return panel
        }
    }

    function panelNamed(id) {

        if (root.zoneFor(id) !== "") {
            return null
        }
        return root.buildPanel(id)
    }

    function zoneFor(id) {
        return shape.inAWorkspace ? shape.zoneOfWindow(id) : ""
    }

    function workspaceWindow() {
        return root.workspaceApart ? workspaceApartWindow.item : null
    }

    function zonesNow() {
        if (root.workspaceApart) {
            return workspaceApartWindow.item !== null ? workspaceApartWindow.item.zones : null
        }
        return root.zonesInside ? zonesHere : null
    }

    function showInTheMiddle(id, control) {
        const zones = root.zonesNow()
        if (zones !== null) {
            zones.reveal(id, control === undefined ? "" : control)
        }
    }

    function raiseTheZoneWindow() {
        const apart = root.workspaceWindow()
        if (apart !== null) {
            apart.showAgain()
        }
    }

    function raiseInAZone(id, control) {
        const zone = root.zoneFor(id)
        if (zone === "") {
            return false
        }
        if (zone === "main") {
            root.showInTheMiddle(id, control)
        }

        const apart = root.workspaceWindow()
        if (apart !== null) {
            apart.showAgain()
        }

        return true
    }

    function showPanelNamed(id, control) {
        if (root.raiseInAZone(id, control)) {
            return
        }
        const panel = root.panelNamed(id)
        if (panel === null) {
            return
        }

        if (!panel.visible) {
            panel.openBeside(root)
        } else {
            panel.raise()
            panel.requestActivate()
        }

        if (control !== undefined && control !== "") {
            panel.reveal(control)
        }
    }

    function hasWindow(id) {
        return uiRegistry.windows.indexOf(id) >= 0
    }

    function dropWithdrawnWindows() {
        for (const id in root.pluginWindows) {
            if (root.hasWindow(id)) {
                continue
            }

            const panel = root.pluginWindows[id]
            delete root.pluginWindows[id]
            if (panel === null || panel === undefined) {
                continue
            }

            panel.hide()
            root.panelClosed(panel)
            panel.destroy()
        }
    }

    Connections {
        target: uiRegistry

        function onWindowsChanged() {
            root.dropWithdrawnWindows()

            if (root.buildEveryWindow) {
                root.buildPluginWindows()
            }
        }
    }

    property var panelOrder: []

    function panelOpened(panel) {
        if (root.panelOrder.indexOf(panel) >= 0) {
            return
        }

        const opened = root.panelOrder.slice()
        opened.push(panel)
        root.panelOrder = opened
        root.panelsMoved()
    }

    function panelClosed(panel) {
        if (root.panelOrder.indexOf(panel) < 0) {
            return
        }
        root.panelOrder = root.panelOrder.filter(other => other !== panel)
        root.panelsMoved()
    }

    function roomTaken(panel, side) {
        let taken = 0
        for (const other of root.panelOrder) {
            if (other === panel) {
                break
            }
            if (other.visible && other.stuckTo === side) {
                taken += other.width
            }
        }
        return taken
    }

    function panelsMoved(held) {
        for (const panel of root.panelOrder) {
            if (panel.visible && panel !== held) {
                panel.restick()
            }
        }
    }

    Platform.SystemTrayIcon {
        id: tray

        visible: root.trayShown

        icon.source: trayModel.icon
        tooltip: root.trayTooltip()

        menu: Platform.Menu {
            id: trayMenu

            Platform.MenuItem {
                text: root.trayTooltip()
                enabled: false
            }

            Platform.MenuSeparator {}

            Instantiator {
                id: perCall

                model: root.trayCalls.length

                delegate: Platform.Menu {
                    id: callFolder

                    required property int index

                    readonly property var modelData: root.trayCalls[callFolder.index] !== undefined
                                                     ? root.trayCalls[callFolder.index]
                                                     : root.noCall

                    visible: root.trayCalls[callFolder.index] !== undefined

                    readonly property bool looking: callFolder.modelData.current

                    title: {
                        void Qt.uiLanguage
                        return root.trayCallTitle(callFolder.modelData)
                    }

                    Platform.MenuItem {
                        text: qsTr("Answer")
                        visible: callFolder.modelData.state === CallViewModel.RingingIn
                        onTriggered: {
                            calls.select(callFolder.modelData.callId)
                            calls.answer()
                            root.showTheWindow()
                        }
                    }

                    Platform.MenuItem {

                        text: qsTr("Decline")
                        visible: callFolder.modelData.state === CallViewModel.RingingIn
                        onTriggered: calls.rejectCall(callFolder.modelData.callId)
                    }

                    Platform.MenuItem {
                        text: callFolder.modelData.held ? qsTr("Resume") : qsTr("Hold")
                        visible: callFolder.modelData.state !== CallViewModel.RingingIn
                        onTriggered: calls.toggleHoldCall(callFolder.modelData.callId)
                    }

                    Platform.MenuItem {
                        text: trayModel.muted ? qsTr("Unmute") : qsTr("Mute")
                        visible: callFolder.modelData.live === true
                        onTriggered: calls.toggleMute()
                    }

                    Platform.MenuItem {
                        text: !calls.recording ? qsTr("Record")
                                               : (calls.recordingPaused
                                                  ? qsTr("Resume recording")
                                                  : qsTr("Pause recording"))
                        visible: callFolder.looking && calls.talking && calls.canRecord
                        enabled: calls.canStartRecording
                        onTriggered: calls.toggleRecording()
                    }

                    Platform.MenuSeparator {
                        visible: callFolder.looking
                                 && (calls.canTransfer || calls.canMerge || calls.inConference)
                    }

                    Platform.MenuItem {
                        text: qsTr("Transfer…")
                        visible: callFolder.looking && calls.canTransfer
                        onTriggered: {
                            root.showTheWindow()
                            root.askWhereToTransfer()
                        }
                    }

                    Platform.MenuItem {
                        text: {
                            void Qt.uiLanguage
                            return calls.mergePartner.length > 0
                                   ? qsTr("Merge with %1").arg(calls.mergePartner)
                                   : qsTr("Merge all")
                        }
                        visible: callFolder.looking && calls.canMerge
                        onTriggered: calls.merge()
                    }

                    Platform.MenuItem {
                        text: qsTr("Split")
                        visible: callFolder.looking && calls.inConference
                        onTriggered: calls.split()
                    }

                    Platform.MenuSeparator {
                        visible: callFolder.modelData.state !== CallViewModel.RingingIn
                    }

                    Platform.MenuItem {
                        text: qsTr("Hang up")
                        visible: callFolder.modelData.state !== CallViewModel.RingingIn
                        onTriggered: calls.hangUpCall(callFolder.modelData.callId)
                    }
                }

                onObjectAdded: (index, object) =>
                    trayMenu.insertMenu(2 + root.entriesBefore(perCall, index), object)
                onObjectRemoved: (index, object) => trayMenu.removeMenu(object)
            }

            Platform.MenuSeparator {
                visible: root.trayCalls.length > 0
            }

            Platform.MenuItem {
                text: settings.captureRunning
                      ? qsTr("Stop capturing %1").arg(settings.captureWhat)
                      : qsTr("Start capture")
                visible: settings.captureAvailable && settings.captureEnabled
                onTriggered: settings.captureRunning
                             ? settings.stopRecordingHere()
                             : settings.startRecordingHere(qsTr("Another application"))
            }

            Platform.MenuSeparator {
                visible: settings.captureAvailable && settings.captureEnabled
            }

            Platform.MenuItem {

                text: qsTr("Dial…")
                visible: root.trayDialLines.length < 2
                onTriggered: root.askForANumber()
            }

            Platform.Menu {
                id: dialFrom

                title: qsTr("Dial…")
                visible: root.trayDialLines.length > 1

                Instantiator {
                    id: dialLines

                    model: root.trayDialLines.length > 1 ? root.trayDialLines.length : 0

                    delegate: Platform.MenuItem {
                        required property int index

                        readonly property var line:
                            root.trayDialLines[index] !== undefined
                            ? root.trayDialLines[index] : { id: "", label: "" }

                        text: line.label
                        onTriggered: {
                            accounts.chooseDialAccount(line.id)
                            root.askForANumber()
                        }
                    }

                    onObjectAdded: (index, object) =>
                        dialFrom.insertItem(root.entriesBefore(dialLines, index), object)
                    onObjectRemoved: (index, object) => dialFrom.removeItem(object)
                }
            }

            Platform.MenuSeparator {}

            Platform.MenuItem {
                text: root.onScreen ? qsTr("Hide the phone") : qsTr("Show the phone")
                visible: !root.hasNoWindow && !root.isTheBar
                onTriggered: root.toggleWindow()
            }

            Platform.MenuItem {
                text: qsTr("Contacts…")

                visible: root.panelHasTab("people", "directory.contacts")
                onTriggered: {
                    root.showTheWindow()
                    root.showPanelSection("people", "directory.contacts")
                }
            }

            Platform.MenuItem {
                text: qsTr("Call history…")

                visible: root.panelHasTab("people", "softphone.history")
                onTriggered: {
                    root.showTheWindow()
                    root.showPanelSection("people", "softphone.history")
                }
            }

            Platform.MenuItem {
                id: settingsEntry

                text: qsTr("Settings…")
                onTriggered: {
                    root.showTheWindow()
                    root.showSettings()
                }
            }

            Platform.MenuSeparator {}

            Platform.MenuItem {
                text: qsTr("Quit")
                onTriggered: root.quitApplication()
            }

            Instantiator {
                id: otherWindows

                model: root.trayWindows.length

                delegate: Platform.MenuItem {
                    required property int index

                    readonly property string which:
                        root.trayWindows[index] !== undefined ? root.trayWindows[index] : ""

                    text: {
                        void uiRegistry.revision
                        return which === "" ? "" : qsTr("%1…").arg(uiRegistry.windowTitle(which))
                    }

                    onTriggered: {
                        root.showTheWindow()
                        root.showPanelNamed(which)
                    }
                }

                onObjectAdded: (index, object) => trayMenu.insertItem(
                                   Math.max(0, root.itemIndexIn(trayMenu, settingsEntry)
                                               - root.entriesPlaced(otherWindows, index)
                                               + root.entriesBefore(otherWindows, index)),
                                   object)
                onObjectRemoved: (index, object) => trayMenu.removeItem(object)
            }

        }

        onMessageClicked: root.showTheWindow()

        onActivated: function(reason) {
            if (reason !== Platform.SystemTrayIcon.Trigger
                    && reason !== Platform.SystemTrayIcon.DoubleClick) {
                return
            }
            if (root.isTheBar) {
                root.showTheWindow()
                return
            }
            root.toggleWindow()
        }
    }

    function keepsRunningOnClose() {
        return settings.closeToTray
    }

    function putThePhoneAway() {
        root.hidePanels()

        if (!root.trayShown) {
            root.showMinimized()
            return
        }

        root.hide()
        root.noteHiding()
    }

    function showTheWindow() {

        if (root.hasNoWindow && !root.strandedWithoutATray) {
            return
        }

        if (root.visibility === Window.Minimized) {
            root.showNormal()
        } else {
            root.show()
        }

        root.raise()
        root.requestActivate()

        desktop.bringToFront(root)
    }

    function askForANumber() {
        if (dialAsking.item === null) {
            dialAsking.active = true
        }
        if (dialAsking.item !== null) {
            dialAsking.item.ask()
        }
    }

    Loader {
        id: dialAsking

        active: false
        sourceComponent: DialWindow {
            phone: root
            transientParent: root
        }
    }

    function toggleWindow() {
        if (root.onScreen) {

            root.putThePhoneAway()
            return
        }
        root.showTheWindow()
    }

    CallBanner {
        id: banner

        darkWanted: root.darkWanted
        glyphs: root.glyphs

        glyphSize: Math.min(root.cardGlyphSize, 30)

        onAnswered: {
            calls.answer()
            banner.hide()
            root.showTheWindow()
        }

        onRejected: {
            calls.reject()
            banner.hide()
        }

        onShown: {
            banner.hide()
            root.showTheWindow()
        }
    }

    CaptureNotice {
        id: captureNotice

        darkWanted: root.darkWanted

        onRecorded: {
            settings.startCaptureOf(captureNotice.what, captureNotice.process)
            captureNotice.hide()
        }

        onDeclined: captureNotice.hide()

        onAlways: {

            settings.setCapturePolicyFor(captureNotice.process,
                                         SettingsViewModel.OnFinding.Record)
            settings.startCaptureOf(captureNotice.what, captureNotice.process)
            captureNotice.hide()
        }
    }

    Connections {
        target: settings

        function onCaptureFound(what, process) {
            captureNotice.what = what
            captureNotice.process = process

            const at = desktop.noticePlace(captureNotice.width, captureNotice.height)
            if (at.width > 0) {
                captureNotice.x = at.x
                captureNotice.y = at.y
            }
            captureNotice.show()
            captureNotice.raise()
        }

        function onCaptureLost() { captureNotice.hide() }

        function onCaptureStateChanged() {
            if (settings.captureRunning) {
                captureNotice.hide()
            }
        }
    }

    Connections {
        target: calls

        function onRedialGotThrough() {
            root.showTheWindow()
        }
    }

    function showBanner() {
        banner.who = calls.offeredLabel.length > 0 ? calls.offeredLabel : trayModel.callLabel
        banner.number = calls.offeredDetail
        banner.line = accounts.severalLines ? accounts.labelOf(calls.accountId) : ""

        const at = desktop.noticePlace(banner.width, banner.height)
        if (at.width > 0) {
            banner.x = at.x
            banner.y = at.y
        }

        banner.show()
        banner.raise()
    }

    Connections {
        target: calls
        function onRingingStopped() { banner.hide() }
    }

    function announceRinging() {
        switch (settings.callNotice) {
        case SettingsViewModel.Notice.Silent:

            return

        case SettingsViewModel.Notice.Raise:

            root.showTheWindow()
            root.alert(0)
            return

        default:

            if (root.active && root.onScreen) {
                console.log("no banner for a call that is ringing: the phone says it is in front — "
                            + "active=" + root.active + " onScreen=" + root.onScreen)
                return
            }

            root.showBanner()

            if (root.visible) {
                root.alert(0)
            }
            return
        }
    }

    Connections {
        target: calls
        function onRingingStarted() { root.announceRinging() }
    }

    Connections {
        target: instance
        function onShowRequested() { root.showTheWindow() }

        function onDialRequested(target) {
            root.showTheWindow()
            calls.dialText = target

            if (settings.callLinkDialsAtOnce && calls.canDial) {
                calls.dial()
            }
        }

        function onQuitRequested() {
            root.hidePanels()
            root.leave()
        }
    }

    Connections {
        target: updates
        function onRestartWanted(image) { root.leave() }
    }

    Connections {
        target: settings
        function onLayoutRestartWanted() { root.leave() }
    }

    function noteHiding() {
        if (!root.trayShown || !tray.supportsMessages
                || windows.noticeGiven("close-to-tray")) {
            return
        }

        windows.rememberNotice("close-to-tray")
        tray.showMessage(qsTr("The phone is still running"),
                         qsTr("Calls still arrive. The icon here brings the window back, and Settings → Startup turns this off."))
    }

    property bool remindedAboutCapture: false

    Connections {
        target: settings

        function onCaptureStateChanged() {
            if (!settings.captureRunning) {
                root.remindedAboutCapture = false
                return
            }
            if (root.remindedAboutCapture || !settings.captureRemind
                    || !root.trayShown || !tray.supportsMessages) {
                return
            }
            root.remindedAboutCapture = true
            tray.showMessage(qsTr("Recording %1").arg(settings.captureWhat),
                             qsTr("Tell the others that you are recording. This phone cannot announce it for you."))
        }
    }

    function quitApplication() {
        if (!trayModel.inCall && !trayModel.ringing && !settings.captureRunning) {

            root.hidePanels()
            root.leave()
            return
        }

        root.showTheWindow()
        quitDialog.open()
    }

    property bool quitting: false

    function leave() {
        root.quitting = true
        Qt.quit()
    }

    Loader {
        id: telemetryNoticeLoader

        active: telemetry.noticeWanted
        sourceComponent: TelemetryNotice { phone: root }
        onLoaded: item.ask()
    }

    Connections {
        target: telemetry

        function onChanged() {
            if (telemetry.noticeWanted && telemetryNoticeLoader.item) {
                telemetryNoticeLoader.item.ask()
            }
        }
    }

    Loader {
        id: shapeChoiceLoader

        active: {
            void windows.noticeRevision
            return !telemetry.noticeWanted && !windows.noticeGiven("layout-1")
        }
        onLoaded: item.ask()

        sourceComponent: ShapeChoice {
            phone: root
            trayAvailable: root.trayAvailable
        }
    }

    Loader {
        id: menuEntryLoader

        active: {
            void windows.noticeRevision
            return settings.menuEntryUnanswered && !telemetry.noticeWanted
                   && windows.noticeGiven("layout-1")
        }

        sourceComponent: MenuEntryOffer {
            phone: root

            onAnswered: (wanted) => settings.setMenuEntry(wanted)
        }

        onLoaded: item.ask()
    }

    ConfirmDialog {
        id: databaseDialog

        title: qsTr("Database cannot be read")

        property bool asked: false
        actionText: qsTr("Start again")
        actionEnabled: !storage.working
        onAccepted: {
            storage.startAgain()
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("This phone's database was made by a different version and this one cannot read it. There is no way to bring it across.\n\nStarting again makes an empty one: every contact and every call in the history goes, and that cannot be undone. Calls themselves are not affected.")
        }
    }

    Connections {
        target: storage

        function onProblemChanged() {
            if (storage.problem === StorageViewModel.Unreadable && !databaseDialog.asked) {
                databaseDialog.asked = true
                databaseDialog.open()
            }
        }
    }

    ConfirmDialog {
        id: newVersionDialog

        title: qsTr("A new version is available")

        actionText: updates.selfReplacing ? qsTr("Update now") : qsTr("Get the package")
        destructive: false

        onAccepted: {
            if (updates.selfReplacing) {
                updates.downloadAndInstall()

                root.showSettings("about")
            } else {
                updates.openDownload()
                updates.notNow()
            }
        }

        onClosed: if (updates.announcing) {
            updates.notNow()
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Version %1 is ready to install. The phone will restart into it.")
                  .arg(updates.offered)
            visible: updates.selfReplacing
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Version %1 is available. This build is installed from a package, so the "
                       + "new one is installed the way this one was.").arg(updates.offered)
            visible: !updates.selfReplacing
        }
    }

    Connections {
        target: updates

        function onChanged() {
            if (updates.announcing && !newVersionDialog.visible) {
                newVersionDialog.open()
            }
        }
    }

    ConfirmDialog {
        id: quitDialog

        title: qsTr("Quit the phone?")
        actionText: qsTr("Quit")
        onAccepted: {
            root.leave()
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap

            text: trayModel.ringing
                ? qsTr("%1 is calling. Quitting will refuse the call.").arg(trayModel.callLabel)
                : trayModel.inCall
                  ? qsTr("You are on a call with %1. Quitting will hang it up.").arg(trayModel.callLabel)

                  : qsTr("%1 is being captured. Quitting will stop the recording.")
                    .arg(settings.captureWhat)
        }
    }

    function trayTooltip() {

        void Qt.uiLanguage

        if (trayModel.recording) {
            if (trayModel.ringing) {
                return qsTr("Recording. %1 is calling").arg(trayModel.callLabel)
            }
            return trayModel.recordings > 1
                 ? qsTr("Recording %n conversation(s)", "", trayModel.recordings)
                 : qsTr("Recording: %1").arg(trayModel.callLabel)
        }

        if (settings.captureRunning) {
            return qsTr("Capturing: %1").arg(settings.captureWhat)
        }

        if (trayModel.ringing) {
            return qsTr("%1 is calling").arg(trayModel.callLabel)
        }

        if (trayModel.inCall) {
            return trayModel.held
                 ? qsTr("On hold: %1").arg(trayModel.callLabel)
                 : qsTr("On a call with %1").arg(trayModel.callLabel)
        }

        void uiRegistry.slotRevision
        if (uiRegistry.itemsIn("primary").indexOf("softphone.dialler") < 0) {
            return qsTr("The telephone is turned off")
        }

        if (trayModel.lines === 0) {
            return qsTr("No account is set up")
        }

        if (trayModel.connected === 0) {
            return trayModel.failed > 0
                 ? qsTr("Not connected: %1").arg(trayModel.lineLabel)
                 : qsTr("Connecting…")
        }

        if (trayModel.failed > 0) {
            return qsTr("%n line(s) connected; %1 is not", "", trayModel.connected)
                .arg(trayModel.lineLabel)
        }

        return qsTr("%n line(s) connected", "", trayModel.connected)
    }

    onClosing: function(close) {

        if (root.quitting) {
            close.accepted = true
            return
        }

        if (root.keepsRunningOnClose()) {
            close.accepted = false
            root.putThePhoneAway()
            return
        }

        close.accepted = false
        root.quitApplication()
    }

    function showPanelSection(id, which) {
        if (root.zoneFor(id) !== "") {
            root.showInTheMiddle(which)

            root.raiseTheZoneWindow()
            return
        }

        const panel = root.panelNamed(id)
        if (panel !== null) {
            panel.showSection(which)
            if (!panel.visible) {
                panel.openBeside(root)
            } else {
                panel.raise()
                panel.requestActivate()
            }
        }
    }

    function panelHasTab(id, tab) {
        void uiRegistry.slotRevision
        return uiRegistry.itemsIn(id + ".tabs").indexOf(tab) >= 0
    }

    Shortcut {
        sequences: [StandardKey.Preferences, "Ctrl+,"]
        onActivated: root.showSettings()
    }

    Instantiator {
        model: settings.hotkeys

        delegate: Shortcut {
            required property var modelData

            sequence: modelData.sequence

            enabled: modelData.sequence.length > 0 && !modelData.global
                     && !settings.capturingKeys

            context: Qt.WindowShortcut

            onActivated: root.runHotkey(modelData.action)
        }
    }

    Connections {
        target: settings
        function onHotkeyActivated(action) { root.runHotkey(action) }
    }

    function runHotkey(action) {
        switch (action) {
        case SettingsViewModel.Action.Answer:
            if (trayModel.canAnswer) {
                calls.answer()
                root.showTheWindow()
            }
            break
        case SettingsViewModel.Action.HangUp:
            if (trayModel.canReject) {
                calls.reject()
            } else if (trayModel.canHangUp) {
                calls.hangUp()
            }
            break
        case SettingsViewModel.Action.HoldResume:
            if (calls.hasCall) {
                calls.toggleHold()
            }
            break
        case SettingsViewModel.Action.Mute:
            if (trayModel.canMute) {
                calls.toggleMute()
            }
            break
        case SettingsViewModel.Action.ShowWindow:
            root.toggleWindow()
            break
        }
    }

    property var panelsHiddenWithWindow: []

    function hidePanels() {
        const open = []
        for (const panel of root.panelOrder) {
            if (panel.visible) {
                open.push(panel)
            }
        }

        root.panelsHiddenWithWindow = open
        for (const panel of open) {

            panel.awayWithThePhone = true
            panel.hide()
        }
    }

    function showPanels() {
        const open = root.panelsHiddenWithWindow
        root.panelsHiddenWithWindow = []

        for (const panel of open) {

            panel.showInPlace()

            panel.awayWithThePhone = false
        }
    }

    onVisibilityChanged: {
        if (root.visibility === Window.Minimized) {

            if (root.panelOrder.length > 0) {
                root.hidePanels()
            }
            return
        }

        if (root.visibility === Window.Hidden) {
            return
        }

        root.takeThePlaceBack()

        root.showPanels()
    }

    readonly property string viewKeys: "dialpad"
    readonly property string viewCalls: "history"
    readonly property string viewNothing: "nothing"

    property string wantedIdle: windows.chose("phone.idle", root.viewCalls)
    property string wantedOnACall: windows.chose("phone.on-a-call", root.viewNothing)

    property bool callPresent: calls.hasCall

    readonly property bool anyHistory: history.available && history.anyNewest

    readonly property string mainView: {
        const wanted = root.callPresent ? root.wantedOnACall : root.wantedIdle
        return wanted === root.viewCalls && !root.anyHistory ? root.viewKeys : wanted
    }

    readonly property bool dialpadShown: root.mainView === root.viewKeys

    readonly property string nextView: {
        const order = [root.viewKeys, root.viewCalls, root.viewNothing]
        const at = order.indexOf(root.mainView)
        for (let step = 1; step <= order.length; ++step) {
            const candidate = order[(at + step) % order.length]
            if (candidate === root.viewCalls && !root.anyHistory) {
                continue
            }
            return candidate
        }
        return root.mainView
    }

    function callStamp(ms) {
        void Qt.uiLanguage

        const at = new Date(ms)
        const today = new Date()
        const sameDay = at.getFullYear() === today.getFullYear()
                        && at.getMonth() === today.getMonth()
                        && at.getDate() === today.getDate()
        return sameDay ? at.toLocaleTimeString(Qt.locale(), Locale.ShortFormat)
                       : at.toLocaleDateString(Qt.locale(), Locale.ShortFormat)
    }

    function viewGlyph(view) {
        switch (view) {
        case root.viewCalls:
            return root.glyph("history")
        case root.viewNothing:
            return root.glyph("chevron-up")
        default:
            return root.glyph("dialpad")
        }
    }

    function showNextView() {
        const wanted = root.nextView
        if (root.callPresent) {
            root.wantedOnACall = wanted
            windows.remembers("phone.on-a-call", wanted)
        } else {
            root.wantedIdle = wanted
            windows.remembers("phone.idle", wanted)
        }
    }

    readonly property int dialKeySize: shape.dialKey

    readonly property int dialDigitSize: shape.dialDigit

    readonly property bool keyLetters: shape.keyLetters

    readonly property int pageAir: shape.air

    readonly property int pageEdge: shape.edge

    readonly property int cardNameSize: shape.cardName

    readonly property int cardButtonPad: shape.cardPad

    readonly property bool keysNeedARule: root.footerExtra === 0

    readonly property int pageRowGap: shape.rowGap

    readonly property int fieldHeight: shape.field

    readonly property int dialogTitleSize:
        shape.field >= 56 ? root.Material.dialogTitleFontPixelSize : 16
    readonly property int dialogPad: shape.field >= 56 ? 24 : 14

    readonly property bool chipsCentred: shape.chipsCentred

    readonly property int keyStripWidth: shape.keyStrip

    readonly property var dialKeys: [
        { digit: "1", letters: "" },
        { digit: "2", letters: "ABC" },
        { digit: "3", letters: "DEF" },
        { digit: "4", letters: "GHI" },
        { digit: "5", letters: "JKL" },
        { digit: "6", letters: "MNO" },
        { digit: "7", letters: "PQRS" },
        { digit: "8", letters: "TUV" },
        { digit: "9", letters: "WXYZ" },
        { digit: "*", letters: "" },

        { digit: "0", letters: "", whenHeld: "+" },
        { digit: "#", letters: "" },
    ]

    readonly property string expandGlyph: "\u25BE"
    readonly property string collapseGlyph: "\u25B4"
    readonly property string separatorGlyph: "\u00B7"
    readonly property string clearGlyph: "\u2715"

    Timer {

        running: calls.hasCall
        interval: 1000
        repeat: true
        onTriggered: calls.tick()
    }
}
