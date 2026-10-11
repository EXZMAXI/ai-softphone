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

import Softphone

ColumnLayout {

    required property var page

    Layout.fillWidth: true
    spacing: 12

    function wordFor(state) {

        void Qt.uiLanguage

        switch (state) {
        case "running": return qsTr("Running")
        case "off": return qsTr("Off")
        case "blocked": return qsTr("Waiting for another")
        case "cycle": return qsTr("Depends on itself")
        case "failed": return qsTr("Did not start")
        }
        return qsTr("Unknown")
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Modules")
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        opacity: 0.75
        font.pixelSize: 12
        text: qsTr("Everything this phone does is one of these. Turning one off takes effect at once and leaves what it has already stored where it is.")
    }

    Label {
        Layout.fillWidth: true
        visible: plugins.problem.length > 0
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        color: Ink.bad(Material.theme)
        text: plugins.problem
    }

    Repeater {
        model: plugins.rows

        delegate: ColumnLayout {
            required property var modelData

            Layout.fillWidth: true
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ListCheckBox {

                    objectName: "module." + modelData.id

                    checked: modelData.enabled

                    enabled: !modelData.required

                    onToggled: {
                        if (checked) {
                            plugins.setEnabled(modelData.id, true)
                            return
                        }

                        const going = plugins.dependentsOf(modelData.id)
                        const doingLess = plugins.suggestersOf(modelData.id)
                        if (going.length === 0 && doingLess.length === 0) {
                            plugins.setEnabled(modelData.id, false)
                            return
                        }

                        checked = true
                        page.confirmDisabling(modelData.id, modelData.name, going, doingLess)
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: modelData.name
                    font.pixelSize: 14
                }

                Label {
                    text: modelData.missing ? qsTr("Plugin file is gone") : wordFor(modelData.state)
                    opacity: 0.75
                    font.pixelSize: 12
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 36
                Layout.bottomMargin: 8
                wrapMode: Text.WordWrap
                opacity: 0.65
                font.pixelSize: 12
                text: modelData.problem.length > 0 ? modelData.problem : modelData.summary
            }
        }
    }

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: plugins.rows.length === 0
    }
}
