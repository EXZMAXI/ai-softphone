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
import QtQuick.Controls

TextField {
    id: field

    TextMenu {}

    required property var cleaner

    signal dialEdited(string dialled)

    inputMethodHints: Qt.ImhDialableCharactersOnly

    onTextEdited: {
        const cleaned = field.cleaner.dialable(field.text)
        if (cleaned !== field.text) {

            const kept = field.cleaner.dialable(
                field.text.substring(0, field.cursorPosition)).length
            field.text = cleaned
            field.cursorPosition = kept
        }
        field.dialEdited(field.text)
    }
}
