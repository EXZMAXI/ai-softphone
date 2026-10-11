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

Item {
    id: panel

    required property var page

    implicitWidth: buttons.any && panel.page.keyColumns === 0 ? panel.page.keyStripWidth : 0

    implicitHeight: {
        if (!buttons.any || panel.page.keyColumns === 0) {
            return 0
        }
        const columns = Math.max(1, panel.page.keyColumns)
        const rows = Math.ceil(buttons.panel.length / columns)
        return Math.min(rows, panel.page.keyBandRows) * 30 + 4
    }

    visible: buttons.any

    Rectangle {

        visible: panel.page.keyColumns === 0 || panel.page.keysNeedARule

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: panel.page.keyColumns === 0 ? undefined : parent.right
        anchors.bottom: panel.page.keyColumns === 0 ? parent.bottom : undefined
        width: panel.page.keyColumns === 0 ? 1 : undefined
        height: panel.page.keyColumns === 0 ? undefined : 1
        color: Qt.rgba(Material.foreground.r, Material.foreground.g, Material.foreground.b,
                       0.12)
    }

    GridView {
        id: keys

        anchors.fill: parent
        anchors.leftMargin: panel.page.keyColumns === 0 ? 1 : 0
        anchors.topMargin: panel.page.keyColumns === 0 ? 8 : 3
        clip: true
        model: buttons.panel

        readonly property int columns: Math.max(1, panel.page.keyColumns)

        cellWidth: Math.floor(keys.width / keys.columns)
        cellHeight: panel.page.keyColumns === 0 ? 44 : 30

        ScrollBar.vertical: ThinScrollBar {}

        delegate: ItemDelegate {
            id: key

            required property var modelData

            width: keys.cellWidth
            height: keys.cellHeight
            onClicked: buttons.dial(key.modelData.id)

            contentItem: RowLayout {
                spacing: 8

                Item {
                    Layout.preferredWidth: 12
                    Layout.preferredHeight: 12
                    Layout.alignment: Qt.AlignVCenter

                    Rectangle {
                        id: lamp

                        anchors.centerIn: parent
                        visible: key.modelData.watched
                        width: 10
                        height: 10
                        radius: width / 2
                        color: page.lampColour(key.modelData.state)

                        SequentialAnimation on opacity {
                            running: key.modelData.state === ButtonsViewModel.Ringing
                            loops: Animation.Infinite
                            alwaysRunToEnd: true
                            NumberAnimation { to: 0.35; duration: 500 }
                            NumberAnimation { to: 1.0; duration: 500 }
                        }
                    }

                    Glyph {
                        anchors.fill: parent
                        visible: !key.modelData.watched
                        source: page.glyph("call")
                        opacity: 0.45
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: key.modelData.label
                    elide: Text.ElideRight
                    font.pixelSize: 12
                }
            }

            Hint {
                owner: key
                visible: key.hovered
                text: page.keyHint(key.modelData)
            }
        }
    }
}
