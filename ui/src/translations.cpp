// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "translations.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#include <vector>

namespace softphone::ui {
namespace {

constexpr auto kResourceDirectory = ":/i18n";
constexpr auto kPrefix = "softphone_";

constexpr auto kSourceLanguage = "en";

constexpr const char *kQtCatalogues[] = {"qtbase", "qtdeclarative", "qt"};

[[nodiscard]] QStringList qtCatalogueDirectories()
{
    QStringList places;
    if (const auto known = QLibraryInfo::path(QLibraryInfo::TranslationsPath); !known.isEmpty()) {
        places.append(known);
    }
    const auto beside = QCoreApplication::applicationDirPath() + QStringLiteral("/translations");
    if (!places.contains(beside)) {
        places.append(beside);
    }
    return places;
}

[[nodiscard]] QString requestedLanguage(const QString &explicitChoice)
{
    if (!explicitChoice.isEmpty()) {
        return explicitChoice;
    }

    if (const auto fromEnvironment = qEnvironmentVariable("SOFTPHONE_LANG");
        !fromEnvironment.isEmpty()) {
        return fromEnvironment;
    }

    return QLocale::system().name();
}

}

void registerMonolithCatalogue(shell::UiRegistry &registry)
{
    registry.addCatalogue(QStringLiteral("application"), QString::fromLatin1(kResourceDirectory),
                          QString::fromLatin1(kPrefix));
}

namespace {

const QStringList &offeredCodes()
{
    static const QStringList offered = QString::fromLatin1(SOFTPHONE_OFFERED_LANGUAGES)
                                           .split(QLatin1Char{' '}, Qt::SkipEmptyParts);
    return offered;
}

[[nodiscard]] QString offeredFor(const QString &language)
{
    for (auto tag : QLocale{language}.uiLanguages()) {
        tag.replace(QLatin1Char{'-'}, QLatin1Char{'_'});

        while (!tag.isEmpty()) {
            if (offeredCodes().contains(tag)) {
                return tag;
            }
            const auto cut = tag.lastIndexOf(QLatin1Char{'_'});
            if (cut < 0) {
                break;
            }
            tag.truncate(cut);
        }
    }
    return {};
}

}

QStringList availableLanguages(std::span<const shell::UiRegistry::Catalogue> catalogues)
{

    QStringList languages;

    for (const auto &catalogue : catalogues) {
        const QDir directory{catalogue.directory};
        for (const auto &entry :
             directory.entryList({catalogue.prefix + QStringLiteral("*.qm")}, QDir::Files)) {
            auto language = QFileInfo{entry}.completeBaseName();
            language.remove(0, catalogue.prefix.size());
            if (offeredCodes().contains(language) && !languages.contains(language)) {
                languages.append(language);
            }
        }
    }

    languages.sort();
    return languages;
}

QStringList availableLanguages(const shell::UiRegistry &registry)
{
    return availableLanguages(registry.catalogues());
}

QString offeredLanguage(std::span<const shell::UiRegistry::Catalogue> catalogues,
                        const QString &stored)
{

    if (stored.isEmpty() || stored.startsWith(QString::fromLatin1(kSourceLanguage)) ||
        availableLanguages(catalogues).contains(stored)) {
        return stored;
    }
    return {};
}

QString offeredLanguage(const shell::UiRegistry &registry, const QString &stored)
{
    return offeredLanguage(registry.catalogues(), stored);
}

QString languageEndonym(const QString &code)
{

    static const QHash<QString, QString> named{
        {QStringLiteral("es"), QString::fromUtf8("espa\u00f1ol")},
        {QStringLiteral("id"), QStringLiteral("bahasa Indonesia")}};
    const auto found = named.constFind(code);
    auto name = found != named.constEnd()
                    ? *found
                    : QLocale{code + QStringLiteral("_001")}.nativeLanguageName();
    if (name.isEmpty()) {
        return code;
    }

    name.replace(0, 1, name.at(0).toUpper());
    return name;
}

std::string contentLanguage(const QString &requested)
{
    const auto language = requestedLanguage(requested);

    if (const auto offered = offeredFor(language); !offered.isEmpty()) {
        return offered.toStdString();
    }

    return language.left(2).toLower().toStdString();
}

QString interfaceLanguage(const QString &requested)
{
    return offeredFor(requestedLanguage(requested));
}

QString installTranslations(QCoreApplication &application,
                            std::span<const shell::UiRegistry::Catalogue> catalogues,
                            const QString &requested)
{

    static std::vector<QTranslator *> installed;
    for (auto *translator : installed) {
        QCoreApplication::removeTranslator(translator);
        delete translator;
    }
    installed.clear();

    const auto language = requestedLanguage(requested);

    if (language.startsWith(QString::fromLatin1(kSourceLanguage))) {
        return QString::fromLatin1(kSourceLanguage);
    }

    auto spoken = QString::fromLatin1(kSourceLanguage);
    bool first = true;

    for (const auto &catalogue : catalogues) {

        auto *translator = new QTranslator{&application};

        const auto loaded = translator->load(QLocale{language}, catalogue.prefix.chopped(1),
                                             QStringLiteral("_"), catalogue.directory);
        if (loaded) {
            QCoreApplication::installTranslator(translator);
            installed.push_back(translator);
            if (first) {
                spoken = language;
            }
        } else {
            delete translator;
        }
        first = false;
    }

    for (const auto *name : kQtCatalogues) {
        for (const auto &where : qtCatalogueDirectories()) {
            auto *translator = new QTranslator{&application};
            if (translator->load(QLocale{language}, QString::fromLatin1(name), QStringLiteral("_"),
                                 where)) {
                QCoreApplication::installTranslator(translator);
                installed.push_back(translator);
                break;
            }
            delete translator;
        }
    }

    return spoken;
}

QString installTranslations(QCoreApplication &application, const shell::UiRegistry &registry,
                            const QString &requested)
{
    return installTranslations(application, registry.catalogues(), requested);
}

}
