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

import Softphone

Item {
    id: shooter

    required property var phone

    required property var settingsPanel

    function theSettings() {
        return shooter.settingsPanel !== null ? shooter.settingsPanel : shooter.phone
    }

    function theWindow(id) {
        const panel = shooter.phone.panelNamed(id)
        if (panel !== null) {
            return panel
        }

        const apart = shooter.phone.workspaceWindow()
        return apart !== null ? apart : shooter.phone
    }

    readonly property string conversation: "shot-conversation"
    readonly property string reading: "shot-reading"
    readonly property string answer: "shot-answer"

    readonly property int into: 62000

    readonly property var callers: ["205", "206", "211"]

    readonly property int meetingIn: 754

    readonly property string dialled: "206"

    readonly property string line: "line-office"

    property int at: -1

    property bool walking: false

    property bool steady: false

    property int tries: 0

    readonly property int settleMs: 600
    readonly property int patience: 60

    property double arrangedAt: 0

    property int invented: 0

    readonly property int talkedFor: 20000

    readonly property var scenes: ({

        "digest": {
            window: () => shooter.theWindow("recordings"),
            arrange: () => shooter.arrangeRecording("answer"),
            ready: () => shooter.recordingReady("answer")
        },
        "transcript": {
            window: () => shooter.theWindow("recordings"),
            arrange: () => shooter.arrangeRecording("reading"),
            ready: () => shooter.recordingReady("reading")
        },

        "recordings": {
            window: () => shooter.theWindow("recordings"),
            arrange: () => shooter.arrangeRecording(""),
            ready: () => shooter.recordingReady("")
        },

        "prompter-translation": {
            window: () => shooter.theWindow("prompter"),
            arrange: () => shooter.arrangePrompter("interpreting"),
            ready: () => shooter.prompterReady()
        },
        "prompter-sales": {
            window: () => shooter.theWindow("prompter"),
            arrange: () => shooter.arrangePrompter("selling"),
            ready: () => shooter.prompterReady()
        },

        "capture": {
            window: () => shooter.phone,
            arrange: () => shooter.arrangeCapture(),
            ready: () => shooter.captureReady()
        },
        "call": {
            window: () => shooter.phone,
            arrange: () => shooter.arrangeCalls(1, "talking"),
            ready: () => calls.hasCall && calls.state === CallViewModel.Active
                         && shooter.longEnough()
        },
        "dialer": {
            window: () => shooter.phone,
            arrange: () => shooter.arrangeDialler(),

            ready: () => calls.canDial && !calls.hasCall
        },
        "calls": {
            window: () => shooter.phone,
            arrange: () => shooter.arrangeCalls(3, "one talking, one parked, one ringing"),
            ready: () => calls.callList.length === 3 && calls.moreThanTheCard
                         && shooter.longEnough()
        },
        "conference": {
            window: () => shooter.phone,
            arrange: () => shooter.arrangeCalls(3, "conference"),
            ready: () => shooter.conferenceReady()
        },
        "directory": {
            window: () => shooter.theWindow("people"),
            arrange: () => shooter.arrangePanel("people")
        },
        "processing": {
            window: () => shooter.theSettings(),
            arrange: () => shooter.arrangeSection("processing")
        },
        "transcription": {
            window: () => shooter.theSettings(),
            arrange: () => shooter.arrangeSection("transcription")
        },
        "dictionaries": {
            window: () => shooter.theSettings(),
            arrange: () => shooter.arrangeSection("dictionaries")
        },
        "accounts": {
            window: () => shooter.theSettings(),
            arrange: () => shooter.arrangeAccounts(),
            ready: () => accounts.editing && accounts.selectedId === shooter.line
        },
        "devices": {
            window: () => shooter.theSettings(),
            arrange: () => shooter.arrangeSection("audio")
        },
        "diagnostics": {
            window: () => shooter.theWindow("diagnostics"),
            arrange: () => shooter.arrangeDiagnostics(),
            ready: () => shooter.diagnosticsReady()
        }
    })

    function conferenceReady() {
        if (calls.canMerge && !calls.inConference) {
            calls.merge()
        }
        return calls.inConference && calls.conferenceList.length === 3
               && shooter.longEnough()
    }

    function longEnough() {
        return Date.now() - shooter.arrangedAt >= shooter.talkedFor
    }

    function arrangeDialler() {
        calls.dialText = shooter.dialled

        shooter.phone.wantedIdle = shooter.phone.viewKeys
        return true
    }

    function arrangeDiagnostics() {
        if (!shooter.arrangePanel("diagnostics")) {
            return false
        }
        shakeTheLine.restart()
        return true
    }

    Timer {
        id: shakeTheLine

        interval: 500
        repeat: false
        onTriggered: accounts.disconnectAccount(shooter.line)
    }

    function theDiagnosticsBody() {
        const window = shooter.theWindow("diagnostics")
        const body = window === null ? null
                                     : ItemsCalled.under(window.contentItem, "diagnosticsBody")
        if (body === null) {
            console.warn("screenshots: the diagnostics panel would not answer to its name")
        }
        return body
    }

    function diagnosticsReady() {

        const body = shooter.theDiagnosticsBody()
        if (body === null) {
            return false
        }

        const seen = diagnostics.messages()
        if (seen.length < 4) {
            return false
        }

        if (body.openedMessage === 0) {

            for (let at = seen.length - 1; at >= 0; --at) {
                if (seen[at].summary.indexOf("REGISTER") >= 0 && !seen[at].incoming) {
                    body.openedMessage = seen[at].sequence
                    break
                }
            }
        }

        return typeof body.openedMessage === "number" && body.openedMessage > 0
    }

    function arrangeCalls(many, what) {
        demo.quiet()

        for (let at = 0; at < many; ++at) {
            const id = "shot-call-" + (shooter.invented++)
            demo.arrives(id, shooter.callers[at])

            if (what !== "one talking, one parked, one ringing" || at + 1 < many) {
                demo.connected(id)
            }
        }

        return true
    }

    function telephoneHere() {
        return uiRegistry.itemsIn("primary").indexOf("softphone.dialler") >= 0
    }

    function arrangeCapture() {
        if (shooter.telephoneHere()) {
            console.warn("screenshots: the capture scene needs a phone assembled",
                         "without the telephone module")
            return false
        }
        if (shots.capturing.length === 0) {
            console.warn("screenshots: nothing was named to capture; see --screenshot-capturing")
            return false
        }

        demoCapture.capturing(shots.capturing, shooter.meetingIn)
        return true
    }

    function captureReady() {
        return settings.captureRunning && captureSound.showing
               && Date.now() - shooter.arrangedAt >= 8000
               && demoCapture.bothVoicesInTheTrail(7)
    }

    function arrangeSection(which) {
        if (shooter.phone.settingsSections().indexOf(which) < 0) {
            console.warn("screenshots: this phone has no settings section called", which)
            return false
        }
        shooter.phone.showSettings(which)
        return true
    }

    function arrangeAccounts() {
        if (!shooter.arrangeSection("accounts")) {
            return false
        }
        accounts.select(shooter.line)
        return true
    }

    function arrangePrompter(which) {
        const office = shots.content
        const script = office.prompter !== undefined ? office.prompter[which] : undefined
        if (script === undefined) {
            console.warn("screenshots: the office carries no prompter scene called", which,
                         "— see --screenshot-content")
            return false
        }
        if (!shooter.phone.hasWindow("prompter")) {
            console.warn("screenshots: no plugin contributes the prompter window; the module",
                         "may be off, or Allow the prompter to be used may be unticked")
            return false
        }

        demo.quiet()
        demoPrompter.forget()

        const id = "shot-call-" + (shooter.invented++)
        demo.arrives(id, script.number)
        demo.connected(id)

        shooter.phone.showPanelNamed("prompter")

        const window = shooter.theWindow("prompter")
        if (window !== null) {
            window.width = 760
        }

        const chosen = shooter.assistantCalled(script.assistant)
        if (chosen === "") {
            console.warn("screenshots: this phone offers no live assistant called",
                         script.assistant)
            return false
        }
        prompter.choose(chosen)

        demoPrompter.begins()
        shooter.saySoFar(script.turns)
        return true
    }

    function saySoFar(turns) {
        for (let at = 0; at < turns.length; ++at) {
            const turn = turns[at]
            const when = 9000 + at * 11000
            demoPrompter.said(turn.voice === "near", turn.text, when)

            if (turn.answer !== undefined && turn.answer !== "") {
                demoPrompter.answered(turn.answer, 1, when)
            }
        }
    }

    function assistantCalled(name) {
        for (const one of prompter.templates) {
            if (one.label === name) {
                return one.id
            }
        }
        return ""
    }

    function prompterReady() {
        return prompter.running && prompter.said.length > 0 && prompter.answers.length > 0
               && Date.now() - shooter.arrangedAt >= 3500
    }

    function arrangePanel(id) {
        if (!shooter.phone.hasWindow(id)) {
            console.warn("screenshots: no plugin contributes the window", id)
            return false
        }
        shooter.phone.showPanelNamed(id)
        return true
    }

    function theRecordingsBody() {
        const window = shooter.theWindow("recordings")
        const body = window === null ? null
                                     : ItemsCalled.under(window.contentItem, "recordingsBody")
        if (body === null) {
            console.warn("screenshots: the recordings panel would not answer to its name")
        }
        return body
    }

    function arrangeRecording(what) {
        if (!shooter.arrangePanel("recordings")) {
            return false
        }

        recordings.closeReading()
        recordings.closePlayer()

        const body = shooter.theRecordingsBody()
        if (body !== null) {
            body.shareTheWindow()
        }

        recordings.open(shooter.conversation)
        if (recordings.openId !== shooter.conversation) {
            console.warn("screenshots: the seeded conversation", shooter.conversation,
                         "is not in this library")
            return false
        }

        if (what === "") {

            recordings.closeReading()
        }
        return true
    }

    function recordingReady(what) {
        if (what === "answer") {
            if (artifacts.openId !== shooter.answer) {
                artifacts.open(shooter.answer)
            }

            return artifacts.openId === shooter.answer && !artifacts.working
                    && (artifacts.structured.length > 0 || artifacts.content.length > 0)
        }

        if (artifacts.openId.length > 0) {
            artifacts.open("")
        }

        if (what === "reading") {
            if (transcript.openId !== shooter.reading) {
                transcript.choose(shooter.reading)
            }
            if (transcript.openId !== shooter.reading || transcript.count === 0) {
                return false
            }

            recordings.seek(shooter.into)
            return true
        }

        return recordings.bothWavesNow().length > 0
    }

    function putEverythingAway() {

        shooter.phone.hideSettings()
        for (const id of uiRegistry.windows) {
            const panel = shooter.phone.panelNamed(id)
            if (panel !== null) {
                panel.hide()
            }
        }
    }

    function linesReady() {

        if (!shooter.telephoneHere()) {
            return true
        }

        let every = true
        for (const line of accounts.accounts) {
            if (!line.enabled) {
                continue
            }
            if (line.state !== AccountsViewModel.Registered) {
                every = false

                if (line.state !== AccountsViewModel.Registering) {
                    accounts.connectAccount(line.id)
                }
            }
        }
        return every
    }

    function begin() {
        shooter.walking = true
        shooter.at = -1
        shooter.step()
    }

    function step() {
        shooter.at += 1
        shooter.tries = 0
        shooter.steady = false
        shooter.arrangedAt = Date.now()

        if (shooter.at >= shots.wanted.length) {
            shooter.phone.leave()
            return
        }

        const name = shots.wanted[shooter.at]
        const scene = shooter.scenes[name]
        if (scene === undefined) {

            console.warn("screenshots: there is no scene called", name)
            Qt.callLater(shooter.step)
            return
        }

        demo.quiet()
        demoCapture.quiet()

        calls.dialText = ""

        shooter.putEverythingAway()
        if (!scene.arrange()) {
            console.warn("screenshots: could not arrange", name)
            Qt.callLater(shooter.step)
            return
        }
        settle.restart()
    }

    Timer {
        id: settle

        interval: shooter.settleMs
        repeat: false

        onTriggered: {
            const name = shots.wanted[shooter.at]
            const scene = shooter.scenes[name]

            const ready = shooter.linesReady()
                          && uiRegistry.settled
                          && (scene.ready === undefined || scene.ready())
            if (ready && !shooter.steady) {
                shooter.steady = true
                settle.restart()
                return
            }

            if (ready && shots.take(name, scene.window())) {
                Qt.callLater(shooter.step)
                return
            }

            shooter.steady = false
            shooter.tries += 1
            if (shooter.tries < shooter.patience) {
                settle.restart()
                return
            }

            console.warn("screenshots: gave up on", name, "after", shooter.tries, "tries")
            Qt.callLater(shooter.step)
        }
    }

    function putThePhoneOnTheScreen() {
        shooter.phone.show()
        shooter.phone.raise()
    }

    Connections {
        target: startup

        function onChanged() {
            if (startup.ready && !shooter.walking) {
                shooter.putThePhoneOnTheScreen()
                begin.restart()
            }
        }
    }

    Timer {
        id: begin

        interval: 1200
        repeat: false
        onTriggered: shooter.begin()
    }

    Component.onCompleted: {
        if (startup.ready) {
            shooter.putThePhoneOnTheScreen()
            begin.restart()
        }
    }
}
