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

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::ui {

using core::Hotkey;
using core::HotkeyAction;

enum class HotkeyProblem {
    None,

    Unreadable,

    NoModifier,
};

[[nodiscard]] std::vector<HotkeyAction> hotkeyActions();

[[nodiscard]] std::vector<Hotkey> defaultHotkeys();

[[nodiscard]] std::string normaliseHotkey(std::string_view sequence);

[[nodiscard]] HotkeyProblem checkHotkey(std::string_view sequence);

[[nodiscard]] std::vector<Hotkey> planHotkeys(std::span<const Hotkey> stored);

[[nodiscard]] std::vector<HotkeyAction> clashingHotkeys(std::span<const Hotkey> bindings);

}
