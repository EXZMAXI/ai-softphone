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

    readonly property color good: Material.color(Material.Green, Material.Shade700)

    function warn(theme) {
        return Ink.warn(theme)
    }

    readonly property color bad: Material.color(Material.Red, Material.Shade600)

    readonly property color info: Material.color(Material.Blue, Material.Shade700)

    readonly property color muted: Material.color(Material.Grey, Material.Shade600)
}
