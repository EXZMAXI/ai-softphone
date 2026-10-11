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

QtObject {
    id: overview

    function hasSection(id) {
        return uiRegistry.settingsSections.indexOf(id) >= 0
    }

    function hasWindow(id) {
        return uiRegistry.windows.indexOf(id) >= 0
    }

    function lit(code) {
        void deeds.revision

        switch (code) {
        case "calls.account":
            return accounts.count > 0
        case "capture.on":
            return settings.captureEnabled

        case "prompter.allowed":
            return settings.prompterOffered
        case "transcription.provider":
            return providers.count > 0
        case "processing.provider":
            return models.count > 0
        case "application.autostart":
            return settings.autostart

        case "application.updates":
            return settings.automaticUpdates
        case "feedback.reporting":
            return telemetry.mode === "extended"
        default:
            return deeds.wasDone(code)
        }
    }

    function moduleState(id) {
        void plugins.rows

        for (const row of plugins.rows) {
            if (row.id === id) {
                return row.state
            }
        }
        return ""
    }

    function moduleOff(id) {
        return overview.moduleState(id) === "off"
    }

    function moduleName(id) {
        void plugins.rows

        for (const row of plugins.rows) {
            if (row.id === id) {
                return row.name
            }
        }
        return id
    }

    function offerOf(id, module) {
        return {
            id: id,
            steps: [{
                code: id + ".module",
                module: module,
                section: overview.hasSection("plugins") ? "plugins" : "",

                control: "module." + module
            }]
        }
    }

    readonly property var plan: {
        void uiRegistry.settingsSections
        void uiRegistry.windows
        void plugins.rows

        const groups = []
        const library = overview.hasWindow("recordings")

        if (overview.hasSection("accounts")) {
            const steps = [
                { code: "calls.account", section: "accounts", control: "theWayIn" },
                { code: "calls.outgoing", phone: true },

                { code: "calls.incoming",
                  section: overview.hasSection("appearance") ? "appearance" : "",
                  control: "incomingNotice" }
            ]
            if (overview.hasSection("recording")) {
                steps.push({ code: "calls.recorded", section: "recording",
                             control: "theWayIn" })
            }
            groups.push({ id: "calls", steps: steps })
        } else if (overview.moduleOff("softphone")) {
            groups.push(overview.offerOf("calls", "softphone"))
        }

        if (overview.hasSection("capture")) {
            const steps = [
                { code: "capture.on", section: "capture", control: "theWayIn" },
                { code: "capture.first", section: "capture", control: "theWayIn" }
            ]
            if (library) {
                steps.push({ code: "capture.named", window: "recordings" })
            }
            groups.push({ id: "capture", steps: steps })
        } else if (overview.moduleOff("capture")) {
            groups.push(overview.offerOf("capture", "capture"))
        }

        if (overview.hasSection("transcription")) {
            const steps = [
                { code: "transcription.provider", section: "transcription",
                  control: "theWayIn" }
            ]
            if (library) {
                steps.push({ code: "transcription.first", window: "recordings" })
            }
            groups.push({ id: "transcription", steps: steps })
        } else if (overview.moduleOff("transcription")) {
            groups.push(overview.offerOf("transcription", "transcription"))
        }

        if (overview.hasSection("processing")) {
            const steps = [
                { code: "processing.provider", section: "processing",
                  control: "theWayIn" }
            ]
            if (library) {
                steps.push({ code: "processing.first", window: "recordings" })
            }
            groups.push({ id: "processing", steps: steps })
        } else if (overview.moduleOff("processing")) {
            groups.push(overview.offerOf("processing", "processing"))
        }

        if (overview.hasSection("realtime")) {
            const live = [
                { code: "prompter.allowed", section: "realtime", control: "theWayIn" }
            ]

            const reachable = overview.hasWindow("prompter")
            live.push({ code: "prompter.first",
                        window: reachable ? "prompter" : "",
                        section: reachable ? "" : "realtime",
                        control: reachable ? "prompterStart" : "theWayIn" })
            groups.push({ id: "prompter", steps: live })
        } else if (overview.moduleOff("realtime")) {
            groups.push(overview.offerOf("prompter", "realtime"))
        }

        if (library) {
            groups.push({
                            id: "library",
                            steps: [
                                { code: "library.search", window: "recordings" },
                                { code: "library.export.recording", window: "recordings" },
                                { code: "library.export.summary", window: "recordings" }
                            ]
                        })
        } else if (overview.moduleOff("mediastorage")) {
            groups.push(overview.offerOf("library", "mediastorage"))
        }

        groups.push({
                        id: "application",
                        steps: [
                            { code: "application.autostart", control: "theWayIn",
                              section: overview.hasSection("startup") ? "startup" : "" },
                            { code: "application.updates", control: "updatesTick",
                              section: overview.hasSection("about") ? "about" : "" }
                        ]
                    })

        groups.push({
                        id: "feedback",
                        steps: [
                            { code: "feedback.reporting", control: "reportingChoice",
                              section: overview.hasSection("about") ? "about" : "" },
                            { code: "feedback.sent", control: "feedbackBox",
                              section: overview.hasSection("about") ? "about" : "" }
                        ]
                    })

        return groups
    }

    readonly property var rows: {
        const out = []
        for (const group of overview.plan) {
            out.push({ heading: true, code: group.id, section: "", control: "", window: "",
                         phone: false, module: "" })
            for (const step of group.steps) {
                out.push({
                             heading: false,
                             code: step.code,
                             section: step.section !== undefined ? step.section : "",
                             control: step.control !== undefined ? step.control : "",
                             window: step.window !== undefined ? step.window : "",
                             phone: step.phone === true,

                             module: step.module !== undefined ? step.module : ""
                         })
            }
        }
        return out
    }

    function counts(row) {
        return !row.heading && row.module === ""
    }

    readonly property int stepCount: {
        let count = 0
        for (const row of overview.rows) {
            if (overview.counts(row)) {
                count += 1
            }
        }
        return count
    }

    readonly property int greenCount: {
        let count = 0
        for (const row of overview.rows) {
            if (overview.counts(row) && overview.lit(row.code)) {
                count += 1
            }
        }
        return count
    }

}
