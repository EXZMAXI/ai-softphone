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

SidePanel {
    id: settingsWindow

    memoryKey: "settings"

    title: settingsBody.windowTitle

    width: settingsBody.implicitWidth
    minimumWidth: settingsBody.leastWidth

    property alias registrationProblem: settingsBody.registrationProblem
    property alias registrationSummary: settingsBody.registrationSummary
    property alias trayAvailable: settingsBody.trayAvailable
    property alias trayShown: settingsBody.trayShown

    property alias section: settingsBody.section
    readonly property alias sectionKeys: settingsBody.sectionKeys

    signal diagnosticsRequested

    signal apiPageRequested(string address)

    signal windowRequested(string id, string control)
    signal phoneRequested

    SettingsBody {
        id: settingsBody

        anchors.fill: parent

        glyphs: settingsWindow.glyphs
        onScreen: settingsWindow.visible
        kept: settingsWindow.visible || settingsWindow.awayWithThePhone
        settling: settingsWindow.placing
        focused: settingsWindow.active

        onDiagnosticsRequested: settingsWindow.diagnosticsRequested()
        onApiPageRequested: (address) => settingsWindow.apiPageRequested(address)
        onWindowRequested: (id, control) => settingsWindow.windowRequested(id, control)
        onPhoneRequested: settingsWindow.phoneRequested()
    }
}
