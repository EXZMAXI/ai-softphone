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

#include <softphone/core/settings.hpp>

#include <QString>
#include <QStringList>

#include <string_view>

namespace softphone::ui {

[[nodiscard]] bool hasANotificationArea();

[[nodiscard]] core::Layout shapeToRunIn(core::Layout stored);

struct RestartCommand {
    QString program;
    QStringList arguments;

    [[nodiscard]] bool valid() const noexcept
    {
        return !program.isEmpty();
    }
};

[[nodiscard]] RestartCommand howToStartAgain();

[[nodiscard]] QString shapeKey(core::Layout layout);

inline constexpr auto kSettingsWindow = "settings";

inline constexpr auto kPrimarySlot = "primary";
inline constexpr auto kFooterSlot = "footer";
inline constexpr auto kSideSlot = "side";
inline constexpr auto kToolbarSlot = "toolbar";

enum class Container {

    Phone,

    Panel,

    Workspace,

    Bar,

    Popover,

    Tray,
};

[[nodiscard]] std::string_view toString(Container container) noexcept;

struct Placement {
    Container container{Container::Panel};
    QString zone;
};

struct PhoneMetrics {

    int column{420};

    bool keyLetters{true};

    int air{8};

    int edge{16};

    int rowGap{16};

    int field{56};

    int keyStrip{148};

    bool oneChipRow{false};

    bool chipsCentred{true};

    int keyColumns{0};

    int dialKey{72};
    int dialDigit{22};

    int cardName{24};

    int cardPad{10};

    int cardButton{40};
    int cardButtonLarge{50};
    int cardGlyph{30};
    int cardGlyphLarge{38};

    int keyGlyph{46};

    int leastHeight{640};

    friend bool operator==(const PhoneMetrics &, const PhoneMetrics &) = default;
};

[[nodiscard]] PhoneMetrics metricsOf(core::Layout shape);

[[nodiscard]] Placement forWindow(core::Layout shape, const QString &windowId);

[[nodiscard]] Placement forSlot(core::Layout shape, const QString &slot);

}
