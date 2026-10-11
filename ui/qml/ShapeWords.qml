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
    id: words

    function labelFor(value) {
        void Qt.uiLanguage

        switch (value) {
        case SettingsViewModel.Layout.Compact:

            return qsTr("Compact")
        case SettingsViewModel.Layout.Workspace:

            return qsTr("One window")
        case SettingsViewModel.Layout.WorkspaceAndPhone:

            return qsTr("Two windows")
        case SettingsViewModel.Layout.Bar:

            return qsTr("Edge bar")
        case SettingsViewModel.Layout.NoWindow:

            return qsTr("Tray only")
        default:

            return qsTr("Multiple windows")
        }
    }

    function wordFor(value) {
        switch (value) {
        case SettingsViewModel.Layout.Compact:
            return "compact"
        case SettingsViewModel.Layout.Workspace:
            return "workspace"
        case SettingsViewModel.Layout.WorkspaceAndPhone:
            return "workspace-and-phone"
        case SettingsViewModel.Layout.Bar:
            return "bar"
        case SettingsViewModel.Layout.NoWindow:
            return "no-window"
        default:
            return "windows"
        }
    }
}
