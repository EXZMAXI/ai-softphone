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

#include <softphone/core/llm.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::core {

enum class JsonAsk {

    Words,

    ResponseFormat,

    Prefill,
};

enum class Thinking {

    AsTheModelLikes,

    Off,

    Low,
};

struct LlmDialect {

    std::string answerLimit{"max_tokens"};

    bool answerLimitOptional{true};

    bool temperature{true};

    double temperatureMax{2.0};

    JsonAsk json{JsonAsk::Words};

    Thinking thinking{Thinking::AsTheModelLikes};
};

[[nodiscard]] LlmDialect dialectFor(std::string_view adapter, std::string_view model);

[[nodiscard]] LlmDialect dialectFor(std::string_view adapter, std::string_view model,
                                    const std::vector<std::pair<std::string, std::string>> &params);

[[nodiscard]] bool isDialectName(std::string_view name);

namespace dialect {
constexpr std::string_view kAnswerLimit = "dialect_answer_limit";
constexpr std::string_view kTemperature = "dialect_temperature";
constexpr std::string_view kJson = "dialect_json";
constexpr std::string_view kThinking = "dialect_thinking";
}

[[nodiscard]] std::vector<std::pair<std::string, std::string>>
correctionsFor(std::string_view complaint, const LlmDialect &now);

[[nodiscard]] std::string_view toString(JsonAsk ask) noexcept;
[[nodiscard]] JsonAsk parseJsonAsk(std::string_view text, JsonAsk fallback) noexcept;

[[nodiscard]] std::string_view toString(Thinking thinking) noexcept;
[[nodiscard]] Thinking parseThinking(std::string_view text, Thinking fallback) noexcept;

[[nodiscard]] LlmAnswer askLearning(const std::function<LlmAnswer(const LlmEndpoint &)> &askOnce,
                                    std::string_view adapter, const LlmEndpoint &where);

}
