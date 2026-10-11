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
    id: dashboard

    required property var page

    Layout.fillWidth: true
    spacing: 12

    readonly property int coat: 16

    property var opened: []

    function isOpen(code) {
        return dashboard.opened.indexOf(code) >= 0
    }

    function toggle(code) {
        const next = dashboard.opened.slice()
        const at = next.indexOf(code)
        if (at >= 0) {
            next.splice(at, 1)
        } else {
            next.push(code)
        }
        dashboard.opened = next
    }

    Overview {
        id: overview
    }

    function hasSection(id) {
        return overview.hasSection(id)
    }

    function hasWindow(id) {
        return overview.hasWindow(id)
    }

    function lit(code) {
        return overview.lit(code)
    }

    function moduleState(id) {
        return overview.moduleState(id)
    }

    function moduleOff(id) {
        return overview.moduleOff(id)
    }

    function moduleName(id) {
        return overview.moduleName(id)
    }

    function offerOf(id, module) {
        return overview.offerOf(id, module)
    }

    readonly property var plan: overview.plan

    function groupTitle(id) {
        void Qt.uiLanguage

        switch (id) {
        case "calls":
            return qsTr("Calls")
        case "capture":
            return qsTr("Capturing other applications")
        case "transcription":
            return qsTr("Transcription")
        case "processing":
            return qsTr("Processing")

        case "prompter":
            return dashboard.moduleName("realtime")
        case "library":
            return qsTr("Recordings")
        case "application":
            return qsTr("Application")
        case "feedback":
            return qsTr("Feedback")
        }
        return ""
    }

    function titleFor(code) {
        void Qt.uiLanguage

        switch (code) {
        case "calls.account":
            return qsTr("Add an account")
        case "calls.outgoing":
            return qsTr("Make a call")
        case "calls.incoming":
            return qsTr("Answer a call")
        case "calls.recorded":
            return qsTr("Record a call")
        case "capture.on":
            return qsTr("Turn capturing on")
        case "capture.first":
            return qsTr("Capture a conversation")
        case "capture.named":
            return qsTr("Give one a name")
        case "transcription.provider":
            return qsTr("Add a recogniser")
        case "transcription.first":
            return qsTr("Transcribe a conversation")
        case "prompter.allowed":
            return qsTr("Allow prompting")
        case "prompter.first":
            return qsTr("Start prompting")
        case "processing.provider":
            return qsTr("Add a model")
        case "processing.first":
            return qsTr("Process a transcript")
        case "library.search":
            return qsTr("Search what was said")
        case "library.export.recording":
            return qsTr("Export a recording")
        case "library.export.summary":
            return qsTr("Export a summary")
        case "application.autostart":
            return qsTr("Start with the computer")
        case "application.updates":
            return qsTr("Keep it up to date")
        case "feedback.reporting":
            return qsTr("Send the extended report")
        case "feedback.sent":
            return qsTr("Tell us what you think")
        }
        return ""
    }

    function whyFor(code) {
        void Qt.uiLanguage

        switch (code) {
        case "calls.account":
            return qsTr("An account is the SIP line your provider or your company issued: a server, a user name and a password. Until one is registered the phone can neither place nor take calls, and every step below waits on it. Settings, under Accounts.")
        case "calls.outgoing":
            return qsTr("One outgoing call confirms three things at once: that the account is registered, that the microphone in use is the one you meant, and that the other side can hear you.")
        case "calls.incoming":
            return qsTr("An incoming call is offered on a card that names the caller. When the window is not in front of you, the same offer arrives as a notification with an Answer button on it.")
        case "calls.recorded":
            return qsTr("A recording is what the rest of this list works on: it is transcribed, summarised, tagged and searched. Settings, under Recording.")
        case "capture.on":
            return qsTr("Conversations held in Zoom, Teams or a browser are not this phone's calls, and it can record them all the same: what the computer plays on one channel, your microphone on the other. Nothing else on that page has any effect until this is on. Settings, under Capture.")
        case "capture.first":
            return qsTr("A captured conversation is stored, transcribed and processed exactly as a call of this phone's own is. The sound is poorer, because it is assembled from two devices of the operating system's rather than taken from inside a call.")
        case "capture.named":
            return qsTr("A captured conversation is filed under the application it was heard in, which says nothing about what it was about. The name is what the list and the search show later.")
        case "transcription.provider":
            return qsTr("A recogniser turns recorded speech into text. It can run on this machine or inside your own network, which is what keeps the audio where it is, or it can be a service elsewhere. Settings, under Transcription.")
        case "transcription.first":
            return qsTr("The transcript is what the search box reads and what a language model is given. Without one a recording can only be played.")
        case "prompter.allowed":
            return qsTr("One tick decides whether a prompter can be started at all, and nothing else on that page has any effect until it is on. It is a tick of its own because a prompter sends what the other party says to a recogniser as it is spoken, and is charged by the minute. Settings, under Prompter.")
        case "prompter.first":
            return qsTr("A prompter listens to a call while it is happening and writes down what is said, either side in its own column. Where the prompt asks a model something, the answer arrives beside the words — which is what makes it worth having open during an interview or a difficult conversation.")
        case "processing.provider":
            return qsTr("A language model reads a transcript and writes what came of the conversation: a summary, the action items, the subjects it covered and anything worth a second look. Settings, under Processing.")
        case "processing.first":
            return qsTr("What a model writes is kept beside the recording and can be read, copied and exported. It is also what makes a long conversation worth opening a year later.")
        case "library.search":
            return qsTr("The box above the list reads the transcripts as well as the names and the numbers, so a word somebody said in March is enough to find the call it was said in.")
        case "library.export.recording":
            return qsTr("A conversation can be saved out as an audio file, to send on or to keep somewhere this phone does not manage. The copy in the list stays where it is and the retention rules still apply to it.")
        case "library.export.summary":
            return qsTr("What a model wrote can be saved as a file of its own. That is usually what is actually sent on, rather than the recording.")
        case "application.autostart":
            return qsTr("A phone that is not running does not ring. Autostart runs it when you log in, and it can start without a window, in the notification area. Settings, under Startup.")
        case "application.updates":
            return qsTr("The phone checks for a new version once a day and reports one when there is one. Nothing is downloaded or installed until you say so. Settings, under About.")
        case "feedback.reporting":
            return qsTr("The extended report adds how many calls and captures this phone has handled and which kind of switch it registers against. It carries no number, no name and nothing that was said. It is what tells us which systems to keep working. Settings, under About.")
        case "feedback.sent":
            return qsTr("What is wrong, what is missing and what got in your way reaches the people who build the phone. It is the shortest route from a fault to a fix.")
        }
        return ""
    }

    function howFor(code) {
        void Qt.uiLanguage

        switch (code) {
        case "calls.account":
            return qsTr("Press Add at the top of that page and fill in the server, the user name and the password. The dot beside the line turns green once it has registered; where it does not, the line carries what the server answered.")
        case "calls.outgoing":
            return qsTr("Type or paste a number into the field at the top of the phone — full international form is the safest — and press the green key. The card that replaces the keypad is where the call is held, muted, transferred and ended.")
        case "calls.incoming":
            return qsTr("There is nothing to set up for this one. What is worth setting is how the phone announces a call — a notification, the window brought to the front, or neither — on the Appearance page, under Incoming calls.")
        case "calls.recorded":
            return qsTr("Press Record on the card of a call in progress. The first control on that page decides what happens without being asked — every call, a question at each one, or only the lines you pick — and the rest of it is what the other side is told before a recording starts.")
        case "capture.on":
            return qsTr("Tick Allow sound capture. Under it: whether a capture starts on its own, how many seconds from before the start are kept, and which applications are never recorded.")
        case "capture.first":
            return qsTr("Start a meeting in another application. Capture can begin on its own, or wait for Record on the strip under the dialler. The strip draws both channels while it works, so each side arriving is something you can see.")
        case "capture.named":
            return qsTr("Open the recordings window, find the conversation and press Rename on its row. What you type replaces the application's name rather than standing beside it.")
        case "transcription.provider":
            return qsTr("Add one on that page: its type, its address and, for a service, a key. Test reports whether it answers before you rely on it. The language spoken on calls is a setting of its own, separate from the language of the interface.")
        case "transcription.first":
            return qsTr("Open a conversation in the recordings window and press Transcribe, or have it done to every recording as it arrives. It takes minutes rather than seconds, and the strip above the list reports what is in progress.")
        case "prompter.allowed":
            return qsTr("Tick Allow the prompter to be used. Under it: which recogniser listens, which model is asked, and what either of them may spend in a month.")
        case "prompter.first":
            return qsTr("Press Start in the prompter window while a call is up. It is dark until there is a conversation, a recogniser that can listen and a prompt that says what to do with what it hears, and the window says which of those is missing.")
        case "processing.provider":
            return qsTr("Add a model the way you added a recogniser, and set a ceiling on what it may spend. What it is asked is a prompt, and the prompts this phone ships with are on the same page.")
        case "processing.first":
            return qsTr("Open a conversation that has a transcript, press the AI button above it and choose a prompt — or have it happen as each transcript is finished. The answers stand in the same list as the transcript.")
        case "library.search":
            return qsTr("Type into the box above the list. The controls beside it narrow what is left — by type, by period, by category and by mark — and they work together with the search rather than instead of it.")
        case "library.export.recording":
            return qsTr("Open a conversation and press Save a copy on the player, then choose where the file goes.")
        case "library.export.summary":
            return qsTr("Open the answer you want beside the transcript and press Save as. What is written out is the text exactly as it stands on the screen.")
        case "application.autostart":
            return qsTr("Tick Start the phone when I log in, and the second box as well if it should come up without a window. Nothing else about the computer is changed.")
        case "application.updates":
            return qsTr("The tick is under Updates on the About page, with the version beside it and a button that looks now.")
        case "feedback.reporting":
            return qsTr("The three levels are on that page, with the full list of what each one sends beside them. It can be changed at any time, in either direction.")
        case "feedback.sent":
            return qsTr("The form is at the foot of the About page: a subject, a message and, if you want an answer, an address. The version and the kind of system go with it, and the log only if you attach it.")
        }
        return ""
    }

    function turnOnTitle(module) {
        void Qt.uiLanguage

        return qsTr("Turn on %1").arg(dashboard.moduleName(module))
    }

    function turnOnWhy(module) {
        void Qt.uiLanguage

        return qsTr("%1 is switched off, so nothing in this group applies and none of it is counted above. Settings, under Modules.").arg(dashboard.moduleName(module))
    }

    function turnOnHow(module) {
        void Qt.uiLanguage
        return qsTr("Tick it on the Modules page. It takes effect at once — the phone does not have to be restarted — and the steps it brings with it appear here.")
    }

    function titleOf(row) {
        return row.module !== "" ? dashboard.turnOnTitle(row.module)
                                 : dashboard.titleFor(row.code)
    }

    function whyOf(row) {
        return row.module !== "" ? dashboard.turnOnWhy(row.module)
                                 : dashboard.whyFor(row.code)
    }

    function howOf(row) {
        return row.module !== "" ? dashboard.turnOnHow(row.module)
                                 : dashboard.howFor(row.code)
    }

    readonly property var rows: overview.rows

    function counts(row) {
        return overview.counts(row)
    }

    readonly property int stepCount: overview.stepCount

    readonly property int greenCount: overview.greenCount

    readonly property int redCount: dashboard.stepCount - dashboard.greenCount

    Label {
        Layout.fillWidth: true

        text: qsTr("Documentation")
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        id: docsLine

        Layout.fillWidth: true

        textFormat: Text.StyledText
        wrapMode: Text.WordWrap
        linkColor: dashboard.page.linkColour

        text: qsTr("Go to the site %1")
              .arg("<a href=\"" + about.documentationPage + "\">"
                   + about.documentation.replace("https://", "") + "</a>")
        onLinkActivated: link => Qt.openUrlExternally(link)

        HoverHandler {
            cursorShape: docsLine.hoveredLink ? Qt.PointingHandCursor
                                              : Qt.ArrowCursor
        }
    }

    Label {
        Layout.fillWidth: true

        text: qsTr("Overview")
        font.pixelSize: 15
        font.bold: true
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 16

        Lamp {
            lit: true
        }

        Label {

            text: qsTr("%n done", "", dashboard.greenCount)
        }

        Lamp {
            lit: false
        }

        Label {

            text: qsTr("%n to go", "", dashboard.redCount)
        }

        Item {
            Layout.fillWidth: true
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 6
        radius: 3
        color: Qt.rgba(Material.foreground.r, Material.foreground.g, Material.foreground.b, 0.12)

        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            radius: parent.radius
            width: dashboard.stepCount > 0
                   ? parent.width * (dashboard.greenCount / dashboard.stepCount) : 0

            color: Mark.good

            Behavior on width {
                NumberAnimation {
                    duration: 180
                }
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: dashboard.redCount === 0

              ? qsTr("Everything is set up. Nothing here is waiting on you.")

              : qsTr("Press a line to read what it is for and to open where it is set.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: -dashboard.coat
        Layout.rightMargin: -dashboard.coat
        spacing: 0

        Repeater {
            model: dashboard.rows

            ColumnLayout {
                id: entry

                required property var modelData

                readonly property bool open: !entry.modelData.heading
                                             && dashboard.isOpen(entry.modelData.code)

                Layout.fillWidth: true
                Layout.topMargin: entry.modelData.heading ? 10 : 0
                spacing: 0

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: dashboard.coat
                    Layout.rightMargin: dashboard.coat
                    Layout.bottomMargin: 2
                    visible: entry.modelData.heading
                    text: entry.modelData.heading ? dashboard.groupTitle(entry.modelData.code) : ""
                    font.pixelSize: 15
                    font.bold: true
                }

                ItemDelegate {
                    Layout.fillWidth: true
                    visible: !entry.modelData.heading

                    leftPadding: dashboard.coat
                    rightPadding: dashboard.coat
                    topPadding: 6
                    bottomPadding: 6

                    onClicked: dashboard.toggle(entry.modelData.code)

                    contentItem: RowLayout {
                        spacing: 12

                        Lamp {
                            shown: entry.modelData.module === ""
                            lit: entry.modelData.module === ""
                                 && dashboard.lit(entry.modelData.code)
                        }

                        Label {
                            Layout.fillWidth: true
                            text: entry.modelData.heading
                                  ? "" : dashboard.titleOf(entry.modelData)
                            wrapMode: Text.WordWrap
                        }

                        Label {
                            Layout.alignment: Qt.AlignVCenter
                            text: entry.open ? "▴" : "▾"
                            opacity: 0.5
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: dashboard.coat + 24
                    Layout.rightMargin: dashboard.coat

                    Layout.topMargin: entry.open ? 6 : 0
                    Layout.bottomMargin: entry.open ? 10 : 0
                    visible: entry.open
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            Layout.fillWidth: true
                            text: entry.modelData.heading
                                  ? "" : dashboard.whyOf(entry.modelData)
                            wrapMode: Text.WordWrap
                            font.pixelSize: 12
                            opacity: 0.8
                        }

                        Label {
                            Layout.fillWidth: true
                            text: entry.modelData.heading
                                  ? "" : dashboard.howOf(entry.modelData)
                            wrapMode: Text.WordWrap
                            font.pixelSize: 12
                            opacity: 0.8
                        }
                    }

                    Button {
                        Layout.alignment: Qt.AlignTop
                        visible: entry.modelData.section !== "" || entry.modelData.window !== ""
                                 || entry.modelData.phone

                        text: qsTr("Go there")
                        onClicked: dashboard.go(entry.modelData)
                    }
                }
            }
        }
    }

    function go(step) {
        if (step.section !== "") {
            dashboard.page.reveal(step.section, step.control)
            return
        }
        if (step.window !== "") {
            dashboard.page.windowRequested(step.window, step.control)
            return
        }
        if (step.phone) {
            dashboard.page.phoneRequested()
        }
    }

    component Lamp: Rectangle {
        required property bool lit

        property bool shown: true

        implicitWidth: 12
        implicitHeight: 12
        radius: 6
        Layout.alignment: Qt.AlignVCenter
        color: !shown
               ? "transparent"
               : (lit ? Mark.good : Mark.bad)
    }
}
