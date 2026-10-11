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

    function forRecording(row) {
        if (!row) {
            return "call-outbound"
        }

        if (row.imported === true) {
            return "arrow-import"
        }
        if (row.external === true) {
            return "apps"
        }
        return row.incoming === true ? "call-inbound" : "call-outbound"
    }
}
