// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "window_shape.hpp"

#include <softphone/shell/windows.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <cstdlib>

namespace softphone::ui {
namespace {

[[nodiscard]] QString environmentValue(const char *name)
{
    const auto *value = std::getenv(name);
    return (value != nullptr && *value != '\0') ? QString::fromLocal8Bit(value) : QString{};
}

[[nodiscard]] QString bundleAround(const QString &executable)
{
    const QFileInfo binary{executable};
    const auto macOs = binary.absoluteDir();
    if (macOs.dirName() != QStringLiteral("MacOS")) {
        return {};
    }
    auto contents = macOs;
    if (!contents.cdUp() || contents.dirName() != QStringLiteral("Contents")) {
        return {};
    }
    auto bundle = contents;
    if (!bundle.cdUp() || !bundle.dirName().endsWith(QStringLiteral(".app"))) {
        return {};
    }
    return bundle.absolutePath();
}

}

bool hasANotificationArea()
{

    return true;
}

core::Layout shapeToRunIn(core::Layout stored)
{
    return core::honouredLayout(stored, hasANotificationArea());
}

QString shapeKey(core::Layout layout)
{
    if (layout == core::Layout::Windows) {
        return {};
    }
    const auto word = core::toString(layout);
    return QString::fromUtf8(word.data(), static_cast<qsizetype>(word.size()));
}

std::string_view toString(Container container) noexcept
{
    switch (container) {
    case Container::Phone:
        return "phone";
    case Container::Panel:
        return "panel";
    case Container::Workspace:
        return "workspace";
    case Container::Bar:
        return "bar";
    case Container::Popover:
        return "popover";
    case Container::Tray:
        return "tray";
    }
    return "unknown";
}

PhoneMetrics metricsOf(core::Layout shape)
{

    constexpr PhoneMetrics kFullSize{};

    constexpr PhoneMetrics kCompact{

 300,

 false,

 4,

 6,

 10,

 40,

 96,

 true,

 false,

 3,

 52,

 17,

 17,

 3,

 28,

 34,

 20,

 26,

 57,

 530,
    };

    switch (shape) {
    case core::Layout::Windows:
        return kFullSize;

    case core::Layout::WorkspaceAndPhone:
        return kFullSize;

    case core::Layout::Compact:
        return kCompact;

    case core::Layout::Workspace: {

        auto column = kCompact;
        column.oneChipRow = false;
        return column;
    }

    case core::Layout::Bar:
    case core::Layout::NoWindow:

        return kCompact;
    }
    return kFullSize;
}

Placement forWindow(core::Layout shape, const QString &windowId)
{
    static_cast<void>(windowId);

    switch (shape) {
    case core::Layout::Windows:

        return {Container::Panel, {}};

    case core::Layout::NoWindow:
    case core::Layout::Bar:
    case core::Layout::Workspace:
    case core::Layout::WorkspaceAndPhone:
    case core::Layout::Compact:

        return {Container::Workspace, QStringLiteral("main")};
    }
    return {Container::Panel, {}};
}

Placement forSlot(core::Layout shape, const QString &slot)
{
    switch (shape) {
    case core::Layout::Windows:

        return {Container::Phone, slot};

    case core::Layout::WorkspaceAndPhone:

        return {Container::Phone, slot};

    case core::Layout::Compact:

        if (slot == QLatin1String(kToolbarSlot)) {
            return {Container::Popover, slot};
        }
        return {Container::Phone, slot};

    case core::Layout::Workspace:

        if (slot == QLatin1String(kToolbarSlot)) {
            return {Container::Popover, slot};
        }
        return {Container::Workspace, QStringLiteral("toolbox")};

    case core::Layout::Bar:

        if (slot == QLatin1String(kToolbarSlot)) {
            return {Container::Popover, slot};
        }

        if (slot == QLatin1String(kPrimarySlot)) {
            return {Container::Popover, slot};
        }
        return {Container::Bar, slot};

    case core::Layout::NoWindow:

        if (slot == QLatin1String(kToolbarSlot)) {
            return {Container::Tray, slot};
        }
        return {Container::Popover, slot};
    }
    return {Container::Phone, slot};
}

RestartCommand howToStartAgain()
{

    if (const auto image = environmentValue("APPIMAGE"); !image.isEmpty()) {
        return {image, {}};
    }

    const auto executable = QCoreApplication::applicationFilePath();

    if (const auto bundle = bundleAround(executable); !bundle.isEmpty()) {
        return {QStringLiteral("/usr/bin/open"), {QStringLiteral("-n"), bundle}};
    }

    return {executable, {}};
}

}
