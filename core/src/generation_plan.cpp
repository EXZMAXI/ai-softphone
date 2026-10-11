// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/generation_plan.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace softphone::core {
namespace {

[[nodiscard]] std::vector<std::string> turnsOf(const std::string &transcript)
{
    std::vector<std::string> turns;
    std::size_t at = 0;
    while (at <= transcript.size()) {
        const auto end = transcript.find('\n', at);
        if (end == std::string::npos) {
            if (at < transcript.size()) {
                turns.push_back(transcript.substr(at));
            }
            break;
        }
        turns.push_back(transcript.substr(at, end - at));
        at = end + 1;
    }
    return turns;
}

[[nodiscard]] std::string joined(const std::vector<std::string> &turns, std::size_t from,
                                 std::size_t upTo)
{
    std::string text;
    for (auto at = from; at < upTo; ++at) {
        if (!text.empty()) {
            text += '\n';
        }
        text += turns.at(at);
    }
    return text;
}

}

GenerationPlan planGeneration(const PromptTemplate &prompt, const PromptContext &context,
                              const LlmCapabilities &what, std::string_view interfaceLanguage,
                              const FoldBudget &budget)
{
    GenerationPlan plan;

    auto whole = renderPrompt(prompt, context, interfaceLanguage);
    if (!whole.unknown.empty()) {
        plan.refusal =
            "the prompt uses something that is not a placeholder: " + whole.unknown.front();
        return plan;
    }

    const auto answer = std::max(1, budget.answerTokens);
    const auto margin = static_cast<int>(std::llround(static_cast<double>(what.contextTokens) *
                                                      std::clamp(budget.margin, 0.0, 0.5)));
    const auto room = what.contextTokens - answer - margin;

    if (room <= 0) {
        plan.refusal = "the model's window has no room left for a prompt once the answer is "
                       "allowed for";
        return plan;
    }

    if (estimateTokens(whole.messages) <= room) {
        GenerationStep step;
        step.prompt = std::move(whole);
        const auto turns = turnsOf(context.transcript);
        step.lastTurn = static_cast<int>(turns.size());
        plan.steps.push_back(std::move(step));
        return plan;
    }

    auto without = context;
    without.transcript.clear();
    without.nearSide.clear();
    without.farSide.clear();
    const auto fixed = renderPrompt(prompt, without, interfaceLanguage);
    const auto overhead = estimateTokens(fixed.messages);

    const auto perPart = room - overhead;
    if (perPart <= 0) {
        plan.refusal = "the prompt is longer than the model's window before any of the "
                       "conversation is added to it";
        return plan;
    }

    const auto turns = turnsOf(context.transcript);
    if (turns.empty()) {
        plan.refusal = "there is nothing to read";
        return plan;
    }

    std::vector<std::pair<std::size_t, std::size_t>> cuts;
    std::size_t from = 0;
    int running = 0;

    for (std::size_t at = 0; at < turns.size(); ++at) {
        const auto cost = estimateTokens(turns.at(at)) + 1;

        if (cost > perPart && at == from) {

            plan.refusal = "one turn of the conversation is longer than the model will read at "
                           "once";
            return plan;
        }

        if (running + cost > perPart && at > from) {
            cuts.emplace_back(from, at);
            from = at;
            running = 0;
        }
        running += cost;
    }
    cuts.emplace_back(from, turns.size());

    plan.folded = cuts.size() > 1;

    int number = 1;
    for (const auto &[first, upTo] : cuts) {
        auto part = context;
        part.transcript = joined(turns, first, upTo);

        part.nearSide.clear();
        part.farSide.clear();

        GenerationStep step;
        step.prompt = renderPrompt(prompt, part, interfaceLanguage);
        step.part = number++;
        step.of = static_cast<int>(cuts.size());
        step.firstTurn = static_cast<int>(first) + 1;
        step.lastTurn = static_cast<int>(upTo);
        plan.steps.push_back(std::move(step));
    }

    return plan;
}

int answerRoomFor(const RenderedPrompt &prompt, const LlmCapabilities &what,
                  const FoldBudget &budget)
{

    if (what.maxOutputTokens <= 0) {
        return 0;
    }

    const auto reserve = std::max(1, std::min(budget.answerTokens, what.maxOutputTokens));

    const auto margin = static_cast<int>(std::llround(static_cast<double>(what.contextTokens) *
                                                      std::clamp(budget.margin, 0.0, 0.5)));
    const auto left = what.contextTokens - estimateTokens(prompt.messages) - margin;

    return std::clamp(left, reserve, what.maxOutputTokens);
}

RenderedPrompt foldAnswers(const PromptTemplate &prompt, const std::vector<std::string> &answers,
                           const PromptContext &context, std::string_view interfaceLanguage)
{

    auto folded = prompt;

    folded.body = "Below are answers about consecutive parts of one telephone call, in order. "
                  "They are not separate calls. Produce a single answer of the same kind about "
                  "the call as a whole, under the same rules the parts were answered under, "
                  "including how often a thing may appear in it. Where several parts say the "
                  "same thing, that is one entry and not several; where they disagree, follow "
                  "the later part.\n";

    int number = 1;
    for (const auto &answer : answers) {
        folded.body += "\n--- part " + std::to_string(number++) + " of " +
                       std::to_string(answers.size()) + " ---\n" + answer + "\n";
    }

    auto without = context;
    without.transcript.clear();
    without.nearSide.clear();
    without.farSide.clear();

    return renderPrompt(folded, without, interfaceLanguage);
}

}
