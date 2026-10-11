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
import QtQuick.Layouts

import Softphone

ConfirmDialog {
    id: naming

    title: qsTr("Name this recording")
    actionText: qsTr("Save")
    destructive: false

    actionEnabled: chosen.text.trim().length > 0

    onAccepted: settings.renameCapture(chosen.text)
    onOpened: {
        chosen.text = settings.captureWhat
        chosen.forceActiveFocus()
        chosen.selectAll()
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("This is what the recording is listed under. It can be changed later in the recordings window.")
    }

    TextField {
        id: chosen

        TextMenu {}

        Layout.fillWidth: true
        placeholderText: qsTr("Name")
        onAccepted: naming.accept()
    }
}
