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

import Softphone

QtObject {

    function forRow(row) {
        if (!row) {
            return "call-outbound"
        }

        if (row.refused === true) {
            return "call-dismiss"
        }

        if (row.outcome === HistoryViewModel.Outcome.PickedUpElsewhere) {
            return "call-checkmark"
        }

        if (row.outcome === HistoryViewModel.Outcome.Missed && row.incoming === true) {
            return "call-missed"
        }
        return row.incoming === true ? "call-inbound" : "call-outbound"
    }
}
