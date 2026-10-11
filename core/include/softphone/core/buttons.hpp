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

#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

enum class LineState {

    Unknown,

    Idle,

    Ringing,

    Busy,
};

[[nodiscard]] std::string_view toString(LineState state) noexcept;
[[nodiscard]] LineState lineStateFrom(std::string_view text) noexcept;

enum class WatchTrouble {

    None,

    NoLine,

    Asking,

    NoSuchLine,

    Refused,

    Silent,
};

[[nodiscard]] std::string_view toString(WatchTrouble trouble) noexcept;

[[nodiscard]] WatchTrouble troubleFrom(int sipStatus) noexcept;

struct LampState {
    LineState state{LineState::Unknown};
    WatchTrouble trouble{WatchTrouble::None};

    friend bool operator==(const LampState &, const LampState &) = default;
};

struct PhoneButton {

    std::string id;

    std::string title;

    std::string number;

    std::string accountId;

    bool watched{true};

    friend bool operator==(const PhoneButton &, const PhoneButton &) = default;
};

struct ButtonSettings {
    std::vector<PhoneButton> buttons;

    friend bool operator==(const ButtonSettings &, const ButtonSettings &) = default;
};

[[nodiscard]] bool isUsable(const PhoneButton &button) noexcept;

[[nodiscard]] std::string buttonLabel(const PhoneButton &button);

[[nodiscard]] std::string nextButtonId(const std::vector<PhoneButton> &buttons);

}
