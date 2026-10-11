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

#include "hotkeys.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace softphone::ui {

enum class GrabProblem {
    None,

    Unsupported,

    Taken,

    Unreadable,
};

struct GrabOutcome {
    HotkeyAction action{HotkeyAction::Answer};
    GrabProblem problem{GrabProblem::None};

    friend bool operator==(const GrabOutcome &, const GrabOutcome &) = default;
};

class IGlobalHotkeys {
public:

    using Activated = std::function<void(HotkeyAction)>;

    IGlobalHotkeys() = default;
    IGlobalHotkeys(const IGlobalHotkeys &) = delete;
    IGlobalHotkeys &operator=(const IGlobalHotkeys &) = delete;
    IGlobalHotkeys(IGlobalHotkeys &&) = delete;
    IGlobalHotkeys &operator=(IGlobalHotkeys &&) = delete;
    virtual ~IGlobalHotkeys() = default;

    [[nodiscard]] virtual std::vector<GrabOutcome> grab(std::span<const Hotkey> bindings) = 0;

    [[nodiscard]] virtual bool supported() const = 0;
};

inline constexpr std::uint16_t kShiftMask = 1U << 0U;
inline constexpr std::uint16_t kLockMask = 1U << 1U;
inline constexpr std::uint16_t kControlMask = 1U << 2U;
inline constexpr std::uint16_t kMod1Mask = 1U << 3U;
inline constexpr std::uint16_t kMod2Mask = 1U << 4U;
inline constexpr std::uint16_t kMod3Mask = 1U << 5U;
inline constexpr std::uint16_t kMod4Mask = 1U << 6U;
inline constexpr std::uint16_t kMod5Mask = 1U << 7U;

[[nodiscard]] std::uint16_t xModifiers(std::string_view sequence);

[[nodiscard]] std::string xKeyName(std::string_view sequence);

[[nodiscard]] std::string hotkeyKeyName(std::string_view sequence);

[[nodiscard]] std::vector<std::uint16_t> withIgnoredModifiers(std::uint16_t modifiers);

inline constexpr std::uint16_t kIgnoredModifiers = kLockMask | kMod2Mask | kMod5Mask;

inline constexpr std::uint32_t kAltFlag = 0x0001U;
inline constexpr std::uint32_t kControlFlag = 0x0002U;
inline constexpr std::uint32_t kShiftFlag = 0x0004U;
inline constexpr std::uint32_t kSuperFlag = 0x0008U;

inline constexpr std::uint32_t kNoRepeatFlag = 0x4000U;

[[nodiscard]] std::uint32_t winModifiers(std::string_view sequence);

inline constexpr std::uint32_t kCommandFlag = 0x0100U;
inline constexpr std::uint32_t kCarbonShiftFlag = 0x0200U;
inline constexpr std::uint32_t kOptionFlag = 0x0800U;
inline constexpr std::uint32_t kCarbonControlFlag = 0x1000U;

[[nodiscard]] std::uint32_t carbonModifiers(std::string_view sequence);

[[nodiscard]] std::optional<std::uint32_t> carbonKeyCode(std::string_view sequence);

[[nodiscard]] std::uint32_t winVirtualKey(std::string_view sequence);

[[nodiscard]] std::unique_ptr<IGlobalHotkeys>
makeGlobalHotkeys(IGlobalHotkeys::Activated activated);

}
