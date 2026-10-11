// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/buttons.hpp>

#include <algorithm>
#include <charconv>

namespace softphone::core {
namespace {

constexpr std::string_view kPrefix = "button-";

}

std::string_view toString(LineState state) noexcept
{
    switch (state) {
    case LineState::Unknown:
        return "unknown";
    case LineState::Idle:
        return "idle";
    case LineState::Ringing:
        return "ringing";
    case LineState::Busy:
        return "busy";
    }
    return "unknown";
}

LineState lineStateFrom(std::string_view text) noexcept
{
    if (text == "idle") {
        return LineState::Idle;
    }
    if (text == "ringing") {
        return LineState::Ringing;
    }
    if (text == "busy") {
        return LineState::Busy;
    }
    return LineState::Unknown;
}

std::string_view toString(WatchTrouble trouble) noexcept
{
    switch (trouble) {
    case WatchTrouble::None:
        return "none";
    case WatchTrouble::NoLine:
        return "no-line";
    case WatchTrouble::Asking:
        return "asking";
    case WatchTrouble::NoSuchLine:
        return "no-such-line";
    case WatchTrouble::Refused:
        return "refused";
    case WatchTrouble::Silent:
        return "silent";
    }
    return "silent";
}

WatchTrouble troubleFrom(int sipStatus) noexcept
{
    switch (sipStatus) {
    case 404:
    case 410:
    case 604:
        return WatchTrouble::NoSuchLine;

    case 401:
    case 403:
    case 405:
    case 407:
    case 415:
    case 489:
    case 501:
        return WatchTrouble::Refused;

    default:

        return WatchTrouble::Silent;
    }
}

bool isUsable(const PhoneButton &button) noexcept
{

    return !button.number.empty();
}

std::string buttonLabel(const PhoneButton &button)
{
    return button.title.empty() ? button.number : button.title;
}

std::string nextButtonId(const std::vector<PhoneButton> &buttons)
{
    int highest = 0;
    for (const auto &button : buttons) {
        if (!button.id.starts_with(kPrefix)) {
            continue;
        }

        const std::string_view digits{button.id.begin() + static_cast<long>(kPrefix.size()),
                                      button.id.end()};
        int number = 0;
        const auto *const end = digits.data() + digits.size();
        if (std::from_chars(digits.data(), end, number).ptr == end) {
            highest = std::max(highest, number);
        }
    }

    return std::string{kPrefix} + std::to_string(highest + 1);
}

}
