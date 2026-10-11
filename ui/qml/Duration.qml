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

QtObject {

    function text(ms) {
        const total = Math.max(0, Math.floor(ms / 1000))
        const seconds = total % 60
        const minutes = Math.floor(total / 60) % 60
        const hours = Math.floor(total / 3600)

        const pad = (value) => value < 10 ? "0" + value : "" + value
        return hours > 0 ? hours + ":" + pad(minutes) + ":" + pad(seconds)
                         : minutes + ":" + pad(seconds)
    }
}
