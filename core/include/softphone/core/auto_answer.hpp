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

#include <chrono>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

enum class AutoAnswerSpelling {
    None,

    CallInfo,

    AlertInfo,

    AnswerMode,
};

inline constexpr std::chrono::seconds kMaxAutoAnswerDelay{120};

inline constexpr std::chrono::seconds kMaxAutoAnswerSetting{20};

struct AutoAnswerRequest {
    AutoAnswerSpelling spelling{AutoAnswerSpelling::None};

    std::chrono::seconds after{0};

    [[nodiscard]] bool asked() const noexcept
    {
        return spelling != AutoAnswerSpelling::None;
    }

    friend bool operator==(const AutoAnswerRequest &, const AutoAnswerRequest &) = default;
};

struct InvitationHeaders {
    std::vector<std::string> callInfo;
    std::vector<std::string> alertInfo;
    std::vector<std::string> answerMode;
};

[[nodiscard]] AutoAnswerRequest autoAnswerRequest(const InvitationHeaders &headers);

enum class AutoAnswer {

    No,

    Invitation,

    AccountSetting,

    SwitchRequest,
};

enum class SwitchRequest {

    Talk,

    Hold,
};

[[nodiscard]] std::optional<SwitchRequest> parseSwitchEvent(std::string_view header) noexcept;

[[nodiscard]] std::string_view toString(SwitchRequest request) noexcept;

struct AutoAnswerPolicy {
    bool enabled{false};

    std::chrono::seconds after{0};

    friend bool operator==(const AutoAnswerPolicy &, const AutoAnswerPolicy &) = default;
};

struct OtherCalls {

    bool any{false};

    bool onAnotherLine{false};
};

[[nodiscard]] bool mayInterrupt(AutoAnswer why, const OtherCalls &others) noexcept;

struct AutoAnswerDecision {
    AutoAnswer why{AutoAnswer::No};
    std::chrono::seconds after{0};

    [[nodiscard]] bool answering() const noexcept
    {
        return why != AutoAnswer::No;
    }

    friend bool operator==(const AutoAnswerDecision &, const AutoAnswerDecision &) = default;
};

[[nodiscard]] AutoAnswerDecision decideAutoAnswer(const AutoAnswerRequest &asked,
                                                  const AutoAnswerPolicy &account,
                                                  const OtherCalls &others) noexcept;

[[nodiscard]] bool fromTheSwitch(std::string_view source,
                                 std::span<const std::string> switchAddresses) noexcept;

struct LineOnTheWire {

    std::string id;

    std::vector<std::string> switchAddresses;

    std::string username;
};

[[nodiscard]] std::optional<std::size_t>
lineThatOwns(std::string_view source, std::string_view user,
             std::span<const LineOnTheWire> lines) noexcept;

[[nodiscard]] std::string_view toString(AutoAnswerSpelling spelling) noexcept;
[[nodiscard]] std::string_view toString(AutoAnswer answer) noexcept;

[[nodiscard]] std::optional<AutoAnswer> parseAutoAnswer(std::string_view text) noexcept;

}
