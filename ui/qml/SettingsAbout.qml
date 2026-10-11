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
        text: qsTr("AI Softphone")
        font.pixelSize: 20
        font.weight: Font.Medium
    }

    Label {
        Layout.fillWidth: true

        text: qsTr("Version %1").arg(about.version)
        opacity: 0.7
        wrapMode: Text.WordWrap
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        Label {
            id: websiteLabel

            Layout.fillWidth: true

            textFormat: Text.StyledText
            wrapMode: Text.WordWrap

            color: Qt.rgba(Material.foreground.r, Material.foreground.g,
                           Material.foreground.b, 0.7)

            linkColor: page.linkColour

            text: qsTr("Website: %1")
                  .arg("<a href=\"" + about.website + "\">"
                       + about.website.replace("https://", "") + "</a>")
            onLinkActivated: link => Qt.openUrlExternally(link)

            HoverHandler {
                cursorShape: websiteLabel.hoveredLink ? Qt.PointingHandCursor
                                                      : Qt.ArrowCursor
            }
        }

        Label {
            id: docsLabel

            Layout.fillWidth: true

            textFormat: Text.StyledText
            wrapMode: Text.WordWrap

            color: Qt.rgba(Material.foreground.r, Material.foreground.g,
                           Material.foreground.b, 0.7)
            linkColor: page.linkColour

            text: qsTr("Documentation: %1")
                  .arg("<a href=\"" + about.documentationPage + "\">"
                       + about.documentation.replace("https://", "") + "</a>")
            onLinkActivated: link => Qt.openUrlExternally(link)

            HoverHandler {
                cursorShape: docsLabel.hoveredLink ? Qt.PointingHandCursor
                                                   : Qt.ArrowCursor
            }
        }
    }

    LabelledPicker {
        id: countryPicker

        Layout.topMargin: 8

        readonly property var entries: [{ code: "", name: "" }]
            .concat(about.countries)

        function labelOf(entry) {
            void Qt.uiLanguage

            return entry === undefined || entry.code === ""
                ? qsTr("Not specified") : entry.name
        }

        function markTheOpenOne() {
            for (let i = 0; i < entries.length; ++i) {
                if (entries[i].code === about.country) {
                    currentIndex = i
                    return
                }
            }
            currentIndex = 0
        }

        Layout.fillWidth: true
        placeholderText: qsTr("Country")
        model: entries

        textRole: "name"

        displayText: labelOf(entries[currentIndex])

        delegate: ItemDelegate {
            required property var modelData
            required property int index

            width: countryPicker.width
            text: countryPicker.labelOf(modelData)
            highlighted: countryPicker.highlightedIndex === index
        }

        Component.onCompleted: markTheOpenOne()
        onCountChanged: Qt.callLater(markTheOpenOne)
        onActivated: (chosen) => about.chooseCountry(
                         countryPicker.entries[chosen].code)

        Connections {
            target: about
            function onCountryChanged() { Qt.callLater(countryPicker.markTheOpenOne) }
            function onCountriesChanged() { Qt.callLater(countryPicker.markTheOpenOne) }
        }
    }

    Button {
        Layout.fillWidth: true
        text: qsTr("Detect automatically")
        onClicked: about.detectCountry()
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("This helps us pick the best update server, and opens the way to language and speech services hosted in your country.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 8
        text: qsTr("Updates")
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        opacity: 0.7
        text: {
            switch (updates.state) {
            case UpdateViewModel.Unconfigured:
                return qsTr("This build has no update address, so it never asks. The builds published on the website have one.")
            case UpdateViewModel.Checking:
                return qsTr("Looking…")
            case UpdateViewModel.UpToDate:
                return qsTr("You have the newest version.")
            case UpdateViewModel.Available:
                return updates.selfReplacing
                    ? qsTr("Version %1 is available. Nothing is downloaded until you ask.").arg(updates.offered)
                    : qsTr("Version %1 is available. This copy was installed by a package, so the new one is installed the same way.").arg(updates.offered)
            case UpdateViewModel.Downloading:
                return qsTr("Downloading version %1…").arg(updates.offered)
            case UpdateViewModel.Ready:
                return qsTr("Version %1 is ready. The phone will close and start again.").arg(updates.offered)
            case UpdateViewModel.Failed:
                return qsTr("No update server answered.")
            default:
                return qsTr("Not checked yet.")
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: updates.state === UpdateViewModel.Failed
                 && updates.networkReachable
                 && about.country === ""
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
        text: qsTr("This computer is online, so the servers themselves could not be reached. Saying which country this phone is in above may help: there may be one nearer you.")
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        opacity: 0.7
        font.pixelSize: 12
        visible: updates.lastChecked !== ""
        text: qsTr("Last checked %1").arg(updates.lastChecked)
    }

    ProgressBar {
        Layout.fillWidth: true
        visible: updates.state === UpdateViewModel.Downloading

        indeterminate: updates.progress < 0
        value: updates.progress < 0 ? 0 : updates.progress
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 8

        Button {
            Layout.fillWidth: true
            text: qsTr("Check for updates")
            enabled: updates.state !== UpdateViewModel.Unconfigured
                     && updates.state !== UpdateViewModel.Checking
                     && updates.state !== UpdateViewModel.Downloading
            onClicked: updates.check()
        }

        Button {
            Layout.fillWidth: true
            visible: updates.state === UpdateViewModel.Available
                     && updates.selfReplacing
            text: qsTr("Download")
            onClicked: updates.fetch()
        }

        Button {
            Layout.fillWidth: true
            visible: updates.state === UpdateViewModel.Available
                     && !updates.selfReplacing && updates.hasDownload
            text: qsTr("Get the package")
            onClicked: updates.openDownload()
        }

        Button {
            Layout.fillWidth: true
            visible: updates.state === UpdateViewModel.Ready
            text: qsTr("Install and restart")
            highlighted: true
            onClicked: updates.install()
        }

        Label {
            Layout.fillWidth: true
            visible: updates.heldBack && updates.state === UpdateViewModel.Ready

            text: qsTr("The new version is downloaded. Installing it restarts the phone, so it "
                       + "is waiting for you: press the button above when nothing is in "
                       + "progress.")
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            opacity: 0.7
        }

        Button {
            Layout.fillWidth: true
            visible: updates.hasNotes
                     && (updates.state === UpdateViewModel.Available
                         || updates.state === UpdateViewModel.Ready)
            flat: true
            text: qsTr("What changed")
            onClicked: updates.openNotes()
        }
    }

    Label {
        Layout.fillWidth: true
        visible: updates.state === UpdateViewModel.Failed
                 && updates.complaint.length > 0
        text: updates.complaint
        wrapMode: Text.WordWrap
        font.family: "monospace"
        font.pixelSize: 12
        opacity: 0.55
    }

    CheckBox {
        Layout.fillWidth: true
        padding: 0
        implicitHeight: 34
        enabled: updates.state !== UpdateViewModel.Unconfigured

        objectName: "updatesTick"
        text: qsTr("Look for updates automatically")
        checked: settings.automaticUpdates
        onToggled: settings.setAutomaticUpdates(checked)
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Once a day, and shortly after the phone starts. It asks a server for one small file and nothing is downloaded or installed without you saying so.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 8
        text: qsTr("Licence")
        font.pixelSize: 15
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("This program is free software under the %1. It comes with no warranty, and you may redistribute it under that licence's terms — the full text ships in the file named LICENSE.").arg(about.licence)
        wrapMode: Text.WordWrap
        opacity: 0.7
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 8

        text: qsTr("Telemetry")
        font.pixelSize: 15
        font.bold: true
    }

    TelemetryChoice {
        Layout.fillWidth: true
    }

    Button {
        Layout.fillWidth: true
        visible: telemetry.fromSource
        enabled: !telemetry.reporting
        text: telemetry.reporting

              ? qsTr("Sending a report…")

              : qsTr("Send telemetry now")
        onClicked: telemetry.sendNow()
    }

    Label {
        Layout.fillWidth: true
        visible: !telemetry.reportsOnItsOwn

        text: qsTr("This build sends no report of its own; the builds published on the website do. A message you write below is sent whatever the build.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.7
    }

    Label {
        Layout.fillWidth: true
        visible: telemetry.fromSource && telemetry.reportOutcome.length > 0
        text: telemetry.reportOutcome
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        font.family: "monospace"
        opacity: 0.7
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 8

        text: qsTr("Feedback")
        font.pixelSize: 15
        font.bold: true
    }

    TextField {
        id: feedbackSubject

        objectName: "feedbackBox"

        TextMenu {}

        Layout.fillWidth: true
        enabled: !telemetry.sending

        placeholderText: qsTr("Subject")
    }

    TextArea {
        id: feedbackMessage

        TextMenu {}

        Layout.fillWidth: true
        Layout.minimumHeight: 96
        enabled: !telemetry.sending
        wrapMode: TextArea.Wrap

        placeholderText: qsTr("Message")
    }

    TextField {
        id: feedbackName

        TextMenu {}

        Layout.fillWidth: true
        enabled: !telemetry.sending
        placeholderText: qsTr("Your name")
    }

    TextField {
        id: feedbackEmail

        TextMenu {}

        Layout.fillWidth: true
        enabled: !telemetry.sending

        placeholderText: qsTr("Address for a reply")
    }

    Label {
        Layout.fillWidth: true

        text: qsTr("Both are optional. Without an address there is no way to answer.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    CheckBox {
        id: feedbackAttach

        Layout.fillWidth: true
        padding: 0
        implicitHeight: 34
        visible: telemetry.attachments.length > 0
        enabled: !telemetry.sending

        text: qsTr("Attach the log")
    }

    Label {
        Layout.fillWidth: true
        visible: feedbackAttach.visible
        text: telemetry.attachments
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Button {
        Layout.fillWidth: true
        enabled: !telemetry.sending && feedbackMessage.text.trim().length > 0

        text: telemetry.sending ? qsTr("Sending…") : qsTr("Send")
        onClicked: telemetry.sendFeedback(feedbackSubject.text, feedbackMessage.text,
                                          feedbackName.text, feedbackEmail.text,
                                          feedbackAttach.checked)
    }

    Label {
        Layout.fillWidth: true
        visible: telemetry.outcome.length > 0
        text: telemetry.outcome
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        color: telemetry.delivered ? Ink.good(Material.theme) : Ink.bad(Material.theme)
    }

    Connections {
        target: telemetry

        function onFeedbackChanged() {
            if (telemetry.delivered) {
                feedbackSubject.text = ""
                feedbackMessage.text = ""
            }
        }
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 8
        text: qsTr("Built with")
        font.pixelSize: 15
        font.bold: true
    }

    Repeater {
        model: about.components

        RowLayout {
            id: componentRow
            required property var modelData

            Layout.fillWidth: true
            spacing: 8

            Label {
                text: componentRow.modelData.name
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: componentRow.modelData.licence
                opacity: 0.7
                font.family: "monospace"
                font.pixelSize: 12
            }
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Each is used under the licence beside it. Where a component offers several, the one named is the one taken.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Item { Layout.fillHeight: true }
}
