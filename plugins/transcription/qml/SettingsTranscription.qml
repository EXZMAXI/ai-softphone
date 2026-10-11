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
import QtQuick.Controls.Material.impl
import QtQuick.Layouts

import Qt.labs.platform as Platform

import Softphone

ColumnLayout {
    id: settingsTranscription

    readonly property string byDefault: qsTr("by default")

    required property var page
    Layout.fillWidth: true
    spacing: 12

    Label {
        Layout.fillWidth: true
        text: qsTr("Transcription")
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("A conversation is transcribed when you ask for it in the recordings window — or by itself, if you have turned processing on under Processing. A recogniser on your own machine costs nothing to run; one in the cloud charges by the minute of audio.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 12
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: qsTr("Recognisers")
            font.pixelSize: 15
            font.bold: true
        }

        Button {

            objectName: "theWayIn"
            visible: !providers.editing
            text: qsTr("Add")
            onClicked: providers.add()
        }
    }

    Label {
        Layout.fillWidth: true
        visible: providers.count === 0 && !providers.editing
        text: qsTr("Nothing is set up yet. A Whisper server on your own machine keeps every conversation in the building; OpenAI needs a key and an internet connection.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    ColumnLayout {
        id: providersColumn

        Layout.fillWidth: true
        spacing: 0

    Repeater {
        model: providers.rows

        delegate: ColumnLayout {
            id: providerEntry

            required property var modelData

            Layout.fillWidth: true
            spacing: 0

        ItemDelegate {
            readonly property var modelData: providerEntry.modelData

            Layout.fillWidth: true
            padding: 6
            highlighted: providers.editingId === modelData.id
            onClicked: providers.editingId === modelData.id
                       ? providers.cancel()
                       : providers.edit(modelData.id)

            contentItem: RowLayout {
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: modelData.name
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: modelData.model.length > 0
                              ? modelData.model + " · " + modelData.endpoint
                              : modelData.endpoint
                        elide: Text.ElideRight
                        font.pixelSize: 12
                        opacity: 0.6
                    }
                }

                Glyph {
                    id: transcribesMark

                    readonly property bool preferred: modelData.isDefault === true

                    Layout.alignment: Qt.AlignVCenter
                    width: 16
                    height: 16
                    visible: modelData.transcribes === true
                    source: settings.glyphSet + "/clock.svg"
                    color: preferred ? Mark.warn(Material.theme) : Material.foreground
                    opacity: preferred ? 1.0 : 0.7

                    HoverHandler {
                        id: transcribesHover

                        cursorShape: Qt.PointingHandCursor
                    }

                    TapHandler {
                        onTapped: providers.makeDefault(modelData.id, false,
                                                          !transcribesMark.preferred)
                    }

                    Hint {

                        readonly property string what: qsTr("Transcribes recorded conversations")

                        readonly property string press: qsTr("Press to change")

                        owner: transcribesMark
                        visible: transcribesHover.hovered

                        text: (transcribesMark.preferred
                               ? qsTr("%1 · %2").arg(what).arg(settingsTranscription.byDefault)
                               : what) + " · " + press
                    }
                }

                Glyph {
                    id: listensMark

                    readonly property bool preferred: modelData.isRealtimeDefault === true

                    Layout.alignment: Qt.AlignVCenter
                    width: 16
                    height: 16
                    visible: modelData.listens === true
                    source: settings.glyphSet + "/flash.svg"
                    color: preferred ? Mark.warn(Material.theme) : Material.foreground
                    opacity: preferred ? 1.0 : 0.7

                    HoverHandler {
                        id: listensHover

                        cursorShape: Qt.PointingHandCursor
                    }

                    TapHandler {
                        onTapped: providers.makeDefault(modelData.id, true,
                                                          !listensMark.preferred)
                    }

                    Hint {

                        readonly property string what: qsTr("Listens while a conversation is happening")

                        readonly property string press: qsTr("Press to change")

                        owner: listensMark
                        visible: listensHover.hovered
                        text: (listensMark.preferred
                               ? qsTr("%1 · %2").arg(what).arg(settingsTranscription.byDefault)
                               : what) + " · " + press
                    }
                }

                Label {
                    visible: !modelData.enabled

                    text: qsTr("off", "a recogniser")
                    font.pixelSize: 12
                    opacity: 0.6
                }
            }
        }

            CardLoader {
                settingsPage: page
                shows: providerEntry
                about: providers
                active: providers.editing
                        && providers.editingId === providerEntry.modelData.id
                card: settingsPage.providerCard
            }
        }
    }
    }

    Button {
        Layout.fillWidth: true
        visible: !providers.editing
                 && providersColumn.implicitHeight > page.scroller.height - 140
        text: qsTr("Add")
        onClicked: providers.add()
    }

    CardLoader {
        settingsPage: page
        about: providers
        active: providers.editing && providers.editingId.length === 0
        card: settingsPage.providerCard
    }

    Item { Layout.fillHeight: true }
}
