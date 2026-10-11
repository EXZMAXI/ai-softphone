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

    function forTheFirst(room, firstFloor, secondFloor, chosen) {
        if (!(room > 0)) {
            return 0
        }

        const both = firstFloor + secondFloor
        if (room <= both) {

            return room * firstFloor / both
        }

        const wanted = chosen < 0 ? room / 2 : chosen
        return Math.max(firstFloor, Math.min(wanted, room - secondFloor))
    }
}
