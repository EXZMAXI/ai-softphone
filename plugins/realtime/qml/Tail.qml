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

import Softphone

Item {
    id: tail

    property var model: []

    property string empty: ""

    property bool answersColumn: false

    property bool following: true

    property int heldIndex: -1

    property int textPixels: 13

    readonly property int stampPixels: Math.max(9, tail.textPixels - 2)

    property bool pickable: false

    signal picked(string text, bool theLastOne)

    RowMenu {
        id: behind

        z: -1

        mayAnswer: where => rows.itemAt(where.x, where.y + rows.contentY) === null

        extra: [{
            label: tail.answersColumn

                ? qsTr("Copy every suggestion")

                : qsTr("Copy the whole transcript"),
            picture: settings.glyphSet + "/copy.svg",
            enabled: rows.count > 0,
            act: () => settings.copyToClipboard(tail.answersColumn
                                                ? prompter.answersAsText()
                                                : prompter.saidAsText())
        }]
    }

    ListView {
        id: rows

        anchors.fill: parent
        clip: true
        spacing: 6

        model: tail.model.length

        ScrollBar.vertical: ThinScrollBar {
            id: bar

            always: true

            onPressedChanged: pressed ? tail.following = false : tail.settle()
        }

        onMovementStarted: tail.following = false

        onMovementEnded: tail.settle()

        onContentHeightChanged: if (tail.following) Qt.callLater(rows.toTheNewest)

        function toTheNewest() {

            if (tail.following) {
                rows.positionViewAtEnd()
            }
        }

        Connections {
            target: tail

            function onModelChanged() {
                if (tail.following || tail.heldIndex < 0) {
                    return
                }
                rows.positionViewAtIndex(Math.min(tail.heldIndex, rows.count - 1),
                                         ListView.Beginning)
            }
        }

        delegate: Item {
            id: line

            required property int index

            readonly property var modelData: tail.model[line.index] !== undefined
                                             ? tail.model[line.index]
                                             : ({})

            width: rows.width - bar.width - 4
            implicitHeight: words.implicitHeight
            height: words.implicitHeight

            TapHandler {
                enabled: tail.pickable
                acceptedButtons: Qt.LeftButton
                onTapped: tail.picked(line.modelData.text ?? "",
                                      line.index === rows.count - 1)
            }

            HoverHandler {
                enabled: tail.pickable
                cursorShape: Qt.PointingHandCursor
            }

            Column {
                id: words

                width: parent.width
                spacing: 2

                Label {
                    width: parent.width
                    visible: !tail.answersColumn && (modelData.gap ?? 0) > 0

                    text: qsTr("— %1 s missing —").arg(((modelData.gap ?? 0) / 1000).toFixed(1))
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: tail.stampPixels
                    color: Ink.warn(Material.theme)
                }

                Label {
                    width: parent.width
                    visible: text.length > 0
                    text: {
                        if (tail.answersColumn) {
                            const covers = modelData.covers ?? 1;

                            return covers > 1
                                ? qsTr("%1 · about %2 replies").arg(modelData.at ?? "").arg(covers)
                                : (modelData.at ?? "");
                        }

                        void Qt.uiLanguage
                        const side = prompter.sideWord(modelData.near ? "near" : "far");
                        return (modelData.at ?? "").length > 0
                            ? qsTr("%1 · %2").arg(modelData.at).arg(side)
                            : side;
                    }
                    font.pixelSize: tail.stampPixels
                    opacity: 0.6
                }

                Label {
                    width: parent.width
                    text: modelData.text ?? ""
                    wrapMode: Text.WordWrap
                    font.pixelSize: tail.textPixels
                    opacity: (modelData.provisional ?? false) ? 0.55 : 1.0
                }
            }

            RowMenu {
                extra: tail.answersColumn
                        ? [{

                            label: qsTr("Copy this suggestion"),
                            picture: settings.glyphSet + "/copy.svg",
                            act: () => settings.copyToClipboard(modelData.text ?? "")
                        }, {

                            label: qsTr("Copy every suggestion"),
                            picture: settings.glyphSet + "/copy.svg",
                            act: () => settings.copyToClipboard(prompter.answersAsText())
                        }]
                        : [{

                            label: qsTr("Copy this line"),
                            picture: settings.glyphSet + "/copy.svg",
                            act: () => settings.copyToClipboard(modelData.text ?? "")
                        }, {

                            label: qsTr("Copy the whole transcript"),
                            picture: settings.glyphSet + "/copy.svg",
                            act: () => settings.copyToClipboard(prompter.saidAsText())
                        }]
            }
        }
    }

    function settle() {
        tail.following = rows.atYEnd
        tail.heldIndex = tail.following ? -1 : rows.indexAt(1, rows.contentY + 1)
    }

    Label {
        anchors.centerIn: parent
        width: parent.width - 24
        visible: rows.count === 0 && tail.empty.length > 0
        text: tail.empty
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }
}
