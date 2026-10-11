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

#include <softphone/core/taxonomy.hpp>
#include <softphone/core/templates.hpp>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

struct AnswerItem {
    std::string text;
    std::string owner;
    std::string due;
};

struct AnswerLabel {
    std::string code;

    std::optional<double> confidence;
};

struct AnswerEvidence {
    std::optional<std::int64_t> atMs;
    std::string quote;
};

struct AnswerCriterion {
    std::string name;

    std::string verdict;
    std::string note;
};

struct AnswerFlag {
    std::string code;
    Severity severity{Severity::Medium};
    std::string evidence;
    std::optional<std::int64_t> atMs;
};

struct Answer {
    AnswerShape shape{AnswerShape::Plain};

    std::string text;

    std::vector<AnswerItem> items;
    std::vector<AnswerLabel> labels;

    std::optional<int> score;
    std::string rationale;

    std::vector<AnswerEvidence> evidence;
    std::vector<AnswerCriterion> criteria;
    std::vector<AnswerFlag> flags;

    std::string json;

    std::string problem;

    [[nodiscard]] bool ok() const noexcept
    {
        return problem.empty();
    }
};

[[nodiscard]] Answer readAnswer(AnswerShape shape, std::string_view text);

struct AnswerWords {

    std::function<std::string(std::string_view kind, std::string_view code)> titleOf;

    std::function<std::string(Severity)> severityOf;

    std::function<std::string(std::string_view)> verdictOf;

    std::string labelKind;
};

[[nodiscard]] std::string renderAnswer(const Answer &answer, const AnswerWords &words = {});

[[nodiscard]] std::string renderAnswerEntry(const Answer &answer, int index,
                                            const AnswerWords &words = {});

[[nodiscard]] std::vector<std::string> unknownCodes(const std::vector<AnswerLabel> &labels,
                                                    const std::vector<TaxonomyEntry> &known);

[[nodiscard]] std::vector<std::string> unknownCodes(const std::vector<AnswerFlag> &flags,
                                                    const std::vector<TaxonomyEntry> &known);

}
