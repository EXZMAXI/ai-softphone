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
    id: zones

    required property var page

    property bool active: false

    property var tabsOffered: []

    readonly property int offerMoved:
        uiRegistry.slotRevision + uiRegistry.revision + mainZone.bodiesRevision

    onOfferMovedChanged: zones.refreshTabsOffered()
    Component.onCompleted: zones.refreshTabsOffered()

    Connections {
        target: uiRegistry

        function onWindowsChanged() {
            zones.refreshTabsOffered()
        }
    }

    function sameOffer(one, other) {
        if (one.length !== other.length) {
            return false
        }
        for (let i = 0; i < one.length; ++i) {
            if (one[i].window !== other[i].window || one[i].tab !== other[i].tab
                || one[i].key !== other[i].key || one[i].label !== other[i].label
                || one[i].glyph !== other[i].glyph) {
                return false
            }
        }
        return true
    }

    function refreshTabsOffered() {
        const wanted = zones.theOffer()
        if (!zones.sameOffer(wanted, zones.tabsOffered)) {
            zones.tabsOffered = wanted
        }
    }

    function theOffer() {
        const out = []
        for (const id of uiRegistry.windows) {
            if (zones.page.zoneFor(id) !== "main") {
                continue
            }

            const body = mainZone.bodies[id]
            const known = uiRegistry.componentOfWindow(id) == ""
                          ? uiRegistry.itemsIn(id + ".tabs") : []
            const tabs = known.length > 0 ? known : (body ? body.tabsHere : [])
            if (tabs.length === 0) {

                const named = uiRegistry.windowGlyph(id)
                out.push({ window: id, tab: "", key: id,
                           label: uiRegistry.windowTitle(id),
                           glyph: named !== "" ? named : (body ? body.drawing : "") })
                continue
            }
            for (const tab of tabs) {
                const said = uiRegistry.itemTitle(tab)
                const named = said !== "" ? said : (body ? body.titleOfTab(tab) : "")
                const drawn = uiRegistry.itemGlyph(tab)
                out.push({ window: id, tab: tab, key: tab,
                           label: named !== "" ? named : uiRegistry.windowTitle(id),
                           glyph: drawn !== "" ? drawn
                                               : (body ? body.glyphOfTab(tab) : "") })
            }
        }
        out.push({ window: "settings", tab: "", key: "settings",
                   label: zones.page.settingsName, glyph: "settings" })
        return out
    }

    readonly property string showing: mainZone.showing

    function show(key) {
        mainZone.show(key)
    }

    function reveal(key, control) {
        zones.show(key)
        if (control === undefined || control === "") {
            return
        }
        const which = mainZone.bodies[mainZone.showingWindow]
        if (which !== undefined && which !== null) {
            which.reveal(control)
        }
    }

    function closeTab(key) {
        mainZone.closeTab(key)
    }

    readonly property bool anythingOpen: mainZone.here.length > 0

    readonly property bool ownWayIn: !zones.page.zonesInside && zones.page.isTheBar

    function openSettingsAt(section) {
        settingsBody.section = section
    }

    Item {
        id: mainZone

        anchors.fill: parent

        readonly property var here: {
            void mainZone.openedRevision
            void mainZone.offerRevision

            const out = []
            for (const key of mainZone.opened) {
                for (const one of zones.tabsOffered) {
                    if (one.key === key) {
                        out.push(one)
                        break
                    }
                }
            }
            return out
        }

        property int offerRevision: 0

        property var bodies: ({})
        property int bodiesRevision: 0

        property var opened: []
        property int openedRevision: 0

        function closeTab(key) {
            const at = mainZone.opened.indexOf(key)
            if (at < 0) {
                return
            }
            const out = mainZone.opened.slice()
            out.splice(at, 1)
            mainZone.opened = out
            mainZone.openedRevision += 1
        }

        function openTab(key) {
            if (mainZone.opened.indexOf(key) >= 0) {
                return
            }
            const out = mainZone.opened.slice()
            out.push(key)
            mainZone.opened = out
            mainZone.openedRevision += 1
        }

        function entryFor(key) {
            for (const one of mainZone.here) {
                if (one.key === key) {
                    return one
                }
            }
            return null
        }

        property string showing: ""

        property string showingWindow: ""

        function show(key) {

            for (const one of zones.tabsOffered) {
                if (one.key === key) {
                    mainZone.openTab(key)
                    mainZone.showTheEntry(one)
                    return
                }
            }
            for (const one of zones.tabsOffered) {
                if (one.window === key) {
                    mainZone.openTab(one.key)
                    mainZone.showTheEntry(one)
                    return
                }
            }
        }

        function showTheEntry(entry) {
            mainZone.showing = entry.key
            mainZone.showingWindow = entry.window
        }

        function chooseOneThatExists() {
            const one = mainZone.entryFor(mainZone.showing)
            if (one !== null) {
                mainZone.showingWindow = one.window
                return
            }
            if (mainZone.here.length > 0) {
                mainZone.showTheEntry(mainZone.here[0])
            } else {
                mainZone.showing = ""
                mainZone.showingWindow = ""
            }
        }

        function openTheFirstOne() {

            if (!(zones.page.zonesInside || zones.ownWayIn)
                || mainZone.opened.length > 0 || zones.tabsOffered.length === 0) {
                return
            }
            mainZone.show(zones.tabsOffered[0].key)
        }

        onHereChanged: mainZone.chooseOneThatExists()

        Component.onCompleted: {
            mainZone.chooseOneThatExists()
            Qt.callLater(mainZone.openTheFirstOne)
        }

        Connections {
            target: zones

            function onTabsOfferedChanged() { mainZone.offerRevision += 1 }
        }

        Connections {
            target: zones

            function onTabsOfferedChanged() { Qt.callLater(mainZone.openTheFirstOne) }
        }

        Rectangle {
            visible: zones.page.zonesInside
            anchors.left: parent.left
            width: 1
            height: parent.height
            color: Qt.rgba(Material.foreground.r, Material.foreground.g,
                           Material.foreground.b, 0.1)
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: zones.page.zonesInside ? 1 : 0
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: mainTabs.implicitHeight

                visible: mainZone.here.length > 1 || zones.ownWayIn

                color: Qt.rgba(Material.foreground.r, Material.foreground.g,
                               Material.foreground.b, 0.045)

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 1
                    color: Qt.rgba(Material.foreground.r, Material.foreground.g,
                                   Material.foreground.b, 0.1)
                }

                TabBar {
                    id: mainTabs

                    anchors.fill: parent

                    background: null

                    Repeater {
                        model: mainZone.here

                        TabButton {
                            id: oneTab

                            required property var modelData

                            text: oneTab.modelData.label

                            width: implicitWidth

                            rightPadding: 30

                            onClicked: mainZone.showTheEntry(oneTab.modelData)

                            ToolButton {
                                id: closeTab

                                anchors.right: parent.right
                                anchors.rightMargin: 6
                                anchors.verticalCenter: parent.verticalCenter

                                visible: (oneTab.hovered || closeTab.hovered)
                                         && mainZone.here.length > 1

                                implicitWidth: 22
                                implicitHeight: 22
                                padding: 0

                                icon.source: settings.glyphSet + "/dismiss.svg"
                                icon.width: 14
                                icon.height: 14

                                Accessible.name: qsTr("Close", "one tab")

                                onClicked: mainZone.closeTab(oneTab.modelData.key)

                                background: Rectangle {
                                    radius: height / 2
                                    color: closeTab.hovered
                                           ? Qt.rgba(Material.foreground.r,
                                                     Material.foreground.g,
                                                     Material.foreground.b, 0.14)
                                           : "transparent"
                                }
                            }
                        }
                    }

                    ToolButton {
                        id: addTab

                        parent: mainTabs
                        visible: zones.ownWayIn

                        anchors.right: parent.right
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter

                        implicitWidth: 26
                        implicitHeight: 26
                        padding: 0

                        icon.source: settings.glyphSet + "/more-vertical.svg"
                        icon.width: 16
                        icon.height: 16

                        Accessible.name: qsTr("Open a section")

                        WaitingDot {
                            shown: zones.page.waysInMarked
                        }

                        onClicked: {
                            const rows = []

                            for (const one of zones.tabsOffered.filter(
                                     tab => tab.window !== "diagnostics")) {
                                rows.push({
                                    label: one.label,
                                    offered: true,
                                    key: one.key,
                                    picture: one.glyph === ""
                                             ? "" : zones.page.glyph(one.glyph),

                                    marked: uiRegistry.marked(one.key)
                                })
                            }
                            addEntries.model = rows
                            addMenu.present(addTab)
                        }

                        background: Rectangle {
                            radius: height / 2
                            color: addTab.hovered
                                   ? Qt.rgba(Material.foreground.r, Material.foreground.g,
                                             Material.foreground.b, 0.10)
                                   : "transparent"
                        }
                    }

                    RightMenu {
                        id: addMenu

                        host: addTab

                        Repeater {
                            id: addEntries

                            model: []

                            TextMenuItem {
                                required property var modelData

                                text: modelData.label
                                picture: modelData.picture
                                marked: modelData.marked

                                onTriggered: mainZone.show(modelData.key)
                            }
                        }
                    }

                    function markTheOpenOne() {
                        if (mainZone.here.length === 0) {
                            return
                        }
                        let at = 0
                        for (let i = 0; i < mainZone.here.length; ++i) {
                            if (mainZone.here[i].key === mainZone.showing) {
                                at = i
                                break
                            }
                        }
                        mainTabs.currentIndex = at
                    }

                    onCountChanged: Qt.callLater(mainTabs.markTheOpenOne)
                    Component.onCompleted: Qt.callLater(mainTabs.markTheOpenOne)

                    Connections {
                        target: mainZone

                        function onHereChanged() { Qt.callLater(mainTabs.markTheOpenOne) }
                        function onShowingChanged() { Qt.callLater(mainTabs.markTheOpenOne) }
                    }
                }
            }

            Item {
                id: mainBody

                Layout.fillWidth: true
                Layout.fillHeight: true

                Repeater {
                    model: {
                        void uiRegistry.slotRevision
                        const out = []
                        for (const id of uiRegistry.windows) {
                            if (zones.page.zoneFor(id) === "main") {
                                out.push(id)
                            }
                        }
                        return out
                    }

                    WindowBody {
                        id: oneBody

                        required property string modelData

                        anchors.fill: parent

                        windowId: modelData
                        visible: mainZone.showingWindow === modelData

                        ownStrip: false

                        glyphs: zones.page.glyphs
                        onScreen: zones.page.onScreen && oneBody.visible

                        kept: oneBody.visible && zones.page.visible
                        settling: false
                        focused: zones.active && oneBody.visible

                        onRecordingsRequested: (callId) => {
                            recordings.showCallsOf(callId, true)
                            mainZone.show("recordings")
                        }

                        Connections {
                            target: mainZone

                            function onShowingChanged() { oneBody.followTheStrip() }
                        }

                        function followTheStrip() {
                            const one = mainZone.entryFor(mainZone.showing)
                            if (one !== null && one.window === oneBody.windowId && one.tab !== "") {
                                oneBody.showSection(one.tab)
                            }
                        }

                        Component.onCompleted: {
                            mainZone.bodies[oneBody.windowId] = oneBody
                            mainZone.bodiesRevision += 1
                        }

                        onItemChanged: mainZone.bodiesRevision += 1

                        onTabsHereChanged: {
                            mainZone.bodiesRevision += 1
                            Qt.callLater(mainZone.chooseOneThatExists)
                            Qt.callLater(oneBody.followTheStrip)
                        }
                    }
                }

                SettingsBody {
                    id: settingsBody

                    anchors.fill: parent

                    visible: mainZone.showingWindow === "settings"

                    glyphs: zones.page.glyphs
                    onScreen: zones.page.onScreen && settingsBody.visible
                    kept: settingsBody.visible
                    settling: false
                    focused: zones.active && settingsBody.visible

                    registrationProblem: zones.page.settingsProblem
                    registrationSummary: zones.page.settingsSummary

                    trayAvailable: zones.page.trayAvailable
                    trayShown: zones.page.trayShown

                    onDiagnosticsRequested: mainZone.show("diagnostics")
                    onApiPageRequested: (address) => Qt.openUrlExternally(address)
                    onWindowRequested: (id, control) => zones.page.showPanelNamed(id, control)
                    onPhoneRequested: zones.page.showTheWindow()
                }

                Label {
                    anchors.centerIn: parent
                    visible: mainZone.here.length === 0

                    text: {
                        void Qt.uiLanguage

                        if (zones.page.zonesInside) {

                            return qsTr("Nothing is open. Choose a section from the list under the phone.")
                        }
                        if (zones.ownWayIn) {

                            return qsTr("Nothing is open. Choose a section from the button at the end of the tabs.")
                        }
                        if (zones.page.hasNoWindow) {

                            return qsTr("Nothing is open. Choose a section from the phone's menu in the notification area.")
                        }

                        return qsTr("Nothing is open. Open a window from the buttons on the phone.")
                    }
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    width: Math.min(360, parent.width - 48)
                    opacity: 0.6
                }
            }
        }
    }
}
