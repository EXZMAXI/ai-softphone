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
    id: notice

    property string what: ""

    property string process: ""

    signal recorded()

    signal declined()

    signal always()

    heading: qsTr("A conversation was heard")
    name: notice.what

    onPressed: notice.declined()

    RowLayout {
        Layout.fillWidth: false
        Layout.alignment: Qt.AlignRight
        spacing: 8

        Button {
            flat: true
            text: qsTr("Not now")
            onClicked: notice.declined()
        }

        Button {
            highlighted: true
            text: qsTr("Record")
            onClicked: notice.recorded()
        }
    }

    Button {
        Layout.fillWidth: true
        flat: true
        visible: notice.what.length > 0

        text: qsTr("Always record this application")
        onClicked: notice.always()
    }
}
