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

    required property var page
    Layout.fillWidth: true
    spacing: 12

    Label {
        Layout.fillWidth: true
        text: qsTr("Dictionaries")
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("What a conversation may be filed under, labelled with, or flagged for. These lists are what the models are shown and what they must choose from, so an answer is always something you can search for later.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        ListCheckBox {
            Layout.fillWidth: true

            text: qsTr("Show deleted")
            checked: taxonomy.showRetired
            onToggled: taxonomy.setShowRetired(checked)
        }
    }

    DictionaryList {
        settingsPage: page
        Layout.fillWidth: true
        kind: "category"
        heading: qsTr("Categories")
        rows: taxonomy.categories
        hint: qsTr("What the conversation was about, one is chosen per conversation.")
    }

    DictionaryList {
        settingsPage: page
        Layout.fillWidth: true
        kind: "tag"
        heading: qsTr("Tags")
        rows: taxonomy.tags
        hint: qsTr("Labels that may all be true of the same conversation.")
    }

    DictionaryList {
        settingsPage: page
        Layout.fillWidth: true
        kind: "red_flag"
        heading: qsTr("Red flags")
        rows: taxonomy.redFlags
        hint: qsTr("Things worth somebody's attention. Each one found is kept with the words it was found in.")
    }

    DictionaryList {
        settingsPage: page
        Layout.fillWidth: true
        kind: "instruction"
        heading: qsTr("Instructions")
        rows: taxonomy.instructions
        hint: qsTr("What is added to every prompt about the shape of the answer and the language to write it in. Written in the language this program was in when the dictionaries were filled, and left alone when you change it — Start again is what rewrites them.")
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 12
        text: qsTr("Defaults")
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Puts every dictionary back as it came with the program. What your conversations are already filed under is left alone.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Button {
        Layout.fillWidth: true
        topInset: 0
        bottomInset: 0
        visible: !taxonomy.editing
        text: qsTr("Restore defaults")
        onClicked: page.restoreDictionaries.open()
    }

    Item { Layout.fillHeight: true }
}
