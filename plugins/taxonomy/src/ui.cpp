// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/taxonomy/plugin.hpp>
#include <softphone/plugins/taxonomy/ui.hpp>

#include <softphone/shell/ui_loading.hpp>

#include <QCoreApplication>
#include <QString>
#include <QUrl>

namespace softphone::plugins::taxonomy {
namespace {

constexpr auto kContext = "Taxonomy";

const auto kComponents = QStringLiteral("qrc:/qt/qml/Softphone/Taxonomy/qml/");

}

void contributeInterface(shell::UiRegistry &registry)
{
    const auto owner = QString::fromUtf8(kOwner.data(), static_cast<qsizetype>(kOwner.size()));

    registry.addSettingsSection(
        "dictionaries", shell::settingsBand::kProcessing, 30, owner,
        [] {
            return QCoreApplication::translate(kContext,
                                               QT_TRANSLATE_NOOP("Taxonomy", "Dictionaries"));
        },
        QUrl{kComponents + QStringLiteral("SettingsDictionaries.qml")});

    registry.addCatalogue(owner, QStringLiteral(":/i18n/taxonomy"), QStringLiteral("taxonomy_"));
}

}

SOFTPHONE_PLUGIN_UI("taxonomy", ::softphone::plugins::taxonomy::contributeInterface)
