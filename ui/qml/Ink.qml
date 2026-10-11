// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

pragma Singleton

import QtQuick
import QtQuick.Controls.Material

QtObject {

    function good(theme) {
        return Material.color(Material.Green, theme === Material.Dark
                              ? Material.Shade200 : Material.Shade800)
    }

    function warn(theme) {
        return theme === Material.Dark
               ? Material.color(Material.Amber)
               : Qt.darker(Material.color(Material.Amber, Material.Shade900), 1.4)
    }

    function bad(theme) {
        return Material.color(Material.Red, theme === Material.Dark
                              ? Material.Shade300 : Material.Shade800)
    }

    function info(theme) {
        return Material.color(Material.Blue, theme === Material.Dark
                              ? Material.Shade300 : Material.Shade800)
    }

    function muted(theme) {
        return Material.color(Material.Grey, theme === Material.Dark
                              ? Material.Shade400 : Material.Shade700)
    }
}
