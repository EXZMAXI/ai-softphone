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
import QtQuick.Controls.Material
import QtQuick.Layouts

NoticeBanner {
    id: banner

    property string who: ""
    property string number: ""

    property string line: ""

    property string glyphs: "qrc:/glyphs/filled"

    property int glyphSize: 24

    signal answered()
    signal rejected()
    signal shown()

    heading: banner.line.length > 0 ? qsTr("Incoming call · %1").arg(banner.line)
                                    : qsTr("Incoming call")
    name: banner.number.length > 0 && banner.number !== banner.who
          ? qsTr("%1 (%2)").arg(banner.who).arg(banner.number)
          : banner.who

    onPressed: banner.shown()

    RowLayout {

        Layout.fillWidth: false
        Layout.alignment: Qt.AlignRight
        spacing: 8

        Button {
            id: rejectKey

            highlighted: true

            Material.background: Mark.bad
            icon.source: banner.glyphs + "/call-end.svg"
            icon.width: banner.glyphSize
            icon.height: banner.glyphSize
            display: AbstractButton.IconOnly
            text: qsTr("Reject")
            Accessible.name: text
            onClicked: banner.rejected()

            Hint {
                owner: rejectKey
                visible: rejectKey.hovered
                text: rejectKey.text
            }
        }

        Button {
            id: answerKey

            highlighted: true

            Material.background: Mark.good
            icon.source: banner.glyphs + "/call.svg"
            icon.width: banner.glyphSize
            icon.height: banner.glyphSize
            display: AbstractButton.IconOnly
            text: qsTr("Answer")
            Accessible.name: text
            onClicked: banner.answered()

            Hint {
                owner: answerKey
                visible: answerKey.hovered
                text: answerKey.text
            }
        }
    }
}
