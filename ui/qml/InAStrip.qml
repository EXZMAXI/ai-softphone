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
import QtQuick.Controls

QtObject {
    id: place

    function centre(dialog, over) {
        if (dialog === null || over === null) {
            return
        }

        const host = over.Window.window
        const roomless = host !== null && host.screen !== null
                         && host.height < dialog.height + 48

        const room = roomless
                     ? Qt.rect(host.screen.virtualX - host.x, host.screen.virtualY - host.y,
                               host.screen.width, host.screen.height)
                     : Qt.rect(0, 0, over.width, over.height)

        const from = dialog.parent !== null ? dialog.parent.mapToItem(null, 0, 0)
                                            : Qt.point(0, 0)
        dialog.x = Math.round(room.x + (room.width - dialog.width) / 2 - from.x)
        dialog.y = Math.round(room.y + (room.height - dialog.height) / 2 - from.y)
    }
}
