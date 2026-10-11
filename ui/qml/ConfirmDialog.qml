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
import QtQuick.Window

Dialog {
    id: ask

    popupType: ask.inAStrip ? Popup.Window : Popup.Item

    required property string actionText

    property bool destructive: true

    property bool actionEnabled: true

    default property alias body: says.data

    width: Math.min(Overlay.overlay.width - 48, 380)

    anchors.centerIn: ask.inAStrip ? undefined : Overlay.overlay

    readonly property Window hostWindow: ask.parent !== null ? ask.parent.Window.window : null

    readonly property bool inAStrip: ask.hostWindow !== null
                                     && ask.hostWindow.height < ask.implicitHeight + 48

    onAboutToShow: ask.putItSomewhere()
    onWidthChanged: Qt.callLater(ask.putItSomewhere)
    onHeightChanged: Qt.callLater(ask.putItSomewhere)

    function putItSomewhere() {
        if (!ask.inAStrip) {
            return
        }

        InAStrip.centre(ask, Overlay.overlay)
    }

    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    standardButtons: Dialog.Cancel

    bottomPadding: 0

    Component.onCompleted: standardButton(Dialog.Cancel).text =
        Qt.binding(() => qsTranslate("QPlatformTheme", "Cancel"))

    contentItem: ColumnLayout {
        spacing: 12

        ColumnLayout {
            id: says

            Layout.fillWidth: true
            spacing: 12
        }

        Button {
            Layout.fillWidth: true
            highlighted: true
            enabled: ask.actionEnabled
            Material.background: ask.destructive ? Mark.bad
                                                 : ask.Material.accentColor
            text: ask.actionText
            onClicked: ask.accept()
        }
    }
}
