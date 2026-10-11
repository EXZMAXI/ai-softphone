// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#pragma once

#include <softphone/shell/ui_registry.hpp>

#include <QString>

#include <QStringList>
#include <span>
#include <string>

class QCoreApplication;

namespace softphone::ui {

void registerMonolithCatalogue(shell::UiRegistry &registry);

QString installTranslations(QCoreApplication &application,
                            std::span<const shell::UiRegistry::Catalogue> catalogues,
                            const QString &requested = {});

QString installTranslations(QCoreApplication &application, const shell::UiRegistry &registry,
                            const QString &requested = {});

[[nodiscard]] std::string contentLanguage(const QString &requested);

[[nodiscard]] QString interfaceLanguage(const QString &requested);

[[nodiscard]] QStringList
availableLanguages(std::span<const shell::UiRegistry::Catalogue> catalogues);

[[nodiscard]] QStringList availableLanguages(const shell::UiRegistry &registry);

[[nodiscard]] QString offeredLanguage(std::span<const shell::UiRegistry::Catalogue> catalogues,
                                      const QString &stored);

[[nodiscard]] QString offeredLanguage(const shell::UiRegistry &registry, const QString &stored);

[[nodiscard]] QString languageEndonym(const QString &code);

}
