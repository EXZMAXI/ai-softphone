// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/blf/plugin.hpp>
#include <softphone/plugins/blf/ui.hpp>

#include <softphone/shell/ui_loading.hpp>

#include <QCoreApplication>
#include <QString>
#include <QUrl>

namespace softphone::plugins::blf {
namespace {

constexpr auto kContext = "Blf";

const auto kComponents = QStringLiteral("qrc:/qt/qml/Softphone/Blf/qml/");

}

void contributeInterface(shell::UiRegistry &registry)
{
    const auto owner = QString::fromUtf8(kOwner.data(), static_cast<qsizetype>(kOwner.size()));

    registry.addSettingsSection(
        "buttons", shell::settingsBand::kTelephone, 40, owner,
        [] { return QCoreApplication::translate(kContext, QT_TRANSLATE_NOOP("Blf", "Buttons")); },
        QUrl{kComponents + QStringLiteral("SettingsButtons.qml")});

    registry.addSlotItem("side", "keys", 10, owner,
                         QUrl{kComponents + QStringLiteral("KeyPanel.qml")});

    registry.addCatalogue(owner, QStringLiteral(":/i18n/blf"), QStringLiteral("blf_"));
}

}

SOFTPHONE_PLUGIN_UI("blf", ::softphone::plugins::blf::contributeInterface)
