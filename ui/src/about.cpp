// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "about.hpp"

#include "translations.hpp"

#include <softphone/core/countries.hpp>
#include <softphone/core/local_hosts.hpp>
#include <softphone/core/version.hpp>

#include <QVariantMap>
#include <QtGlobal>

#include <array>
#include <utility>

namespace softphone::ui {
namespace {

struct Component {
    const char *name;
    const char *licence;
    const char *url;
};

constexpr std::array kCommonComponents{
    Component{"Qt 6", "GPL-2.0 or GPL-3.0", "https://www.qt.io"},
    Component{"pjproject (PJSIP)", "GPL-2.0-or-later", "https://www.pjsip.org"},
    Component{"Opus", "BSD-3-Clause", "https://opus-codec.org"},
    Component{"SQLite", "public domain", "https://www.sqlite.org"},
    Component{"nlohmann/json", "MIT", "https://github.com/nlohmann/json"},
    Component{"cpp-httplib", "MIT", "https://github.com/yhirose/cpp-httplib"},
    Component{"IXWebSocket", "BSD-3-Clause", "https://github.com/machinezone/IXWebSocket"},
    Component{"nghttp2", "MIT", "https://nghttp2.org"},
    Component{"LAME", "LGPL-2.0-or-later", "https://lame.sourceforge.io"},
    Component{"minimp3", "CC0-1.0", "https://github.com/lieff/minimp3"},

    Component{"OpenSSL", "Apache-2.0", "https://www.openssl.org"},
    Component{"Fluent UI System Icons", "MIT",
              "https://github.com/microsoft/fluentui-system-icons"},
};

#if defined(_WIN32)

constexpr std::array<Component, 0> kPlatformComponents{};

#else

constexpr std::array kPlatformComponents{
    Component{"libsecret", "LGPL-2.1-or-later", "https://wiki.gnome.org/Projects/Libsecret"},
    Component{"ALSA", "LGPL-2.1-or-later", "https://www.alsa-project.org"},
    Component{"PulseAudio (client)", "LGPL-2.1-or-later",
              "https://www.freedesktop.org/wiki/Software/PulseAudio/"},
};

#endif

}

About::About(QObject *parent) : QObject{parent} {}

QString About::version() const
{
    return QString::fromUtf8(core::versionString().data(),
                             static_cast<qsizetype>(core::versionString().size()));
}

QString About::qtVersion() const
{

    return QString::fromLatin1(qVersion());
}

QString About::website() const
{

    return QString::fromStdString(core::hosts::siteFor(region_.country));
}

QString About::documentation() const
{
    return QString::fromStdString(core::hosts::docsFor(region_.country));
}

QString About::documentationPage() const
{
    return QString::fromStdString(
        core::hosts::docsPageFor(region_.country, installed_.toStdString()));
}

void About::addressesMoved()
{
    Q_EMIT websiteChanged();
    Q_EMIT documentationChanged();
}

void About::speak(const QString &language)
{

    const auto whole = ui::interfaceLanguage(language);
    const auto wanted = (whole.isEmpty() ? language : whole).left(2).toLower();
    if (language_ == wanted && installed_ == whole) {
        return;
    }

    const auto namesMoved = language_ != wanted;
    const auto pageMoved = installed_ != whole;
    language_ = wanted;
    installed_ = whole;

    if (namesMoved) {
        Q_EMIT countriesChanged();
    }
    if (pageMoved) {
        Q_EMIT documentationChanged();
    }
}

QVariantList About::countries() const
{
    QVariantList list;
    const auto &all = core::countries(language_.toStdString());
    list.reserve(static_cast<qsizetype>(all.size()));
    for (const auto &one : all) {
        list.append(QVariantMap{
            {QStringLiteral("code"), QString::fromStdString(one.code)},
            {QStringLiteral("name"), QString::fromStdString(one.name)},
        });
    }
    return list;
}

QString About::country() const
{
    return QString::fromStdString(region_.country);
}

void About::chooseCountry(const QString &code)
{

    const auto asked = code.toStdString();
    core::RegionSettings wanted;
    wanted.country = core::isCountry(asked) ? asked : std::string{};

    wanted.decided = true;

    if (wanted == region_) {
        return;
    }
    region_ = std::move(wanted);
    if (told_) {
        told_(region_);
    }
    Q_EMIT countryChanged();
    addressesMoved();
}

void About::detectCountry()
{
    chooseCountry(countryOfThisMachine());
}

QString About::countryOfThisMachine()
{

    return QString::fromStdString(core::machineCountry());
}

void About::applyRegion(const core::RegionSettings &settings)
{

    if (!settings.decided && region_.decided) {
        return;
    }
    if (settings == region_) {
        return;
    }
    region_ = settings;
    Q_EMIT countryChanged();
    addressesMoved();
}

core::RegionSettings About::region() const
{
    return region_;
}

void About::setRegionCallback(std::function<void(core::RegionSettings)> told)
{
    told_ = std::move(told);
}

QString About::licence() const
{

    return QStringLiteral("GPL-2.0-or-later");
}

QVariantList About::components() const
{
    QVariantList list;
    list.reserve(static_cast<qsizetype>(kCommonComponents.size() + kPlatformComponents.size()));

    const auto append = [&list](const Component &component) {
        list.append(QVariantMap{{QStringLiteral("name"), QString::fromUtf8(component.name)},
                                {QStringLiteral("licence"), QString::fromUtf8(component.licence)},
                                {QStringLiteral("url"), QString::fromUtf8(component.url)}});
    };

    for (const auto &component : kCommonComponents) {
        append(component);
    }
    for (const auto &component : kPlatformComponents) {
        append(component);
    }

    return list;
}

}
