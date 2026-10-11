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

#include <QVariantList>
#include <QVariantMap>

#include <vector>

namespace softphone::ui {

struct ChipPlace {
    double x{0};
    double y{0};
};

struct ChipWrap {

    std::vector<ChipPlace> places;

    int rows{0};
};

enum class Lines {

    Filled,

    Evened,
};

[[nodiscard]] ChipWrap wrapChips(const std::vector<double> &widths, double available,
                                 double spacing, double rowHeight, double rowSpacing,
                                 Lines lines = Lines::Filled);

[[nodiscard]] QVariantMap wrapAsVariant(const QVariantList &widths, double available,
                                        double spacing, double rowHeight, double rowSpacing,
                                        Lines lines = Lines::Filled);

}
