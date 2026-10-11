// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/capture/plugin.hpp>
#include <softphone/plugins/capture/ui.hpp>

#include <softphone/shell/ui_loading.hpp>
#include <softphone/shell/windows.hpp>

#include <QCoreApplication>
#include <QString>
#include <QUrl>

namespace softphone::plugins::capture {
namespace {

constexpr auto kContext = "Capture";

const auto kComponents = QStringLiteral("qrc:/qt/qml/Softphone/Capture/qml/");

}

void contributeInterface(shell::UiRegistry &registry)
{
    const auto owner = QString::fromUtf8(kOwner.data(), static_cast<qsizetype>(kOwner.size()));

    registry.addSettingsSection(
        "capture", shell::settingsBand::kTelephone, 60, owner,
        [] {

            return QCoreApplication::translate(kContext, QT_TRANSLATE_NOOP("Capture", "Capture"));
        },
        QUrl{kComponents + QStringLiteral("SettingsCapture.qml")});

    registry.addSlotItem(QStringLiteral("footer"), QStringLiteral("capture.strip"), 10, owner,
                         QUrl{kComponents + QStringLiteral("CaptureStrip.qml")});

    registry.addSlotItem(QStringLiteral("primary"), QStringLiteral("capture.page"), 20, owner,
                         QUrl{kComponents + QStringLiteral("CapturePage.qml")});

    registry.addSlotItem(QString::fromUtf8(shell::kBarExtrasSlot), QStringLiteral("capture.bar"),
                         10, owner, QUrl{kComponents + QStringLiteral("CaptureBar.qml")});

    registry.addCatalogue(owner, QStringLiteral(":/i18n/capture"), QStringLiteral("capture_"));
}

}

SOFTPHONE_PLUGIN_UI("capture", ::softphone::plugins::capture::contributeInterface)
