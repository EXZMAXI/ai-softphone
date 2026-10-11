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

Loader {
    id: slot

    required property string what

    required property var page

    property string noun: "the contributed item"

    property bool aSection: false

    property int asked: 0
    readonly property int mostAsks: 3

    Component.onCompleted: slot.build()

    function build() {
        const where = slot.aSection ? uiRegistry.componentOf(slot.what)
                                    : uiRegistry.componentOfItem(slot.what)
        if (where == "") {
            return
        }

        if (slot.source == where) {
            return
        }

        slot.setSource(where, { page: slot.page })
    }

    function buildAgain() {
        slot.source = ""
        slot.build()
    }

    onStatusChanged: {
        if (slot.status !== Loader.Error) {
            return
        }

        if (slot.asked < slot.mostAsks) {
            slot.asked += 1
            console.warn(slot.noun, slot.what,
                         "was interrupted while it was being built; asking again, attempt",
                         slot.asked)
            Qt.callLater(slot.buildAgain)
            return
        }

        console.warn(slot.noun, slot.what, "did not load:", slot.source)
    }
}
