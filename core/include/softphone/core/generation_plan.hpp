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
#include <softphone/core/templates.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

struct GenerationStep {
    RenderedPrompt prompt;

    int part{1};
    int of{1};

    int firstTurn{0};
    int lastTurn{0};
};

struct GenerationPlan {
    std::vector<GenerationStep> steps;

    bool folded{false};

    std::string refusal;

    [[nodiscard]] bool usable() const noexcept
    {
        return refusal.empty() && !steps.empty();
    }
};

struct FoldBudget {

    int answerTokens{1024};

    double margin{0.1};
};

[[nodiscard]] GenerationPlan planGeneration(const PromptTemplate &prompt,
                                            const PromptContext &context,
                                            const LlmCapabilities &what,
                                            std::string_view interfaceLanguage,
                                            const FoldBudget &budget = {});

[[nodiscard]] int answerRoomFor(const RenderedPrompt &prompt, const LlmCapabilities &what,
                                const FoldBudget &budget = {});

[[nodiscard]] RenderedPrompt foldAnswers(const PromptTemplate &prompt,
                                         const std::vector<std::string> &answers,
                                         const PromptContext &context,
                                         std::string_view interfaceLanguage);

}
