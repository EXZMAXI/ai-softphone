// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "phone_shape.hpp"

#include "chip_wrap.hpp"

#include <softphone/shell/windows.hpp>

namespace softphone::ui {

PhoneShape::PhoneShape(core::Layout layout, QObject *parent)
    : QObject{parent}, layout_{layout}, metrics_{metricsOf(layout)}
{

    toolbarIsAMenu_ =
        forSlot(layout_, QString::fromUtf8(kToolbarSlot)).container != Container::Phone;

    const auto keypad = forSlot(layout_, QString::fromUtf8(kPrimarySlot)).container;
    hasAColumn_ = keypad == Container::Phone || keypad == Container::Workspace;
    phoneInTheWorkspace_ = keypad == Container::Workspace;

    inAWorkspace_ =
        forWindow(layout_, QStringLiteral("recordings")).container == Container::Workspace;
}

QString PhoneShape::zoneOfWindow(const QString &windowId) const
{
    const auto placed = forWindow(layout_, windowId);
    return placed.container == Container::Workspace ? placed.zone : QString{};
}

QVariantMap PhoneShape::placeInRows(const QVariantList &widths, double available, double spacing,
                                    double rowHeight, double rowSpacing) const
{

    return wrapAsVariant(widths, available, spacing, rowHeight, rowSpacing, Lines::Evened);
}

QString PhoneShape::peopleWindow()
{
    return QString::fromUtf8(shell::kPeopleWindow);
}

QString PhoneShape::name() const
{
    const auto word = core::toString(layout_);
    return QString::fromUtf8(word.data(), static_cast<qsizetype>(word.size()));
}

}
