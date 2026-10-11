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
        text: qsTr("Webhooks")
        font.pixelSize: 15
        font.bold: true
    }

    CheckBox {
        Layout.fillWidth: true
        text: qsTr("Tell another system about calls")
        checked: settings.webhooksEnabled
        onToggled: settings.setWebhooksEnabled(checked)
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("A request is sent for each event you tick below.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    TextField {

        TextMenu {}

        Layout.fillWidth: true
        enabled: settings.webhooksEnabled

        placeholderText: qsTr("Address, for example http://crm.local/calls")
        text: settings.webhookUrl
        onEditingFinished: settings.setWebhookUrl(text)
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        enabled: settings.webhooksEnabled

        Label {
            text: qsTr("Method")
            font.pixelSize: 13
        }

        ListRadioButton {
            text: settings.postName
            checked: settings.webhookPost
            onToggled: if (checked) settings.setWebhookPost(true)
        }

        ListRadioButton {
            text: settings.getName
            checked: !settings.webhookPost
            onToggled: if (checked) settings.setWebhookPost(false)
        }

        Item { Layout.fillWidth: true }
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 4
        text: qsTr("Events")
        font.pixelSize: 13
        font.bold: true
        enabled: settings.webhooksEnabled
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0
        enabled: settings.webhooksEnabled

        Repeater {
            model: uiRegistry.outboundEvents

            delegate: ListCheckBox {
                required property var modelData

                Layout.fillWidth: true

                text: {
                    void uiRegistry.revision
                    return uiRegistry.outboundEventTitle(modelData)
                }
                checked: {
                    void settings.webhookRevision
                    return settings.webhookWants(modelData)
                }
                onToggled: settings.setWebhookEvent(modelData, checked)
            }
        }
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 4
        visible: settings.webhooksEnabled && !settings.webhookPost
        text: qsTr("Address")
        font.pixelSize: 13
        font.bold: true
    }

    TextField {
        id: templateField

        TextMenu {}

        Layout.fillWidth: true
        visible: settings.webhooksEnabled && !settings.webhookPost
        placeholderText: qsTr("Query template")
        text: settings.webhookTemplate
        onEditingFinished: settings.setWebhookTemplate(text)
    }

    Label {
        Layout.fillWidth: true
        visible: templateField.visible
        text: settings.webhookTemplateProblems.length > 0
              ? qsTr("This phone does not know: %1").arg(
                    settings.webhookTemplateProblems.join(", "))
              : qsTr("For example: %1").arg(page.templateExample)
        color: settings.webhookTemplateProblems.length > 0
               ? Ink.bad(Material.theme)
               : Material.foreground
        wrapMode: Text.WrapAnywhere
        font.pixelSize: 12
        opacity: settings.webhookTemplateProblems.length > 0 ? 1.0 : 0.6
    }

    Label {
        Layout.fillWidth: true
        visible: templateField.visible
        text: qsTr("You can use: %1").arg(
                  settings.webhookPlaceholders.join(", "))
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 4
        text: qsTr("Authorization")
        font.pixelSize: 13
        font.bold: true
        enabled: settings.webhooksEnabled
    }

    TextField {

        TextMenu {}

        Layout.fillWidth: true
        enabled: settings.webhooksEnabled
        placeholderText: qsTr("Header name")
        text: settings.webhookHeaderName
        onEditingFinished: settings.setWebhookHeaderName(text)
    }

    TextField {
        id: tokenField

        TextMenu {}

        Layout.fillWidth: true
        enabled: settings.webhooksEnabled
        echoMode: TextInput.Password
        placeholderText: settings.webhookTokenStored
                         ? qsTr("Saved — type to replace it")
                         : qsTr("Header value")
        onEditingFinished: {
            settings.setWebhookToken(text)
            text = ""
        }
    }

    Label {
        Layout.fillWidth: true

        text: page.secretNote()
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        color: page.secretNoteIsPlain() ? Material.foreground : Ink.warn(Material.theme)
        opacity: page.secretNoteIsPlain() ? 0.6 : 1.0
    }

    Button {
        Layout.fillWidth: true
        Layout.topMargin: 8
        enabled: settings.webhooksEnabled && settings.webhookUrl !== ""
        text: qsTr("Send a test event")
        onClicked: settings.sendTestWebhook()
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Sends one event for a call that never happened.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 16
        text: qsTr("Local control")
        font.pixelSize: 15
        font.bold: true
    }

    CheckBox {
        id: apiSwitch

        Layout.fillWidth: true
        text: qsTr("Let other programs on this computer drive the phone")
        checked: settings.apiEnabled
        onToggled: settings.setApiEnabled(checked)

        contentItem: Label {
            text: apiSwitch.text
            font: apiSwitch.font
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignVCenter
            leftPadding: apiSwitch.indicator.width + apiSwitch.spacing
        }
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("A small web interface that answers only to this computer — nothing on the network can reach it. Without a token below, it may read and it may control calls: anything that can run a program here can pick up your telephone. With a token, it can also change what is stored.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    TextField {

        TextMenu {}

        Layout.fillWidth: true
        enabled: settings.apiEnabled
        placeholderText: qsTr("Port")
        text: settings.apiPort > 0 ? String(settings.apiPort) : ""
        inputMethodHints: Qt.ImhDigitsOnly
        validator: IntValidator {
            bottom: 1024
            top: 65535
        }
        onTextEdited: settings.setApiPort(parseInt(text, 10) || 0)
    }

    TextField {
        id: apiTokenField

        TextMenu {}

        Layout.fillWidth: true
        enabled: settings.apiEnabled
        echoMode: TextInput.Password
        placeholderText: settings.apiTokenStored
                         ? qsTr("Saved — type to replace it")
                         : qsTr("Token")
        onEditingFinished: {
            settings.setApiToken(text)
            text = ""
        }
    }

    Label {
        Layout.fillWidth: true

        text: page.secretNote()
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        color: page.secretNoteIsPlain() ? Material.foreground : Ink.warn(Material.theme)
        opacity: page.secretNoteIsPlain() ? 0.6 : 1.0
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 4
        text: qsTr("Access")
        font.pixelSize: 13
        font.bold: true
        enabled: settings.apiEnabled
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0
        enabled: settings.apiEnabled

        Repeater {
            model: uiRegistry.apiResources

            delegate: ListCheckBox {
                required property var modelData

                Layout.fillWidth: true

                text: {
                    void uiRegistry.revision
                    return uiRegistry.apiResourceTitle(modelData)
                }
                checked: {
                    void settings.apiRevision
                    return settings.apiServes(modelData)
                }
                onToggled: settings.setApiResource(modelData, checked)
            }
        }
    }

    Button {
        Layout.fillWidth: true
        Layout.topMargin: 8
        enabled: settings.apiEnabled
        text: qsTr("Open the API's own page")
        onClicked: page.apiPageRequested(settings.apiAddress)
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("The address answers with a list of everything it serves, in English, with links you can follow from a browser.")
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.6
    }

    Item { Layout.fillHeight: true }
}
