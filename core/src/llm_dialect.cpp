// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/llm_dialect.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <vector>

namespace softphone::core {
namespace {

[[nodiscard]] std::string folded(std::string_view text)
{
    std::string out;
    out.reserve(text.size());
    for (const char letter : text) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(letter))));
    }
    return out;
}

enum class Match {

    Begins,

    Anywhere,
};

struct Rule {
    std::string_view adapter;
    std::string_view name;
    Match how;
    LlmDialect wants;
};

[[nodiscard]] const std::vector<Rule> &rules()
{
    static const std::vector<Rule> table = {
        {"openai",
         "gpt-5",
         Match::Begins,
         {"max_completion_tokens", true, false, 2.0, JsonAsk::ResponseFormat,
          Thinking::AsTheModelLikes}},
        {"openai",
         "o1",
         Match::Begins,
         {"max_completion_tokens", true, false, 2.0, JsonAsk::ResponseFormat,
          Thinking::AsTheModelLikes}},
        {"openai",
         "o3",
         Match::Begins,
         {"max_completion_tokens", true, false, 2.0, JsonAsk::ResponseFormat,
          Thinking::AsTheModelLikes}},
        {"openai",
         "o4",
         Match::Begins,
         {"max_completion_tokens", true, false, 2.0, JsonAsk::ResponseFormat,
          Thinking::AsTheModelLikes}},

        {"openai",
         "",
         Match::Begins,
         {"max_tokens", true, true, 2.0, JsonAsk::ResponseFormat, Thinking::AsTheModelLikes}},

        {"anthropic",
         "opus-5",
         Match::Anywhere,
         {"max_tokens", false, false, 1.0, JsonAsk::Words, Thinking::Off}},
        {"anthropic",
         "sonnet-5",
         Match::Anywhere,
         {"max_tokens", false, false, 1.0, JsonAsk::Words, Thinking::Off}},
        {"anthropic",
         "fable-5",
         Match::Anywhere,
         {"max_tokens", false, false, 1.0, JsonAsk::Words, Thinking::Low}},

        {"anthropic",
         "",
         Match::Begins,
         {"max_tokens", false, true, 1.0, JsonAsk::Words, Thinking::AsTheModelLikes}},
    };
    return table;
}

[[nodiscard]] std::string valueOf(const std::vector<std::pair<std::string, std::string>> &params,
                                  std::string_view name)
{
    const auto found = std::find_if(params.begin(), params.end(),
                                    [name](const auto &pair) { return pair.first == name; });
    return found == params.end() ? std::string{} : found->second;
}

}

std::string_view toString(JsonAsk ask) noexcept
{
    switch (ask) {
    case JsonAsk::Words:
        return "words";
    case JsonAsk::ResponseFormat:
        return "response_format";
    case JsonAsk::Prefill:
        return "prefill";
    }
    return "words";
}

JsonAsk parseJsonAsk(std::string_view text, JsonAsk fallback) noexcept
{
    if (text == "words") {
        return JsonAsk::Words;
    }
    if (text == "response_format") {
        return JsonAsk::ResponseFormat;
    }
    if (text == "prefill") {
        return JsonAsk::Prefill;
    }
    return fallback;
}

std::string_view toString(Thinking thinking) noexcept
{
    switch (thinking) {
    case Thinking::AsTheModelLikes:
        return "as-the-model-likes";
    case Thinking::Off:
        return "off";
    case Thinking::Low:
        return "low";
    }
    return "as-the-model-likes";
}

Thinking parseThinking(std::string_view text, Thinking fallback) noexcept
{
    if (text == "as-the-model-likes") {
        return Thinking::AsTheModelLikes;
    }
    if (text == "off") {
        return Thinking::Off;
    }
    if (text == "low") {
        return Thinking::Low;
    }
    return fallback;
}

LlmDialect dialectFor(std::string_view adapter, std::string_view model)
{
    const auto name = folded(model);
    for (const auto &rule : rules()) {
        if (rule.adapter != adapter) {
            continue;
        }
        if (rule.name.empty() ||
            (rule.how == Match::Begins ? name.starts_with(rule.name)
                                       : name.find(rule.name) != std::string::npos)) {
            return rule.wants;
        }
    }

    return {};
}

LlmDialect dialectFor(std::string_view adapter, std::string_view model,
                      const std::vector<std::pair<std::string, std::string>> &params)
{
    auto wants = dialectFor(adapter, model);

    const auto limit = std::find_if(params.begin(), params.end(), [](const auto &pair) {
        return pair.first == dialect::kAnswerLimit;
    });
    if (limit != params.end()) {
        wants.answerLimit = limit->second;
    }

    if (const auto heat = valueOf(params, dialect::kTemperature); !heat.empty()) {
        wants.temperature = heat == "true" || heat == "1" || heat == "yes";
    }
    if (const auto json = valueOf(params, dialect::kJson); !json.empty()) {
        wants.json = parseJsonAsk(json, wants.json);
    }
    if (const auto thinking = valueOf(params, dialect::kThinking); !thinking.empty()) {
        wants.thinking = parseThinking(thinking, wants.thinking);
    }
    return wants;
}

namespace {

[[nodiscard]] bool mentions(const std::string &folded_text, std::string_view what)
{
    return folded_text.find(what) != std::string::npos;
}

constexpr std::string_view kLimitNames[] = {
    "max_completion_tokens",
    "max_output_tokens",
    "max_tokens",
};

}

std::vector<std::pair<std::string, std::string>> correctionsFor(std::string_view complaint,
                                                                const LlmDialect &now)
{
    std::vector<std::pair<std::string, std::string>> learnt;
    if (complaint.empty()) {
        return learnt;
    }
    const auto said = folded(complaint);

    if (!now.answerLimit.empty() && mentions(said, folded(now.answerLimit))) {
        for (const auto &name : kLimitNames) {
            if (name == now.answerLimit) {
                continue;
            }
            if (mentions(said, name)) {
                learnt.emplace_back(std::string{dialect::kAnswerLimit}, std::string{name});
                break;
            }
        }

        if (learnt.empty() && (mentions(said, "unsupported") || mentions(said, "not supported") ||
                               mentions(said, "not permitted"))) {
            learnt.emplace_back(std::string{dialect::kAnswerLimit}, std::string{});
        }
    }

    if (now.temperature && mentions(said, "temperature") &&
        (mentions(said, "unsupported") || mentions(said, "not supported") ||
         mentions(said, "not permitted") || mentions(said, "deprecated") ||
         mentions(said, "only the default"))) {
        learnt.emplace_back(std::string{dialect::kTemperature}, "false");
    }

    if (now.json == JsonAsk::ResponseFormat && mentions(said, "response_format")) {
        learnt.emplace_back(std::string{dialect::kJson}, std::string{toString(JsonAsk::Words)});
    }

    if (now.json == JsonAsk::Prefill &&
        (mentions(said, "prefill") || mentions(said, "end with a user message"))) {
        learnt.emplace_back(std::string{dialect::kJson}, std::string{toString(JsonAsk::Words)});
    }

    const auto refusedOutright = mentions(said, "unsupported") || mentions(said, "not supported") ||
                                 mentions(said, "not permitted") || mentions(said, "disabled") ||
                                 mentions(said, "extra inputs") || mentions(said, "not allowed") ||
                                 mentions(said, "input should");
    if (now.thinking == Thinking::Off && mentions(said, "thinking") && refusedOutright) {
        learnt.emplace_back(std::string{dialect::kThinking}, std::string{toString(Thinking::Low)});
    } else if (now.thinking == Thinking::Low &&
               (mentions(said, "effort") || mentions(said, "output_config")) && refusedOutright) {
        learnt.emplace_back(std::string{dialect::kThinking},
                            std::string{toString(Thinking::AsTheModelLikes)});
    }
    return learnt;
}

LlmAnswer askLearning(const std::function<LlmAnswer(const LlmEndpoint &)> &askOnce,
                      std::string_view adapter, const LlmEndpoint &where)
{

    const auto write = [](std::vector<std::pair<std::string, std::string>> &into,
                          const std::pair<std::string, std::string> &one) {
        const auto found = std::find_if(
            into.begin(), into.end(), [&one](const auto &pair) { return pair.first == one.first; });
        if (found == into.end()) {
            into.push_back(one);
        } else {
            found->second = one.second;
        }
    };

    constexpr int kRounds = 3;

    auto corrected = where;
    std::vector<std::pair<std::string, std::string>> learnt;

    for (int round = 0;; ++round) {
        auto answer = askOnce(corrected);

        const auto more = answer.status == LlmStatus::Refused && round + 1 < kRounds
                              ? correctionsFor(answer.detail, dialectFor(adapter, corrected.model,
                                                                         corrected.params))
                              : std::vector<std::pair<std::string, std::string>>{};
        if (more.empty()) {

            if (answer.status != LlmStatus::Refused) {
                answer.learnt = std::move(learnt);
            }
            return answer;
        }

        for (const auto &one : more) {
            write(corrected.params, one);
            write(learnt, one);
        }
    }
}

bool isDialectName(std::string_view name)
{
    return name == dialect::kAnswerLimit || name == dialect::kTemperature ||
           name == dialect::kJson || name == dialect::kThinking;
}

}
