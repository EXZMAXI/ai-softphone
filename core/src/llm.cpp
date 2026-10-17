// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/llm.hpp>

#include <algorithm>
#include <cstddef>

namespace softphone::core {

std::string_view toString(LlmRole role) noexcept
{
    switch (role) {
    case LlmRole::System:
        return "system";
    case LlmRole::User:
        return "user";
    case LlmRole::Assistant:
        return "assistant";
    }
    return "user";
}

std::optional<LlmRole> parseLlmRole(std::string_view text) noexcept
{
    if (text == "system") {
        return LlmRole::System;
    }
    if (text == "user") {
        return LlmRole::User;
    }
    if (text == "assistant") {
        return LlmRole::Assistant;
    }
    return std::nullopt;
}

std::string_view toString(LlmStatus status) noexcept
{
    switch (status) {
    case LlmStatus::Ok:
        return "ok";
    case LlmStatus::Unauthorised:
        return "unauthorised";
    case LlmStatus::RateLimited:
        return "rate-limited";
    case LlmStatus::TooLarge:
        return "too-large";
    case LlmStatus::Unreachable:
        return "unreachable";
    case LlmStatus::Refused:
        return "refused";
    case LlmStatus::Unsupported:
        return "unsupported";
    case LlmStatus::Malformed:
        return "malformed";
    case LlmStatus::Cancelled:
        return "cancelled";
    case LlmStatus::Failed:
        return "failed";
    }
    return "failed";
}

bool worthRetrying(LlmStatus status) noexcept
{
    switch (status) {
    case LlmStatus::RateLimited:
    case LlmStatus::Unreachable:
    case LlmStatus::Failed:
        return true;

    case LlmStatus::Ok:
    case LlmStatus::Unauthorised:
    case LlmStatus::Unsupported:
    case LlmStatus::Cancelled:
        return false;

    case LlmStatus::TooLarge:
    case LlmStatus::Malformed:
        return false;

    case LlmStatus::Refused:
        return false;
    }
    return false;
}

std::int64_t costMicros(const LlmUsage &usage, std::int64_t inPricePerMillion,
                        std::int64_t outPricePerMillion) noexcept
{
    if (inPricePerMillion <= 0 && outPricePerMillion <= 0) {

        return 0;
    }

    const auto spent = static_cast<std::int64_t>(usage.promptTokens) * inPricePerMillion +
                       static_cast<std::int64_t>(usage.answerTokens) * outPricePerMillion;

    constexpr std::int64_t kMillion = 1'000'000;
    return (spent + kMillion / 2) / kMillion;
}

int estimateTokens(std::string_view text) noexcept
{

    return static_cast<int>((text.size() + 2) / 3);
}

int estimateTokens(const std::vector<LlmMessage> &messages) noexcept
{

    int total = 8;
    for (const auto &message : messages) {
        total += 4 + estimateTokens(message.text);
    }
    return total;
}

namespace {

[[nodiscard]] std::string balancedFrom(std::string_view answer, std::size_t from)
{
    const auto opening = answer.find_first_of("{[", from);
    if (opening == std::string_view::npos) {
        return {};
    }

    const char open = answer.at(opening);
    const char close = open == '{' ? '}' : ']';

    int depth = 0;
    bool inString = false;
    bool escaped = false;

    for (std::size_t at = opening; at < answer.size(); ++at) {
        const char c = answer.at(at);

        if (escaped) {
            escaped = false;
            continue;
        }
        if (inString) {
            if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }

        if (c == '"') {
            inString = true;
        } else if (c == open) {
            ++depth;
        } else if (c == close) {
            --depth;
            if (depth == 0) {
                return std::string{answer.substr(opening, at - opening + 1)};
            }
        }
    }

    return {};
}

[[nodiscard]] std::string balancedIn(std::string_view answer)
{
    for (auto from = answer.find_first_of("{["); from != std::string_view::npos;
         from = answer.find_first_of("{[", from + 1)) {
        if (auto found = balancedFrom(answer, from); !found.empty()) {
            return found;
        }
    }
    return {};
}

}

std::string firstJsonValue(std::string_view answer)
{

    constexpr std::string_view kFence = "```";
    if (const auto fence = answer.find(kFence); fence != std::string_view::npos) {
        const auto afterFence = answer.find('\n', fence);
        if (afterFence != std::string_view::npos) {
            const auto closing = answer.find(kFence, afterFence);
            const auto length = closing == std::string_view::npos ? std::string_view::npos
                                                                  : closing - afterFence - 1;
            if (auto inside = balancedIn(answer.substr(afterFence + 1, length)); !inside.empty()) {
                return inside;
            }
        }
    }

    return balancedIn(answer);
}

std::vector<std::string> knownChatModels(std::string_view type)
{

    if (type == "yandex") {
        return {
            "yandexgpt-5.1",     "yandexgpt-5-pro",     "aliceai-llm",
            "aliceai-llm-flash", "deepseek-v4.1-flash", "qwen3.6-35b-a3b",
        };
    }

    return {};
}

}
