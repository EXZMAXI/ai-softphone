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
    id: contactsTab

    property string glyphs: ""

    property bool chosen: false

    property var dial: function (place) { place() }

    readonly property string title: qsTr("Contacts")

    readonly property string drawing: "book-contacts"

    signal dialRefused(int reason)

    readonly property int controlHeight: 40

    readonly property int filterHeight: 44

    function glyph(name) {
        return contactsTab.glyphs === "" ? "" : contactsTab.glyphs + "/" + name + ".svg"
    }

    property string chosenContact: ""

    function lineLabel(id) {
        void Qt.uiLanguage

        if (id === "") {

            return qsTr("Line")
        }
        for (const line of contacts.lines) {
            if (line.id === id) {
                return line.label
            }
        }
        return id
    }

    function emptyText() {
        void Qt.uiLanguage

        if (!contacts.available) {
            return qsTr("The address book cannot be opened, so contacts cannot be shown or changed. The phone works as usual.")
        }
        if (contacts.shown > 0) {
            return ""
        }
        return contacts.total === 0
               ? qsTr("No contacts yet. Add somebody, and their calls will arrive with their name.")
               : qsTr("Nothing here matches that.")
    }

    function contactDetail(row) {
        void Qt.uiLanguage

        if (row.lineLabel === "") {
            return row.number
        }

        const line = row.lineMissing
                   ? qsTr("%1 — this line is gone; calls go out on the current one")
                     .arg(row.lineLabel)
                   : row.lineLabel
        return qsTr("%1 · %2").arg(row.number).arg(line)
    }

    function faultText(fault) {
        void Qt.uiLanguage

        switch (fault) {
        case ContactsViewModel.Fault.MissingName:
            return qsTr("A contact needs a name.")
        case ContactsViewModel.Fault.MissingNumber:
            return qsTr("A contact needs a number.")
        default:
            return ""
        }
    }

    readonly property bool isTheBook: true

    property Component contactCard: Component {
        Pane {
            id: card

            leftPadding: 16
            rightPadding: 16
            topPadding: 8
            bottomPadding: 12

            background: Rectangle {
                color: Qt.rgba(Material.foreground.r, Material.foreground.g,
                               Material.foreground.b, 0.06)
            }

            contentItem: ColumnLayout {
                spacing: 12

                TextField {
                    id: nameField

                    TextMenu {}

                    Layout.fillWidth: true

                    placeholderText: qsTr("Name", "of a person")
                    text: contacts.name
                    onTextEdited: contacts.name = text

                    Component.onCompleted: nameField.forceActiveFocus()
                }

                TextField {

                    TextMenu {}

                    Layout.fillWidth: true
                    placeholderText: qsTr("Number")
                    text: contacts.number
                    onTextEdited: contacts.number = text
                }

                PickerBox {
                    id: linePicker

                    Layout.fillWidth: true

                    model: {
                        const lines = [""]
                        for (const line of contacts.lines) {
                            lines.push(line.id)
                        }
                        return lines
                    }

                    displayText: contactsTab.lineLabel(currentValue === undefined
                                                       ? "" : currentValue)

                    delegate: ItemDelegate {

                        required property var modelData
                        required property int index

                        width: linePicker.width
                        text: contactsTab.lineLabel(modelData)
                        highlighted: linePicker.highlightedIndex === index
                    }

                    onActivated: contacts.accountId = currentValue

                    function markTheStoredOne() {
                        const at = linePicker.model.indexOf(contacts.accountId)
                        linePicker.currentIndex = at < 0 ? 0 : at
                    }

                    Component.onCompleted: linePicker.markTheStoredOne()

                    Connections {
                        target: contacts

                        function onEditorChanged() {
                            linePicker.markTheStoredOne()
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: text !== "" && (contacts.name !== "" || contacts.number !== "")
                    wrapMode: Text.Wrap
                    color: Ink.bad(Material.theme)
                    text: contactsTab.faultText(contacts.fault)
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Save")
                        enabled: contacts.canSave
                        highlighted: true
                        onClicked: contacts.save()
                    }

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Cancel")
                        onClicked: contacts.cancelEdit()
                    }

                    Button {
                        Layout.fillWidth: true
                        visible: contacts.editingId !== ""
                        text: qsTr("Delete")
                        Material.foreground: Material.Red
                        onClicked: removeDialog.open()
                    }
                }
            }
        }
    }

    ConfirmDialog {
        id: removeDialog

        title: qsTr("Delete this contact?")

        property string removing: ""
        property string named: ""
        onAboutToShow: {
            removeDialog.removing = contacts.editingId
            removeDialog.named = contacts.name
        }
        actionText: qsTr("Delete")
        onAccepted: {
            contacts.removeContact(removeDialog.removing)
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("%1 will be deleted. Calls already in the history keep the number.")
                  .arg(removeDialog.named)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        focus: true

        Keys.onEscapePressed: (event) => {
            contacts.cancelEdit()
            contactsTab.chosenContact = ""
            contactsTab.chosenCall = ""

            contactsTab.forceActiveFocus()
            event.accepted = true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            Layout.topMargin: 0
            spacing: 8
            visible: contacts.available

            TextField {
                id: searchField

                TextMenu {}

                implicitHeight: contactsTab.filterHeight

                Layout.fillWidth: true
                placeholderText: qsTr("Search")
                text: contacts.search
                onTextEdited: contacts.search = text

                rightPadding: clearSearch.visible ? clearSearch.width + 8 : undefined

                ToolButton {
                    id: clearSearch

                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    visible: searchField.text !== ""
                    display: AbstractButton.IconOnly
                    icon.source: contactsTab.glyph("dismiss")
                    icon.width: 14
                    icon.height: 14
                    flat: true
                    text: qsTr("Clear the search")
                    Accessible.name: text

                    Hint {
                        owner: clearSearch
                        visible: clearSearch.hovered
                        text: clearSearch.text
                    }
                    onClicked: {
                        contacts.search = ""
                        searchField.forceActiveFocus()
                    }
                }
            }

            Button {

                implicitHeight: contactsTab.filterHeight
                topInset: 0
                bottomInset: 0

                text: qsTr("Add")
                enabled: contacts.available && !contacts.editing
                onClicked: contacts.addContact()
            }
        }

        Loader {
            Layout.fillWidth: true
            Layout.leftMargin: 0
            Layout.rightMargin: 0

            Layout.preferredHeight: item ? item.implicitHeight : 0

            visible: active

            active: contacts.editing && contacts.editingId === ""
            sourceComponent: contactsTab.contactCard
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list

                anchors.fill: parent
                visible: contacts.available && contacts.shown > 0
                clip: true

                spacing: 0

                model: contacts.rows

                ScrollBar.vertical: ThinScrollBar {}

                delegate: Column {
                    id: contactEntry

                    required property var modelData
                    required property int index

                    width: list.width

                    ItemDelegate {
                        id: contactRow

                        readonly property var modelData: contactEntry.modelData

                        RowMenu {
                            acts: contactRowActs
                            number: contactRow.modelData.number
                        }

                        width: list.width

                        height: Math.max(contactText.implicitHeight, contactsTab.controlHeight) + 16
                        hoverEnabled: true

                        readonly property bool showsControls:
                            contactRow.hovered || contactsTab.chosenContact === contactRow.modelData.id
                        highlighted: contactsTab.chosenContact === modelData.id

                        onClicked: contactsTab.chosenContact =
                            contactsTab.chosenContact === modelData.id ? "" : modelData.id

                        TapHandler {
                            acceptedButtons: Qt.LeftButton
                            onDoubleTapped: contactsTab.dial(
                                () => contacts.dial(contactRow.modelData.id))
                        }

                        RowLayout {
                            id: row

                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16

                            spacing: 0

                            ColumnLayout {
                                id: contactText

                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                spacing: 2

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.name
                                    elide: Text.ElideRight
                                    font.pixelSize: 15
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: contactsTab.contactDetail(modelData)
                                    elide: Text.ElideRight
                                    opacity: 0.7
                                    font.pixelSize: 13
                                }
                            }

                        }

                        RowActs {
                            id: contactRowActs

                            anchors.right: parent.right
                            anchors.rightMargin: 16
                            anchors.verticalCenter: parent.verticalCenter
                            visible: contactRow.showsControls
                            over: contactRow.hovered
                            picked: contactRow.highlighted

                            RowButton {
                                id: editButton

                                glyphs: contactsTab.glyphs
                                glyph: "edit"

                                text: qsTr("Edit")

                                onClicked: contacts.editing
                                           && contacts.editingId === modelData.id
                                           ? contacts.cancelEdit()
                                           : contacts.edit(modelData.id)
                            }

                            RowButton {
                                glyphs: contactsTab.glyphs
                                glyph: "call"
                                text: qsTr("Call")
                                onClicked: contactsTab.dial(() => contacts.dial(modelData.id))
                            }
                        }
                    }

                    Loader {
                        id: cardHere

                        property bool placed: false

                        width: contactEntry.width

                        height: cardHere.item ? cardHere.item.implicitHeight : 0

                        active: contacts.editing
                                && contacts.editingId === contactEntry.modelData.id
                        sourceComponent: contactsTab.contactCard

                        onLoaded: cardHere.placed = false

                        onHeightChanged: {
                            if (!cardHere.placed && cardHere.height > 0) {
                                cardHere.placed = true
                                list.positionViewAtIndex(contactEntry.index, ListView.Contain)
                            }
                        }
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                width: parent.width - 32
                visible: text !== ""
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                opacity: 0.7
                text: contactsTab.emptyText()
            }

        }

        Connections {
            target: contacts

            function onDialRefused(reason) {
                contactsTab.dialRefused(reason)
            }
        }
    }
}
