// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/auto_answer.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <optional>
#include <utility>

namespace softphone::core {
namespace {

[[nodiscard]] std::string lower(std::string_view text)
{
    std::string folded;
    folded.reserve(text.size());
    for (const auto character : text) {
        folded.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
    }
    return folded;
}

[[nodiscard]] std::string_view trim(std::string_view text)
{
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) {
        text.remove_prefix(1);
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0) {
        text.remove_suffix(1);
    }
    return text;
}

[[nodiscard]] std::string outsideAngles(std::string_view value)
{
    std::string outside;
    bool inside = false;
    for (const auto character : value) {
        if (character == '<') {
            inside = true;
        } else if (character == '>') {
            inside = false;
        } else if (!inside) {
            outside.push_back(character);
        }
    }
    return outside;
}

[[nodiscard]] std::optional<std::string> parameter(std::string_view value, std::string_view name)
{
    const auto folded = lower(value);
    const auto wanted = lower(name);

    std::size_t at = 0;
    while ((at = folded.find(';', at)) != std::string::npos) {
        auto start = ++at;
        while (start < folded.size() &&
               std::isspace(static_cast<unsigned char>(folded[start])) != 0) {
            ++start;
        }

        if (folded.compare(start, wanted.size(), wanted) != 0) {
            continue;
        }

        auto after = start + wanted.size();
        while (after < folded.size() &&
               std::isspace(static_cast<unsigned char>(folded[after])) != 0) {
            ++after;
        }
        if (after >= folded.size() || folded[after] != '=') {
            continue;
        }
        ++after;

        const auto end = folded.find(';', after);
        auto text = trim(std::string_view{value}.substr(
            after, end == std::string::npos ? std::string_view::npos : end - after));
        if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
            text = text.substr(1, text.size() - 2);
        }
        return std::string{text};
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<std::chrono::seconds> seconds(std::string_view text)
{
    if (text.empty() || !std::all_of(text.begin(), text.end(), [](char character) {
            return std::isdigit(static_cast<unsigned char>(character)) != 0;
        })) {
        return std::nullopt;
    }

    long long value = 0;
    const auto *const last = text.data() + text.size();
    const auto [stopped, error] = std::from_chars(text.data(), last, value);
    if (error != std::errc{} || stopped != last || value > kMaxAutoAnswerDelay.count()) {
        return std::nullopt;
    }
    return std::chrono::seconds{value};
}

constexpr std::array<std::string_view, 5> kIntercomTokens{
    "intercom", "alert-autoanswer", "auto-answer", "autoanswer", "ring-answer",
};

[[nodiscard]] bool namesIntercom(std::string_view value)
{
    const auto outside = lower(outsideAngles(value));

    std::string word;
    std::vector<std::string> words;
    for (const auto character : outside) {
        if (std::isalnum(static_cast<unsigned char>(character)) != 0 || character == '-') {
            word.push_back(character);
        } else if (!word.empty()) {
            words.push_back(std::exchange(word, std::string{}));
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    for (std::size_t at = 0; at < words.size(); ++at) {
        const auto joined = at + 1 < words.size() ? words[at] + '-' + words[at + 1] : std::string{};
        for (const auto token : kIntercomTokens) {
            if (words[at] == token || joined == token) {
                return true;
            }
        }
    }
    return false;
}

}

AutoAnswerRequest autoAnswerRequest(const InvitationHeaders &headers)
{
    for (const auto &value : headers.callInfo) {
        const auto asked = parameter(value, "answer-after");
        if (!asked.has_value()) {
            continue;
        }
        if (const auto delay = seconds(*asked)) {
            return AutoAnswerRequest{AutoAnswerSpelling::CallInfo, *delay};
        }
    }

    for (const auto &value : headers.alertInfo) {
        if (!namesIntercom(value)) {
            continue;
        }

        const auto delay = parameter(value, "delay");
        return AutoAnswerRequest{AutoAnswerSpelling::AlertInfo,
                                 delay ? seconds(*delay).value_or(std::chrono::seconds{0})
                                       : std::chrono::seconds{0}};
    }

    for (const auto &value : headers.answerMode) {
        const auto text = std::string_view{value};
        const auto token = lower(trim(text.substr(0, text.find(';'))));
        if (token == "auto") {
            return AutoAnswerRequest{AutoAnswerSpelling::AnswerMode, std::chrono::seconds{0}};
        }
    }

    return {};
}

std::optional<SwitchRequest> parseSwitchEvent(std::string_view header) noexcept
{
    const auto token = lower(trim(header.substr(0, header.find(';'))));
    if (token == "talk") {
        return SwitchRequest::Talk;
    }
    if (token == "hold") {
        return SwitchRequest::Hold;
    }
    return std::nullopt;
}

std::string_view toString(SwitchRequest request) noexcept
{
    return request == SwitchRequest::Hold ? "hold" : "talk";
}

bool mayInterrupt(AutoAnswer why, const OtherCalls &others) noexcept
{
    if (!others.any) {
        return true;
    }
    return why == AutoAnswer::Invitation && !others.onAnotherLine;
}

AutoAnswerDecision decideAutoAnswer(const AutoAnswerRequest &asked, const AutoAnswerPolicy &account,
                                    const OtherCalls &others) noexcept
{
    if (asked.asked() && mayInterrupt(AutoAnswer::Invitation, others)) {
        return AutoAnswerDecision{AutoAnswer::Invitation, asked.after};
    }
    if (account.enabled && mayInterrupt(AutoAnswer::AccountSetting, others)) {
        return AutoAnswerDecision{AutoAnswer::AccountSetting, account.after};
    }
    return {};
}

bool fromTheSwitch(std::string_view source, std::span<const std::string> switchAddresses) noexcept
{
    if (source.empty() || switchAddresses.empty()) {
        return false;
    }

    return std::any_of(switchAddresses.begin(), switchAddresses.end(),
                       [source](const std::string &address) {
                           return !address.empty() && lower(address) == lower(source);
                       });
}

std::optional<std::size_t> lineThatOwns(std::string_view source, std::string_view user,
                                        std::span<const LineOnTheWire> lines) noexcept
{
    std::optional<std::size_t> only;

    for (std::size_t at = 0; at < lines.size(); ++at) {
        const auto &line = lines[at];
        if (!fromTheSwitch(source, line.switchAddresses)) {
            continue;
        }

        if (!user.empty() && line.username != user) {
            continue;
        }

        if (only.has_value()) {

            return std::nullopt;
        }
        only = at;
    }

    return only;
}

std::string_view toString(AutoAnswer answer) noexcept
{
    switch (answer) {
    case AutoAnswer::No:
        break;
    case AutoAnswer::Invitation:
        return "invitation";
    case AutoAnswer::AccountSetting:
        return "account-setting";
    case AutoAnswer::SwitchRequest:
        return "switch-request";
    }
    return "no";
}

std::optional<AutoAnswer> parseAutoAnswer(std::string_view text) noexcept
{
    for (const auto answer : {AutoAnswer::No, AutoAnswer::Invitation, AutoAnswer::AccountSetting,
                              AutoAnswer::SwitchRequest}) {
        if (toString(answer) == text) {
            return answer;
        }
    }
    return std::nullopt;
}

std::string_view toString(AutoAnswerSpelling spelling) noexcept
{
    switch (spelling) {
    case AutoAnswerSpelling::None:
        break;
    case AutoAnswerSpelling::CallInfo:
        return "call-info";
    case AutoAnswerSpelling::AlertInfo:
        return "alert-info";
    case AutoAnswerSpelling::AnswerMode:
        return "answer-mode";
    }
    return "none";
}

}
