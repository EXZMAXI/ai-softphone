// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "window_memory.hpp"

#include "window_placement.hpp"

#include <softphone/core/config_store.hpp>
#include <softphone/core/paths.hpp>

#include <QGuiApplication>
#include <QScreen>
#include <QSettings>

#include <utility>

#include <filesystem>
#include <system_error>

namespace softphone::ui {
namespace {

[[nodiscard]] std::filesystem::path storeFile()
{
    return core::dataDirectory() / "windows.ini";
}

[[nodiscard]] QString storePath()
{
    const auto file = storeFile();

    std::error_code failed;
    std::filesystem::create_directories(file.parent_path(), failed);

    return QString::fromStdString(core::asUtf8(file));
}

[[nodiscard]] QSettings openStore()
{
    return QSettings{storePath(), QSettings::IniFormat};
}

[[nodiscard]] QList<QRect> availableScreens()
{
    QList<QRect> geometries;

    if (const auto *primary = QGuiApplication::primaryScreen(); primary != nullptr) {
        geometries.append(primary->availableGeometry());
    }
    for (const auto *screen : QGuiApplication::screens()) {
        if (screen != QGuiApplication::primaryScreen()) {
            geometries.append(screen->availableGeometry());
        }
    }
    return geometries;
}

}

WindowMemory::WindowMemory(QString shape, QObject *parent)
    : QObject{parent}, shape_{std::move(shape)}
{}

QString WindowMemory::filed(const QString &subtree, const QString &key) const
{

    if (shape_.isEmpty()) {
        return subtree + QStringLiteral("/") + key;
    }
    return subtree + QStringLiteral("/") + shape_ + QStringLiteral("/") + key;
}

QRect WindowMemory::restore(const QString &key, int minimumWidth, int minimumHeight) const
{
    const auto settings = openStore();
    const auto saved = settings.value(filed(QStringLiteral("windows"), key)).toRect();
    if (!saved.isValid() || saved.width() <= 0 || saved.height() <= 0) {
        return {};
    }

    return placeWindow(saved, availableScreens(), QSize{minimumWidth, minimumHeight});
}

QSize WindowMemory::restoreSize(const QString &key, int minimumWidth, int minimumHeight) const
{
    const auto settings = openStore();
    const auto saved =
        settings.value(filed(QStringLiteral("windows"), key) + QStringLiteral("-size")).toSize();
    if (!saved.isValid() || saved.width() <= 0 || saved.height() <= 0) {
        return {};
    }

    const auto screens = availableScreens();
    const auto origin = screens.isEmpty() ? QPoint{0, 0} : screens.first().topLeft();
    return placeWindow(QRect{origin, saved}, screens, QSize{minimumWidth, minimumHeight}).size();
}

void WindowMemory::rememberSize(const QString &key, int width, int height)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    auto settings = openStore();
    settings.setValue(filed(QStringLiteral("windows"), key) + QStringLiteral("-size"),
                      QSize{width, height});
}

bool WindowMemory::noticeGiven(const QString &key) const
{
    return openStore().value(QStringLiteral("notices/") + key, false).toBool();
}

void WindowMemory::rememberNotice(const QString &key)
{
    auto settings = openStore();
    settings.setValue(QStringLiteral("notices/") + key, true);

    ++noticeRevision_;
    Q_EMIT noticeRevisionChanged();
}

QString WindowMemory::chose(const QString &key, const QString &fallback) const
{
    return openStore().value(filed(QStringLiteral("chosen"), key), fallback).toString();
}

void WindowMemory::remembers(const QString &key, const QString &value)
{
    auto settings = openStore();
    settings.setValue(filed(QStringLiteral("chosen"), key), value);
}

void WindowMemory::remember(const QString &key, int x, int y, int width, int height)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    auto settings = openStore();
    settings.setValue(filed(QStringLiteral("windows"), key), QRect{x, y, width, height});
}

}
