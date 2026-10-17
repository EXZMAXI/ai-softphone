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
    id: phone

    required property var page

    readonly property int pageAir: phone.page.pageAir

    readonly property int pageEdge: phone.page.pageEdge

    readonly property int rowGap: phone.page.pageRowGap
    readonly property int fieldHeight: phone.page.fieldHeight

    readonly property bool severalChips: accounts.count > 1

    readonly property bool holdsTheWaysIn: true

    readonly property int rowAir: 8

    readonly property int extraHeight: Math.max(0, lineStrip.needed - 28)

    component RecordingMark: Rectangle {
        implicitWidth: 10
        implicitHeight: 10
        radius: 5
        color: calls.recordingKeeping ? Mark.bad : "transparent"
        border.width: calls.recordingKeeping ? 0 : 2
        border.color: Mark.bad

        SequentialAnimation on opacity {
            running: calls.recordingShown && calls.recordingKeeping
            loops: Animation.Infinite
            NumberAnimation { to: 0.35; duration: 900 }
            NumberAnimation { to: 1.0; duration: 900 }
        }
    }

    component CardButton: Button {
        id: cardButton

        required property string glyph

        property string hintText: cardButton.text

        topInset: 0
        bottomInset: 0

        topPadding: phone.page.cardButtonPad
        bottomPadding: phone.page.cardButtonPad

        Layout.fillWidth: true

        focusPolicy: Qt.NoFocus

        icon.source: phone.page.glyph(cardButton.glyph)
        icon.width: phone.page.cardGlyphSize
        icon.height: phone.page.cardGlyphSize
        Layout.preferredHeight: phone.page.cardButtonHeight
        display: AbstractButton.IconOnly

        Accessible.name: text

        Hint {
            owner: cardButton
            visible: cardButton.hovered
            text: cardButton.hintText
        }
    }

    component CallRowButton: RowButton {
        glyphs: phone.page.glyphs
        box: 40
        target: 36
        circle: 32
        focusPolicy: Qt.NoFocus

        tintOpacity: 1.0
    }

    component CallKey: Button {
        id: callKey

        required property int keySize

        implicitWidth: keySize
        implicitHeight: keySize
        Layout.preferredWidth: keySize
        Layout.preferredHeight: keySize

        enabled: (calls.canDial || calls.lastDialled !== "") && accounts.connected

        focusPolicy: Qt.NoFocus
        padding: 0

        topInset: 0
        bottomInset: 0
        leftInset: 0
        rightInset: 0

        Accessible.name: text

        Hint {
            owner: callKey
            visible: callKey.hovered
            text: callKey.text
        }

        onClicked: {
            if (calls.canDial) {
                calls.dial()
            } else {
                calls.offerLastDialled()
            }
        }

        background: Rectangle {
            radius: width / 2

            color: callKey.enabled
                   ? (callKey.down
                      ? Qt.darker(Mark.good, 1.35)
                      : (callKey.hovered ? Qt.lighter(Mark.good, 1.25) : Mark.good))
                   : Qt.rgba(Material.foreground.r, Material.foreground.g,
                             Material.foreground.b, 0.10)
        }

        text: qsTr("Call")
        icon.source: phone.page.glyph("call")

        icon.color: callKey.enabled
                    ? "white"
                    : Qt.rgba(Material.foreground.r, Material.foreground.g,
                              Material.foreground.b, 0.35)

        readonly property int glyphSize: Math.round(callKey.keySize * phone.page.keyGlyphShare)

        icon.width: callKey.glyphSize
        icon.height: callKey.glyphSize
        display: AbstractButton.IconOnly

        contentItem: Item {
            Glyph {
                anchors.centerIn: parent
                width: callKey.glyphSize
                height: callKey.glyphSize
                source: phone.page.glyph("call")
                color: callKey.icon.color
            }
        }
    }

        anchors.fill: parent

        anchors.leftMargin: 0

        enabled: startup.ready

        anchors.topMargin: 0

        anchors.bottomMargin: 0
        spacing: 16

        Keys.onEscapePressed: phone.page.dropFocus()

        ScrollView {
            id: scroller

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            leftPadding: phone.pageEdge
            rightPadding: phone.pageEdge

            contentWidth: availableWidth

            ScrollBar.vertical: ThinScrollBar {
                parent: scroller
                x: scroller.mirrored ? 0 : scroller.width - width
                y: scroller.topPadding
                height: scroller.availableHeight
            }

            ColumnLayout {
                id: content

                width: scroller.availableWidth

                height: Math.max(implicitHeight, scroller.availableHeight)
                spacing: phone.rowGap

                TapHandler {
                    onTapped: phone.page.dropFocus()
                }

                Item { Layout.preferredHeight: phone.pageAir }

                RowLayout {
                    Layout.leftMargin: phone.pageAir
                    Layout.rightMargin: phone.pageAir
                    Layout.fillWidth: true

                    enabled: accounts.connected
                    spacing: 12

                    DialField {
                        id: dialField

                        implicitHeight: phone.fieldHeight

                        Layout.fillWidth: true
                        cleaner: calls

                        rightPadding: fieldButton.width + 4
                        placeholderText: qsTr("Number")
                        onDialEdited: dialled => calls.dialText = dialled
                        onAccepted: calls.dial()

                        Binding {
                            target: dialField
                            property: "text"
                            value: calls.dialText
                            restoreMode: Binding.RestoreNone
                        }

                        ToolButton {
                            id: fieldButton

                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            flat: true

                            focusPolicy: Qt.NoFocus

                            display: AbstractButton.IconOnly
                            icon.source: calls.dialText.length > 0
                                         ? phone.page.glyph("dismiss")
                                         : phone.page.viewGlyph(phone.page.nextView)
                            icon.width: 16
                            icon.height: 16

                            text: {
                                void Qt.uiLanguage

                                if (calls.dialText.length > 0) {
                                    return qsTr("Clear the number")
                                }
                                switch (phone.page.nextView) {
                                case phone.page.viewCalls:
                                    return qsTr("Show the last calls")
                                case phone.page.viewNothing:
                                    return phone.page.mainView === phone.page.viewCalls
                                           ? qsTr("Hide the last calls")
                                           : qsTr("Hide the dialling keys")
                                default:
                                    return qsTr("Show the dialling keys")
                                }
                            }
                            Accessible.name: text

                            onClicked: {
                                if (calls.dialText.length > 0)
                                    calls.dialText = ""
                                else
                                    phone.page.showNextView()
                            }

                            Hint {
                                owner: parent
                                visible: parent.hovered
                                text: parent.text
                            }
                        }
                    }

                    CallKey {
                        visible: !phone.page.dialpadShown

                        keySize: dialField.implicitHeight
                    }
                }

                ColumnLayout {

                    Layout.alignment: Qt.AlignHCenter
                    spacing: 8
                    visible: phone.page.dialpadShown

                    GridLayout {
                        enabled: accounts.connected
                        columns: 3
                        columnSpacing: 28
                        rowSpacing: 8

                        Repeater {
                            model: phone.page.dialKeys

                            Button {
                                id: dialKey

                                required property var modelData

                                Layout.preferredWidth: phone.page.dialKeySize
                                Layout.preferredHeight: phone.page.dialKeySize
                                flat: true

                                topInset: 0
                                bottomInset: 0
                                leftInset: 0
                                rightInset: 0
                                padding: 0

                                focusPolicy: Qt.NoFocus
                                Accessible.name: dialKey.modelData.digit
                                onClicked: calls.dialText = calls.dialText
                                           + dialKey.modelData.digit

                                onPressAndHold: {
                                    if (dialKey.modelData.whenHeld) {
                                        calls.dialText = calls.dialText
                                                         + dialKey.modelData.whenHeld
                                    }
                                }

                                background: Rectangle {
                                    radius: width / 2
                                    color: Qt.rgba(Material.foreground.r, Material.foreground.g,
                                                   Material.foreground.b,
                                                   dialKey.down ? 0.24
                                                                : (dialKey.hovered ? 0.14 : 0.07))
                                }

                                contentItem: ColumnLayout {
                                    spacing: 0

                                    Label {
                                        Layout.fillWidth: true
                                        horizontalAlignment: Text.AlignHCenter
                                        text: dialKey.modelData.digit
                                        font.pixelSize: phone.page.dialDigitSize
                                        font.weight: Font.DemiBold
                                    }

                                    Label {

                                        visible: phone.page.keyLetters

                                        Layout.fillWidth: true

                                        Layout.preferredHeight: 13
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter

                                        text: dialKey.modelData.letters
                                              || dialKey.modelData.whenHeld || ""

                                        font.pixelSize: dialKey.modelData.letters ? 9 : 16
                                        font.letterSpacing: 1.5
                                        opacity: 0.6
                                    }
                                }
                            }
                        }
                    }

                    CallKey {
                        Layout.alignment: Qt.AlignHCenter
                        keySize: phone.page.dialKeySize
                    }
                }

                Frame {
                    Layout.leftMargin: phone.pageAir
                    Layout.rightMargin: phone.pageAir
                    id: callCard

                    RowMenu {
                        number: calls.remoteDetail !== "" ? calls.remoteDetail
                                                          : calls.remoteLabel
                    }

                    Layout.fillWidth: true

                    visible: calls.hasCall && !calls.offeringWhileBusy

                    implicitHeight: cardBody.implicitHeight + callCard.topPadding
                                    + callCard.bottomPadding

                    background: Rectangle {
                        radius: callCard.Material.roundedScale
                        color: "transparent"
                        border.color: callCard.Material.frameColor
                        clip: true

                        SignalDrawing {
                            anchors.fill: parent
                            visible: sound.showing && sound.behind
                            feed: sound
                            shape: SignalDrawing.Aurora
                            localColour: phone.page.nearColour
                            remoteColour: phone.page.farColour
                            faint: phone.page.faintColour
                        }
                    }

                    ColumnLayout {
                        id: cardBody

                        anchors.fill: parent
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            SignalDrawing {
                                Layout.preferredWidth: 7
                                Layout.fillHeight: true
                                visible: sound.showing && sound.flanking
                                feed: sound
                                shape: SignalDrawing.Upright
                                side: SignalDrawing.Remote
                                localColour: phone.page.nearColour
                                remoteColour: phone.page.farColour
                                faint: phone.page.faintColour
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                RowLayout {
                                    Layout.alignment: Qt.AlignHCenter
                                    spacing: 8

                                    visible: !calls.inConference

                                    Label {
                                        visible: !calls.inConference

                                        text: phone.page.callStateText(calls.state, calls.held,
                                                                 calls.heldByThem, calls.ending)

                                        color: calls.heldByThem ? Ink.warn(Material.theme)
                                                                : Material.foreground
                                        opacity: 0.7
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        visible: !calls.inConference
                                                 && (calls.connectedAtMs > 0 || phone.page.ringingOut)
                                        text: phone.page.separatorGlyph
                                        opacity: 0.4
                                    }

                                    Label {
                                        visible: !calls.inConference
                                                 && (calls.connectedAtMs > 0 || phone.page.ringingOut)
                                        text: calls.connectedAtMs > 0
                                              ? phone.page.duration(calls.connectedAtMs, calls.endedAtMs)
                                              : phone.page.duration(calls.startedAtMs, 0)
                                        opacity: 0.7
                                    }

                                    Label {
                                        visible: calls.recordingShown
                                        text: phone.page.separatorGlyph
                                        opacity: 0.4
                                    }

                                    RecordingMark {
                                        visible: calls.recordingShown
                                        Layout.alignment: Qt.AlignVCenter
                                    }

                                    Label {
                                        visible: calls.recordingShown && calls.recordingPaused
                                        text: qsTr("Paused")
                                        color: Ink.bad(Material.theme)
                                        opacity: 0.85
                                    }

                                    Label {
                                        visible: prompter.running
                                        text: phone.page.separatorGlyph
                                        opacity: 0.4
                                    }

                                    Label {

                                        text: qsTr("Listening")
                                        visible: prompter.running
                                        color: Ink.warn(Material.theme)
                                        opacity: 0.85
                                    }
                                }

                                Label {
                                    Layout.fillWidth: true
                                    visible: !calls.inConference
                                    text: calls.remoteLabel
                                    font.pixelSize: phone.page.cardNameSize
                                    font.weight: Font.Medium
                                    elide: Text.ElideRight
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                RowLayout {
                                    Layout.alignment: Qt.AlignHCenter
                                    visible: calls.inConference
                                    spacing: 8

                                    Label {
                                        text: qsTr("Conference")
                                        font.pixelSize: phone.page.cardNameSize
                                        font.weight: Font.Medium
                                    }

                                    RecordingMark {
                                        visible: calls.recordingShown
                                        Layout.alignment: Qt.AlignVCenter
                                    }

                                    Label {
                                        visible: calls.recordingShown && calls.recordingPaused
                                        text: qsTr("Paused")
                                        color: Ink.bad(Material.theme)
                                        opacity: 0.85
                                    }
                                }

                                Label {
                                    Layout.fillWidth: true
                                    visible: accounts.severalLines && text.length > 0
                                    text: phone.page.lineNote(calls.accountId)
                                    opacity: 0.7
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.topMargin: 4
                                    Layout.bottomMargin: 4
                                    visible: calls.askingToRecord
                                    spacing: 6

                                    Label {
                                        Layout.fillWidth: true
                                        text: qsTr("Record this call?")
                                        font.weight: Font.Medium
                                        horizontalAlignment: Text.AlignHCenter
                                        wrapMode: Text.WordWrap
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Button {
                                            Layout.fillWidth: true
                                            text: qsTr("Record")
                                            highlighted: true
                                            Material.background: phone.page.onDark(Material.Red)
                                            onClicked: calls.answerRecording(true)
                                        }

                                        Button {
                                            Layout.fillWidth: true
                                            text: qsTr("Not this one")
                                            flat: true
                                            onClicked: calls.answerRecording(false)
                                        }
                                    }
                                }

                                Label {
                                    Layout.fillWidth: true
                                    visible: phone.page.recordingRefusal.length > 0 && !calls.ended
                                    text: phone.page.recordingRefusal
                                    color: Ink.bad(Material.theme)
                                    font.pixelSize: 12
                                    wrapMode: Text.WordWrap
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                Label {
                                    Layout.fillWidth: true
                                    visible: calls.referredBy.length > 0
                                    text: qsTr("Transferred by %1").arg(calls.referredBy)
                                    opacity: 0.7
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                Label {
                                    Layout.fillWidth: true
                                    visible: !calls.ended
                                             && (calls.transfer === CallViewModel.TransferRequested
                                                 || calls.transfer === CallViewModel.TransferRinging)
                                    text: calls.transfer === CallViewModel.TransferRinging
                                          ? qsTr("Ringing %1…").arg(calls.transferTarget)
                                          : qsTr("Transferring to %1…").arg(calls.transferTarget)
                                    opacity: 0.7
                                    font.pixelSize: 12
                                    wrapMode: Text.WordWrap
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                Label {
                                    id: transferFailure

                                    Layout.fillWidth: true
                                    visible: calls.transfer === CallViewModel.TransferFailed && showing
                                    text: phone.page.transferFailureText(calls.transferStatus,
                                                                   calls.transferTarget)

                                    color: calls.transferUnanswered ? Ink.bad(Material.theme)
                                                                    : Ink.warn(Material.theme)
                                    opacity: 0.9
                                    font.pixelSize: 12
                                    wrapMode: Text.WordWrap
                                    horizontalAlignment: Text.AlignHCenter

                                    property bool showing: true

                                    onTextChanged: showing = true

                                    Timer {
                                        running: transferFailure.visible
                                        interval: 8000
                                        onTriggered: transferFailure.showing = false
                                    }
                                }

                                Label {
                                    Layout.fillWidth: true
                                    visible: calls.remoteDetail.length > 0 && !calls.inConference
                                    text: calls.remoteDetail
                                    opacity: 0.7
                                    elide: Text.ElideRight
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.topMargin: 2
                                    visible: sound.showing && sound.drawing === SoundViewModel.Levels
                                    spacing: 8

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Glyph {
                                            Layout.preferredWidth: 16
                                            Layout.preferredHeight: 16
                                            source: phone.page.glyph("speaker")
                                            opacity: 0.6
                                        }

                                        SignalDrawing {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 10
                                            feed: sound
                                            shape: SignalDrawing.Bar
                                            side: SignalDrawing.Remote
                                            localColour: phone.page.nearColour
                                            remoteColour: phone.page.farColour
                                            faint: phone.page.faintColour
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Glyph {
                                            Layout.preferredWidth: 16
                                            Layout.preferredHeight: 16
                                            source: phone.page.glyph(calls.muted ? "mic-off" : "mic")
                                            opacity: 0.6
                                        }

                                        SignalDrawing {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 10
                                            feed: sound
                                            shape: SignalDrawing.Bar
                                            side: SignalDrawing.Local
                                            localColour: phone.page.nearColour
                                            remoteColour: phone.page.farColour
                                            faint: phone.page.faintColour
                                        }
                                    }
                                }

                                Item {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: sound.height
                                    visible: sound.showing && sound.height > 0
                                             && sound.drawing !== SoundViewModel.Levels

                                    SignalDrawing {
                                        anchors.fill: parent
                                        feed: sound
                                        shape: phone.page.shapeOf(sound.drawing)
                                        initials: phone.page.haloMark()
                                        localColour: phone.page.nearColour
                                        remoteColour: phone.page.farColour
                                        faint: phone.page.faintColour
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    visible: calls.inConference
                                    spacing: 2

                                    Repeater {
                                        model: calls.conferenceList

                                        RowLayout {
                                            id: participant

                                            required property var modelData

                                            Layout.fillWidth: true
                                            spacing: 8

                                            Label {
                                                Layout.fillWidth: true

                                                text: participant.modelData.number.length > 0
                                                      ? qsTr("%1 (%2)").arg(participant.modelData.who)
                                                                       .arg(participant.modelData.number)
                                                      : participant.modelData.who
                                                elide: Text.ElideRight
                                            }

                                            Label {
                                                visible: participant.modelData.heldByThem
                                                text: qsTr("has you on hold")
                                                color: Ink.warn(Material.theme)
                                                opacity: 0.9
                                                font.pixelSize: 12
                                            }

                                            Label {
                                                visible: participant.modelData.connectedAtMs > 0
                                                text: phone.page.duration(participant.modelData.connectedAtMs, 0)
                                                opacity: 0.7
                                                font.pixelSize: 12
                                            }

                                            Button {
                                                Layout.preferredHeight: 28
                                                topPadding: 0
                                                bottomPadding: 0
                                                flat: true
                                                focusPolicy: Qt.NoFocus
                                                font.pixelSize: 12
                                                text: qsTr("Hang up")
                                                onClicked: calls.hangUpCall(participant.modelData.callId)
                                            }
                                        }
                                    }
                                }
                            }

                            SignalDrawing {
                                Layout.preferredWidth: 7
                                Layout.fillHeight: true
                                visible: sound.showing && sound.flanking
                                feed: sound
                                shape: SignalDrawing.Upright
                                side: SignalDrawing.Local
                                localColour: phone.page.nearColour
                                remoteColour: phone.page.farColour
                                faint: phone.page.faintColour
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            visible: !calls.ended || calls.canRedial || calls.redialling

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                visible: !calls.ended

                                RowLayout {
                                    Layout.fillWidth: true

                                    visible: !calls.ended
                                    spacing: 6

                                    CardButton {

                                        visible: calls.talking && calls.canRecord
                                        enabled: calls.canStartRecording
                                        glyph: !calls.recording ? "record"
                                                                : (calls.recordingPaused
                                                                   ? "record" : "record-pause")
                                        text: !calls.recording ? qsTr("Record")
                                                               : (calls.recordingPaused
                                                                  ? qsTr("Resume recording")
                                                                  : qsTr("Pause recording"))
                                        onClicked: calls.toggleRecording()
                                    }

                                    CardButton {
                                        visible: calls.incoming
                                        highlighted: true
                                        Material.background: phone.page.onDark(Material.Green)
                                        glyph: "call"
                                        text: qsTr("Answer")
                                        onClicked: calls.answer()
                                    }

                                    CardButton {

                                        visible: calls.incoming
                                        highlighted: true
                                        Material.background: phone.page.onDark(Material.Red)
                                        glyph: "call-end"
                                        text: qsTr("Reject")
                                        onClicked: calls.reject()
                                    }

                                    CardButton {

                                        visible: calls.talking
                                        enabled: !calls.held
                                        glyph: calls.muted ? "mic-off" : "mic"
                                        text: calls.muted ? qsTr("Unmute") : qsTr("Mute")
                                        onClicked: calls.toggleMute()
                                    }

                                    CardButton {

                                        visible: calls.talking && !calls.inConference
                                        glyph: calls.held ? "resume" : "hold"
                                        text: calls.held ? qsTr("Resume") : qsTr("Hold")
                                        onClicked: calls.toggleHold()
                                    }

                                    CardButton {

                                        visible: !calls.incoming && !calls.ended
                                        Material.background: phone.page.onDark(Material.Red)
                                        highlighted: true
                                        glyph: "call-end"

                                        text: calls.inConference ? qsTr("Hang up all") : qsTr("Hang up")
                                        onClicked: {
                                            if (calls.inConference) {
                                                calls.hangUpConference()
                                            } else {
                                                calls.hangUp()
                                            }
                                        }
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true

                                    visible: !calls.ended && (calls.canTransfer || calls.canMerge
                                                              || calls.inConference || phone.page.keysOffered)
                                    spacing: 6

                                    CardButton {
                                        visible: calls.canTransfer
                                        glyph: "transfer"
                                        text: qsTr("Transfer…")
                                        onClicked: phone.page.askWhereToTransfer()
                                    }

                                    CardButton {
                                        visible: calls.canMerge
                                        glyph: "merge"
                                        text: calls.mergePartner.length > 0
                                              ? qsTr("Merge with %1").arg(calls.mergePartner)
                                              : qsTr("Merge all")
                                        onClicked: calls.merge()
                                    }

                                    CardButton {
                                        visible: calls.inConference
                                        glyph: "split"
                                        text: qsTr("Split")
                                        onClicked: calls.split()
                                    }

                                    CardButton {
                                        id: keypadDisclosure

                                        visible: phone.page.keysOffered
                                        enabled: phone.page.keysReachable

                                        highlighted: phone.page.keypadShown && phone.page.keysReachable

                                        Material.accent: phone.page.onDark(Material.Teal)
                                        glyph: "dialpad"
                                        text: phone.page.keypadShown ? qsTr("Hide keypad") : qsTr("Keypad")
                                        hintText: phone.page.keysReachable
                                                  ? keypadDisclosure.text
                                                  : qsTr("Not while the call is on hold.")
                                        onClicked: phone.page.keypadShown = !phone.page.keypadShown
                                    }
                                }

                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6
                                visible: calls.canRedial || calls.redialling

                                Label {
                                    Layout.fillWidth: true
                                    visible: calls.redialling
                                    text: phone.page.redialProgress()
                                    wrapMode: Text.WordWrap
                                    horizontalAlignment: Text.AlignHCenter
                                    opacity: 0.8
                                    font.pixelSize: 13
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    CardButton {
                                        visible: !calls.redialling
                                        glyph: "redial"
                                        text: qsTr("Keep trying")
                                        onClicked: calls.startRedial()
                                    }

                                    CardButton {
                                        visible: calls.redialling
                                        glyph: "call-dismiss"
                                        text: qsTr("Stop trying")
                                        onClicked: calls.cancelRedial()
                                    }
                                }
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                visible: phone.page.keysReachable && phone.page.keypadShown
                                columns: 3
                                columnSpacing: 6
                                rowSpacing: 6

                                Repeater {
                                    model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "*", "0", "#"]

                                    Button {
                                        required property string modelData
                                        Layout.fillWidth: true

                                        Layout.preferredHeight: phone.page.cardButtonHeight

                                        topInset: 0
                                        bottomInset: 0

                                        topPadding: 0
                                        bottomPadding: 0

                                        enabled: calls.canSendDtmf

                                        focusPolicy: Qt.NoFocus
                                        text: modelData
                                        onClicked: calls.sendDtmf(modelData)
                                    }
                                }
                            }

                            Label {
                                Layout.fillWidth: true

                                visible: phone.page.keysReachable && phone.page.keypadShown
                                         && !calls.canSendDtmf

                                text: qsTr("Keys go nowhere while they hold you.")
                                wrapMode: Text.WordWrap
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: 12
                                opacity: 0.7
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: calls.muted
                            text: qsTr("Your microphone is off.")
                            horizontalAlignment: Text.AlignHCenter
                            color: Ink.warn(Material.theme)
                            font.pixelSize: 12
                        }
                    }
                }

                Frame {
                    Layout.leftMargin: phone.pageAir
                    Layout.rightMargin: phone.pageAir
                    Layout.fillWidth: true
                    visible: calls.offeringWhileBusy

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 12

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Incoming call")
                            opacity: 0.7
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Label {
                            Layout.fillWidth: true
                            text: calls.offeredLabel
                            font.pixelSize: phone.page.cardNameSize
                            font.weight: Font.Medium
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: calls.offeredDetail.length > 0
                            text: calls.offeredDetail
                            opacity: 0.7
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignHCenter
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            CardButton {
                                highlighted: true
                                Material.background: phone.page.onDark(Material.Green)
                                glyph: "call-pause"
                                text: qsTr("Hold and answer")
                                onClicked: calls.answer()
                            }

                            CardButton {
                                highlighted: true
                                Material.background: phone.page.onDark(Material.Amber)
                                glyph: "call-swap"
                                text: qsTr("End and answer")
                                onClicked: calls.answerEndingOthers()
                            }

                            CardButton {
                                highlighted: true
                                Material.background: phone.page.onDark(Material.Red)
                                glyph: "call-dismiss"
                                text: qsTr("Decline")
                                onClicked: calls.reject()
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("On a call with %1").arg(calls.remoteLabel)
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignHCenter
                            opacity: 0.7
                            font.pixelSize: 12
                        }
                    }
                }

                ListView {
                    id: callList

                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(count, 3) * 52

                    visible: calls.moreThanTheCard
                    model: calls.callList
                    clip: true
                    interactive: contentHeight > height

                    delegate: ItemDelegate {
                        id: callRow
                        required property var modelData

                        width: callList.width
                        height: 52
                        hoverEnabled: true

                        leftPadding: phone.rowAir
                        rightPadding: phone.rowAir

                        highlighted: callRow.modelData.current
                        onClicked: calls.select(callRow.modelData.callId)

                        readonly property bool showsControls:
                            callRow.hovered && callRow.modelData.state !== CallViewModel.Ended

                        contentItem: RowLayout {

                            opacity: callRow.modelData.state === CallViewModel.Ended ? 0.5 : 1.0
                            spacing: 8

                            Item {
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16
                                Layout.alignment: Qt.AlignVCenter

                                Glyph {
                                    anchors.fill: parent
                                    visible: callRow.modelData.live || callRow.modelData.held
                                    source: phone.page.glyph(callRow.modelData.live
                                                       ? "speaker" : "hold")
                                    opacity: callRow.modelData.live ? 1.0 : 0.7
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: 8
                                Layout.preferredHeight: 8
                                Layout.alignment: Qt.AlignVCenter
                                visible: callRow.modelData.recording === true
                                radius: 4
                                color: callRow.modelData.live
                                       ? Mark.bad : "transparent"
                                border.width: callRow.modelData.live ? 0 : 2
                                border.color: Mark.bad
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Label {
                                    Layout.fillWidth: true
                                    text: callRow.modelData.number.length > 0
                                          ? qsTr("%1 (%2)").arg(callRow.modelData.who)
                                                           .arg(callRow.modelData.number)
                                          : callRow.modelData.who
                                    font.weight: callRow.modelData.current ? Font.Medium
                                                                           : Font.Normal
                                    elide: Text.ElideRight
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: phone.page.rowStateText(callRow.modelData)
                                    color: callRow.modelData.heldByThem
                                           ? Ink.warn(Material.theme)
                                           : Material.foreground
                                    opacity: 0.7
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                            }

                            Label {
                                Layout.leftMargin: 8
                                visible: callRow.modelData.connectedAtMs > 0
                                         && !callRow.showsControls
                                text: phone.page.duration(callRow.modelData.connectedAtMs,
                                                    callRow.modelData.endedAtMs)
                                opacity: 0.7
                                font.pixelSize: 12
                            }

                            Row {
                                Layout.alignment: Qt.AlignVCenter

                                Layout.preferredHeight: 36
                                spacing: 0
                                visible: callRow.showsControls

                                CallRowButton {

                                    visible: callRow.modelData.state === CallViewModel.Active
                                             && !callRow.modelData.inConference
                                    glyph: callRow.modelData.held ? "resume" : "hold"
                                    text: callRow.modelData.held ? qsTr("Resume") : qsTr("Hold")
                                    onClicked: calls.toggleHoldCall(callRow.modelData.callId)
                                }

                                CallRowButton {

                                    readonly property bool ringing:
                                        callRow.modelData.state === CallViewModel.RingingIn

                                    glyph: "call-end"
                                    text: ringing ? qsTr("Reject") : qsTr("Hang up")
                                    onClicked: {
                                        if (ringing) {
                                            calls.rejectCall(callRow.modelData.callId)
                                        } else {
                                            calls.hangUpCall(callRow.modelData.callId)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.leftMargin: phone.pageAir
                    Layout.rightMargin: phone.pageAir
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    spacing: 8
                    visible: phone.page.mainView === phone.page.viewCalls
                             && calls.moreThanTheCard

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        implicitHeight: 1
                        color: Material.foreground
                        opacity: 0.15
                    }

                    Label {

                        text: qsTr("History")
                        font.pixelSize: 11
                        opacity: 0.6
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        implicitHeight: 1
                        color: Material.foreground
                        opacity: 0.15
                    }
                }

                ListView {
                    id: recentCalls

                    readonly property int rowHeight: 44

                    Layout.fillWidth: true
                    Layout.preferredHeight: count * recentCalls.rowHeight
                    visible: phone.page.mainView === phone.page.viewCalls
                    clip: true

                    readonly property bool newestInView: {
                        void scroller.contentItem.contentY
                        void scroller.height
                        void recentCalls.y

                        if (!recentCalls.visible || !phone.page.onScreen
                            || recentCalls.count === 0) {
                            return false
                        }
                        const at = recentCalls.mapToItem(scroller, 0, 0)
                        return at.y + recentCalls.rowHeight > 0 && at.y < scroller.height
                    }

                    onNewestInViewChanged: history.watching(recentCalls, newestInView)
                    Component.onCompleted: history.watching(recentCalls, newestInView)

                    interactive: false

                    model: history.newestCount

                    delegate: ItemDelegate {
                        id: recentRow

                        required property int index

                        readonly property var modelData: {
                            void history.newestCount;
                            return history.newestAt(recentRow.index);
                        }

                        width: recentCalls.width
                        height: recentCalls.rowHeight

                        leftPadding: phone.rowAir
                        rightPadding: phone.rowAir

                        RowMenu {
                            number: recentRow.modelData.number
                            extra: [{

                                label: qsTr("Call"),

                                picture: settings.glyphSet + "/call.svg",
                                act: () => history.redial(recentRow.modelData.id)
                            }]
                        }

                        TapHandler {
                            acceptedButtons: Qt.LeftButton
                            onDoubleTapped: history.redial(recentRow.modelData.id)
                        }

                        contentItem: RowLayout {
                            spacing: 8

                            Glyph {
                                Layout.alignment: Qt.AlignVCenter
                                Layout.preferredWidth: 16
                                Layout.preferredHeight: 16

                                source: phone.page.glyph(
                                            CallGlyph.forRow(recentRow.modelData))

                                color: CallInk.forMark(recentRow.modelData.mark)
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Label {
                                    Layout.fillWidth: true

                                    text: {
                                        void Qt.uiLanguage

                                        const who = recentRow.modelData.name !== ""
                                                    ? recentRow.modelData.name
                                                    : recentRow.modelData.number

                                        return recentRow.modelData.times > 1
                                               ? qsTr("%1 (%2)").arg(who)
                                                                .arg(recentRow.modelData.times)
                                               : who
                                    }
                                    elide: Text.ElideRight
                                    font.pixelSize: 13
                                }

                                Label {
                                    Layout.fillWidth: true
                                    visible: recentRow.modelData.name !== ""
                                             && recentRow.modelData.name
                                                !== recentRow.modelData.number
                                    text: recentRow.modelData.number
                                    elide: Text.ElideRight
                                    font.pixelSize: 11
                                    opacity: 0.6
                                }
                            }

                            Label {
                                Layout.alignment: Qt.AlignVCenter
                                text: phone.page.callStamp(recentRow.modelData.startedAtMs)
                                font.pixelSize: 11
                                opacity: 0.6
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true

            Layout.leftMargin: phone.pageEdge + phone.pageAir
            Layout.rightMargin: phone.pageEdge + phone.pageAir
            spacing: 6

            Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: phone.page.dialRefusal
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Ink.bad(Material.theme)
            }

            Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: phone.page.spotlightSentence()
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Ink.bad(Material.theme)
            }

            Label {
                Layout.fillWidth: true
                visible: accounts.spotlightRetrying
                text: qsTr("Trying again shortly.")
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.7
            }

            Label {
                Layout.fillWidth: true
                visible: settings.firstRun && !accounts.connected
                text: qsTr("This phone is not set up yet. Open Settings to add your account.")
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.7
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Material.foreground
                opacity: 0.15
            }

            RowLayout {
                id: lineStrip

                Layout.fillWidth: true

                readonly property real needed: Math.max(28, chipArea.wanted)

                Layout.preferredHeight: lineStrip.needed
                Layout.minimumHeight: lineStrip.needed
                Layout.maximumHeight: lineStrip.needed
                spacing: 10

                Item {
                    Layout.preferredWidth:
                        phone.page.chipsCentred
                        ? dndChip.width + (waysInChip.visible
                                           ? waysInChip.width + lineStrip.spacing : 0)
                        : 0
                    Layout.preferredHeight: 1
                }

                Item {
                    visible: !phone.severalChips
                    Layout.fillWidth: true
                }

                Rectangle {
                    id: statusDot

                    visible: !phone.severalChips
                    Layout.preferredWidth: 10
                    Layout.preferredHeight: 10
                    Layout.alignment: Qt.AlignVCenter
                    radius: 5
                    color: phone.page.statusColor(accounts.spotlightState)

                    SequentialAnimation on opacity {
                        running: accounts.busy
                        loops: Animation.Infinite
                        onRunningChanged: {
                            if (!running) {
                                statusDot.opacity = 1
                            }
                        }
                        NumberAnimation { to: 0.3; duration: 600 }
                        NumberAnimation { to: 1.0; duration: 600 }
                    }
                }

                Label {
                    visible: !phone.severalChips
                    Layout.alignment: Qt.AlignVCenter
                    text: phone.page.statusText(accounts.spotlightState)
                    font.pixelSize: 13
                    opacity: 0.8
                }

                Item {
                    visible: !phone.severalChips
                    Layout.fillWidth: true
                }

                Item {
                    id: chipArea

                    visible: phone.severalChips
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    clip: true

                    readonly property real furniture: 8 + 6 + 12 + 12

                    readonly property real rowHeight: 24
                    readonly property real rowSpacing: 4
                    readonly property real chipSpacing: 6

                    property var places: []
                    property int rows: 0

                    readonly property real wanted:
                        chipArea.rows === 0
                        ? 0
                        : chipArea.rows * chipArea.rowHeight
                          + (chipArea.rows - 1) * chipArea.rowSpacing

                    function measure() {

                        if (chipArea.width <= 0) {
                            chipArea.places = []
                            chipArea.rows = 0
                            return
                        }

                        const widths = []
                        for (let index = 0; index < chips.count; ++index) {
                            const chip = chips.itemAt(index)
                            widths.push(chip !== null && chip.visible
                                        ? chip.naturalNameWidth + chipArea.furniture
                                        : 0)
                        }

                        if (phone.page.oneChipRow && widths.length > 0) {

                            const drawn = []
                            for (let at = 0; at < widths.length; ++at) {
                                if (widths[at] > 0) {
                                    drawn.push(at)
                                }
                            }
                            const room = chipArea.width
                                         - chipArea.chipSpacing * Math.max(0, drawn.length - 1)
                            const each = drawn.length === 0
                                         ? 0 : Math.max(0, Math.floor(room / drawn.length))
                            const places = []
                            let where = 0
                            for (let at = 0; at < widths.length; ++at) {
                                if (widths[at] === 0) {
                                    places.push({ x: 0, y: 0, width: 0 })
                                    continue
                                }
                                places.push({ x: where, y: 0, width: each })
                                where += each + chipArea.chipSpacing
                            }
                            chipArea.places = places
                            chipArea.rows = 1
                            return
                        }

                        const laid = accounts.chipPlaces(widths, chipArea.width,
                                                         chipArea.chipSpacing,
                                                         chipArea.rowHeight, chipArea.rowSpacing)
                        chipArea.places = laid.places
                        chipArea.rows = laid.rows
                    }

                    Component.onCompleted: chipArea.measure()
                    onWidthChanged: Qt.callLater(chipArea.measure)

                    Connections {
                        target: accounts

                        function onAccountsChanged() {

                            Qt.callLater(chipArea.measure)
                        }
                    }

                    Repeater {
                        id: chips

                        model: phone.severalChips ? accounts.count : 0

                        delegate: AbstractButton {
                            id: lineChip

                            required property int index

                            readonly property var modelData: {
                                void accounts.count;
                                return accounts.accountAt(lineChip.index);
                            }

                            readonly property real naturalNameWidth:
                                Math.ceil(Math.max(nameMetrics.width,
                                                   nameMetrics.advanceWidth)) + 1

                            readonly property var place:
                                lineChip.index < chipArea.places.length
                                ? chipArea.places[lineChip.index]
                                : null

                            readonly property bool current:
                                phone.page.lineInTheCard.length > 0
                                ? lineChip.modelData.id === phone.page.lineInTheCard
                                : lineChip.modelData.dialsFrom

                            visible: lineChip.modelData.enabled
                            hoverEnabled: true
                            implicitHeight: chipArea.rowHeight

                            x: lineChip.place ? lineChip.place.x : 0
                            y: (lineChip.place ? lineChip.place.y : 0)
                               + Math.max(0, (chipArea.height - chipArea.wanted) / 2)

                            onNaturalNameWidthChanged: Qt.callLater(chipArea.measure)
                            onVisibleChanged: Qt.callLater(chipArea.measure)

                            TextMetrics {
                                id: nameMetrics

                                font.family: chipName.font.family
                                font.pixelSize: chipName.font.pixelSize
                                font.bold: true
                                text: lineChip.modelData.shortLabel
                            }

                            leftPadding: 12
                            rightPadding: 12
                            topPadding: 0
                            bottomPadding: 0
                            implicitWidth: chipRow.implicitWidth + leftPadding + rightPadding
                            onClicked: accounts.chooseDialAccount(lineChip.modelData.id)
                            Accessible.name: qsTr("Call from %1").arg(lineChip.modelData.shortLabel)

                            Hint {
                                owner: lineChip
                                visible: lineChip.hovered
                                text: lineChip.modelData.shortLabel
                            }

                            background: Rectangle {
                                radius: 12
                                color: lineChip.current
                                       ? Qt.rgba(Material.accentColor.r, Material.accentColor.g,
                                                 Material.accentColor.b, 0.22)
                                       : (lineChip.hovered
                                          ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                                                    Material.foreground.b, 0.10)
                                          : "transparent")
                                border.width: lineChip.modelData.dialsFrom ? 1 : 0
                                border.color: Qt.rgba(Material.accentColor.r,
                                                      Material.accentColor.g,
                                                      Material.accentColor.b, 0.55)
                            }

                            contentItem: Row {
                                id: chipRow

                                spacing: 6

                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: phone.page.lineColor(lineChip.modelData)
                                }

                                Label {
                                    id: chipName

                                    anchors.verticalCenter: parent.verticalCenter

                                    width: Math.min(
                                        lineChip.naturalNameWidth,
                                        Math.max(0, (lineChip.place && phone.page.oneChipRow
                                                     ? lineChip.place.width
                                                     : chipArea.width)
                                                    - chipArea.furniture))
                                    elide: Text.ElideRight
                                    text: lineChip.modelData.shortLabel
                                    font.pixelSize: 12
                                    font.bold: lineChip.current
                                    opacity: 0.9
                                }
                            }
                        }
                    }

                }

                ToolButton {
                    id: dndChip

                    Layout.alignment: Qt.AlignVCenter
                    implicitHeight: 24
                    topInset: 0
                    bottomInset: 0

                    implicitWidth: 24
                    focusPolicy: Qt.NoFocus
                    hoverEnabled: true
                    padding: 0

                    contentItem: Item {
                        Glyph {
                            anchors.centerIn: parent
                            width: 14
                            height: 14
                            source: settings.glyphSet + "/call-prohibited.svg"
                            color: calls.doNotDisturb ? "white" : Mark.bad
                        }
                    }

                    Accessible.name: qsTr("Do not disturb")

                    background: Rectangle {
                        radius: height / 2
                        color: calls.doNotDisturb
                               ? Mark.bad
                               : dndChip.hovered
                                 ? Qt.rgba(Mark.bad.r,
                                           Mark.bad.g,
                                           Mark.bad.b, 0.12)
                                 : "transparent"
                        border.width: calls.doNotDisturb ? 0 : 1
                        border.color: Qt.rgba(Mark.bad.r,
                                              Mark.bad.g,
                                              Mark.bad.b, 0.5)
                    }

                    Hint {
                        owner: dndChip
                        visible: dndChip.hovered
                        text: calls.doNotDisturb
                              ? qsTr("Do not disturb is on — every incoming call is refused. Click to turn it off.")
                              : qsTr("Do not disturb: refuse every incoming call")
                    }

                    onClicked: calls.toggleDoNotDisturb()
                }

                ToolButton {
                    id: waysInChip

                    visible: phone.page.waysInAsMenu

                    Layout.alignment: Qt.AlignVCenter
                    implicitHeight: 24
                    implicitWidth: 24
                    topInset: 0
                    bottomInset: 0
                    padding: 0
                    focusPolicy: Qt.NoFocus
                    hoverEnabled: true

                    icon.source: settings.glyphSet + "/more-vertical.svg"
                    icon.width: 16
                    icon.height: 16

                    Accessible.name: qsTr("More")

                    onClicked: phone.page.showTheWaysIn(waysInChip)

                    WaitingDot {
                        shown: phone.page.waysInMarked
                    }

                    background: Rectangle {
                        radius: height / 2
                        color: waysInChip.down
                               ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                                         Material.foreground.b, 0.16)
                               : waysInChip.hovered
                                 ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                                           Material.foreground.b, 0.08)
                                 : "transparent"
                    }
                }

            }

        }
    }
