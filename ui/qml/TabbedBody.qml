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

    property string windowId: ""

    implicitWidth: 460

    property int leastWidth: 380

    property string glyphs: ""
    property bool onScreen: false
    property bool kept: false
    property bool settling: false
    property bool focused: false

    property bool ownStrip: true

    function titleOfTab(id) {
        void panel.tabPanelsRevision
        const showing = panel.tabPanels[id]
        return showing ? showing.title : ""
    }

    function glyphOfTab(id) {
        void panel.tabPanelsRevision
        const showing = panel.tabPanels[id]
        return showing && showing.drawing !== undefined ? showing.drawing : ""
    }

    readonly property string windowTitle: panel.titleNow

    signal recordingsRequested(string callId)

    property string section: ""

    readonly property var tabIds: {
        void uiRegistry.slotRevision
        return uiRegistry.itemsIn(panel.windowId + ".tabs")
    }

    property var tabPanels: ({})

    property int tabPanelsRevision: 0

    readonly property string titleNow: {
        void uiRegistry.revision
        void panel.tabPanelsRevision

        const name = uiRegistry.windowTitle(panel.windowId)
        const showing = panel.tabPanels[panel.section]
        const tab = showing ? showing.title : ""

        if (tab === "" || panel.tabIds.length < 2) {
            return tab === "" ? name : tab
        }
        return qsTr("%1 · %2").arg(name).arg(tab)
    }

    property string bookSection: ""

    function showSection(which) {
        panel.section = which
        panel.sectionShown()
    }

    signal sectionShown

    onTabIdsChanged: panel.chooseATabThatExists()

    Component.onCompleted: panel.chooseATabThatExists()

    function chooseATabThatExists() {
        if (panel.tabIds.indexOf(panel.section) < 0) {
            panel.section = panel.tabIds.length > 0 ? panel.tabIds[0] : ""
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Keys.onEscapePressed: (event) => {
            panel.forceActiveFocus()
            event.accepted = true
        }

        TabBar {
            id: tabs

            Layout.fillWidth: true

            visible: panel.ownStrip && panel.tabIds.length > 1

            currentIndex: Math.max(0, panel.tabIds.indexOf(panel.section))

            onCurrentIndexChanged: {
                const wanted = panel.tabIds[tabs.currentIndex] ?? ""
                if (wanted !== "" && wanted !== panel.section) {
                    panel.section = wanted
                }
            }

            background: Rectangle {
                color: Material.foreground
                opacity: 0.04
            }

            Repeater {
                model: panel.tabIds

                TabButton {
                    required property string modelData

                    text: {
                        void panel.tabPanelsRevision
                        const one = panel.tabPanels[modelData]
                        return one ? one.title : ""
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Layout.topMargin: tabs.visible ? 0 : 12

            Repeater {
                model: panel.tabIds

                Loader {
                    id: tabLoader

                    required property string modelData

                    anchors.fill: parent
                    visible: tabLoader.item !== null && panel.section === tabLoader.modelData

                    source: {
                        void uiRegistry.slotRevision
                        return uiRegistry.componentOfItem(tabLoader.modelData)
                    }

                    onLoaded: {
                        tabLoader.item.glyphs = Qt.binding(() => panel.glyphs)
                        tabLoader.item.chosen =
                            Qt.binding(() => panel.section === tabLoader.modelData)
                        tabLoader.item.dial = panel.dialOrSayWhyNot

                        if (contacts.available && ("addContact" in tabLoader.item)) {

                            tabLoader.item.addContact = (number, name) => {
                                if (panel.bookSection !== "") {
                                    panel.section = panel.bookSection
                                }
                                contacts.addContact(number, name)
                            }
                        }

                        if (tabLoader.item.isTheBook !== undefined) {
                            panel.bookSection = tabLoader.modelData
                        }

                        if (tabLoader.item.recordingsRequested !== undefined) {
                            tabLoader.item.recordingsRequested.connect(panel.recordingsRequested)
                        }

                        if (tabLoader.item.dialRefused !== undefined) {
                            tabLoader.item.dialRefused.connect((reason) => panel.refuse(reason))
                        }

                        panel.tabPanels[tabLoader.modelData] = tabLoader.item
                        panel.tabPanelsRevision += 1
                    }

                    onStatusChanged: {
                        if (tabLoader.status === Loader.Error) {
                            console.warn("the tab", tabLoader.modelData,
                                         "did not load:", tabLoader.source)
                        }
                    }
                }
            }
        }

        Label {
            id: refusal

            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            property int reason: -1

            visible: refusal.reason >= 0
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 13
            color: Ink.warn(Material.theme)

            text: {
                switch (refusal.reason) {
                case Dialling.Refusal.NotConnected:
                    return qsTr("No line is registered yet.")
                case Dialling.Refusal.NoLine:
                    return qsTr("No account is switched on, so there is no line to call from.")
                case Dialling.Refusal.NothingToDial:
                    return qsTr("This row has no number to call back.")
                case Dialling.Refusal.TooManyCalls:
                    return qsTr("As many calls are up as this phone will carry.")
                case Dialling.Refusal.Vanished:
                    return qsTr("The account this would have called from is no longer there.")
                default:
                    return ""
                }
            }

            function say(reason) {
                refusal.reason = reason
                forgetting.restart()
            }

            Component.onCompleted: panel.refuse = refusal.say

            Timer {
                id: forgetting

                interval: 8000
                onTriggered: refusal.reason = -1
            }

            Connections {
                target: panel

                function onSectionChanged() {
                    refusal.reason = -1
                }
            }
        }

        Item {
            Layout.preferredHeight: 16
        }
    }

    function dialOrSayWhyNot(dial) {
        if (!accounts.connected) {
            panel.refuse(Dialling.Refusal.NotConnected)
            return
        }
        dial()
    }

    property var refuse: function (reason) {}
}
