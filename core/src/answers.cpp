// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/answers.hpp>

#include <softphone/core/llm.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <string>

namespace softphone::core {
namespace {

[[nodiscard]] Answer refuse(AnswerShape shape, std::string problem)
{
    Answer answer;
    answer.shape = shape;
    answer.problem = std::move(problem);
    return answer;
}

[[nodiscard]] std::string atText(std::int64_t ms)
{
    const auto whole = ms < 0 ? 0 : ms / 1000;
    std::array<char, 32> text{};
    std::snprintf(text.data(), text.size(), "%lld:%02lld", static_cast<long long>(whole / 60),
                  static_cast<long long>(whole % 60));
    return std::string{text.data()};
}

[[nodiscard]] nlohmann::json::const_iterator fieldIn(const nlohmann::json &object, const char *name)
{
    if (const auto exact = object.find(name); exact != object.end()) {
        return exact;
    }
    if (!object.is_object()) {
        return object.end();
    }

    const auto letters = [](std::string_view text) {
        std::string out;
        for (const char letter : text) {

            const auto byte = static_cast<unsigned char>(letter);
            if (std::isalnum(byte) != 0) {
                out.push_back(static_cast<char>(std::tolower(byte)));
            }
        }
        return out;
    };

    const auto wanted = letters(name);
    if (wanted.empty()) {
        return object.end();
    }

    for (auto one = object.begin(); one != object.end(); ++one) {
        if (letters(one.key()) == wanted) {
            return one;
        }
    }
    return object.end();
}

[[nodiscard]] std::string textOf(const nlohmann::json &object, const char *name)
{
    const auto found = fieldIn(object, name);
    if (found == object.end() || found->is_null()) {
        return {};
    }
    if (found->is_string()) {
        return found->get<std::string>();
    }
    return found->dump();
}

[[nodiscard]] std::optional<std::int64_t> timeOf(const nlohmann::json &object, const char *name)
{
    const auto found = fieldIn(object, name);
    if (found == object.end() || !found->is_number()) {
        return std::nullopt;
    }
    const auto at = found->get<std::int64_t>();

    return at < 0 ? std::nullopt : std::optional<std::int64_t>{at};
}

[[nodiscard]] std::vector<AnswerEvidence> evidenceIn(const nlohmann::json &object, const char *name)
{
    std::vector<AnswerEvidence> out;
    const auto found = fieldIn(object, name);
    if (found == object.end() || !found->is_array()) {
        return out;
    }
    for (const auto &entry : *found) {
        if (!entry.is_object()) {
            continue;
        }
        AnswerEvidence one;
        one.quote = textOf(entry, "quote");
        one.atMs = timeOf(entry, "at_ms");
        if (!one.quote.empty()) {
            out.push_back(std::move(one));
        }
    }
    return out;
}

[[nodiscard]] std::optional<nlohmann::json> objectIn(std::string_view text)
{
    const auto found = firstJsonValue(text);
    if (found.empty()) {
        return std::nullopt;
    }
    auto parsed = nlohmann::json::parse(found, nullptr, false);
    if (parsed.is_discarded()) {
        return std::nullopt;
    }
    return parsed;
}

[[nodiscard]] std::optional<int> scoreIn(const nlohmann::json &object, const char *name)
{
    const auto found = fieldIn(object, name);
    if (found == object.end() || !found->is_number()) {
        return std::nullopt;
    }
    return static_cast<int>(std::llround(found->get<double>()));
}

}

Answer readAnswer(AnswerShape shape, std::string_view text)
{
    Answer answer;
    answer.shape = shape;

    if (shape == AnswerShape::Plain) {
        answer.text = std::string{text};

        while (!answer.text.empty() &&
               std::isspace(static_cast<unsigned char>(answer.text.front()))) {
            answer.text.erase(answer.text.begin());
        }
        while (!answer.text.empty() &&
               std::isspace(static_cast<unsigned char>(answer.text.back()))) {
            answer.text.pop_back();
        }
        if (answer.text.empty()) {
            return refuse(shape, "the model answered with nothing");
        }
        return answer;
    }

    const auto parsed = objectIn(text);
    if (!parsed.has_value()) {
        return refuse(shape, "the model did not answer in the shape it was asked for");
    }
    const auto &object = *parsed;

    switch (shape) {
    case AnswerShape::Plain:

    case AnswerShape::Moments:
        break;

    case AnswerShape::Json: {

        answer.json = object.dump(2);
        return answer;
    }

    case AnswerShape::List: {
        const auto items = fieldIn(object, "items");
        if (items == object.end() || !items->is_array()) {
            return refuse(shape, "the answer carried no list of items");
        }
        for (const auto &entry : *items) {
            AnswerItem one;
            if (entry.is_string()) {

                one.text = entry.get<std::string>();
            } else if (entry.is_object()) {
                one.text = textOf(entry, "text");
                one.owner = textOf(entry, "owner");
                one.due = textOf(entry, "due");
            }
            if (!one.text.empty()) {
                answer.items.push_back(std::move(one));
            }
        }
        break;
    }

    case AnswerShape::Labels: {
        const auto labels = fieldIn(object, "labels");
        if (labels == object.end() || !labels->is_array()) {
            return refuse(shape, "the answer carried no codes");
        }
        for (const auto &entry : *labels) {
            AnswerLabel one;
            if (entry.is_string()) {
                one.code = entry.get<std::string>();
            } else if (entry.is_object()) {
                one.code = textOf(entry, "code");
                if (const auto sure = fieldIn(entry, "confidence");
                    sure != entry.end() && sure->is_number()) {
                    one.confidence = std::clamp(sure->get<double>(), 0.0, 1.0);
                }
            }
            if (!one.code.empty()) {
                answer.labels.push_back(std::move(one));
            }
        }
        break;
    }

    case AnswerShape::Score: {
        answer.score = scoreIn(object, "score");
        if (!answer.score.has_value()) {
            return refuse(shape, "the answer carried no score");
        }
        answer.rationale = textOf(object, "rationale");
        answer.evidence = evidenceIn(object, "evidence");
        break;
    }

    case AnswerShape::Rubric: {
        answer.score = scoreIn(object, "overall");
        if (!answer.score.has_value()) {
            return refuse(shape, "the answer carried no overall score");
        }
        answer.rationale = textOf(object, "rationale");

        const auto criteria = fieldIn(object, "criteria");
        if (criteria == object.end() || !criteria->is_array()) {
            return refuse(shape, "the answer judged no criteria");
        }
        for (const auto &entry : *criteria) {
            if (!entry.is_object()) {
                continue;
            }
            AnswerCriterion one;
            one.name = textOf(entry, "name");
            one.verdict = textOf(entry, "verdict");
            one.note = textOf(entry, "note");
            if (!one.name.empty()) {
                answer.criteria.push_back(std::move(one));
            }
        }
        if (answer.criteria.empty()) {
            return refuse(shape, "the answer judged no criteria");
        }
        break;
    }

    case AnswerShape::Flags: {
        const auto flags = fieldIn(object, "flags");
        if (flags == object.end() || !flags->is_array()) {
            return refuse(shape, "the answer carried no flags");
        }
        for (const auto &entry : *flags) {
            if (!entry.is_object()) {
                continue;
            }
            AnswerFlag one;
            one.code = textOf(entry, "code");
            one.severity = parseSeverity(textOf(entry, "severity")).value_or(Severity::Medium);
            one.evidence = textOf(entry, "evidence");
            one.atMs = timeOf(entry, "at_ms");
            if (!one.code.empty()) {
                answer.flags.push_back(std::move(one));
            }
        }
        break;
    }

    case AnswerShape::Qa: {
        answer.text = textOf(object, "answer");
        if (answer.text.empty()) {
            return refuse(shape, "the answer said nothing");
        }
        answer.evidence = evidenceIn(object, "citations");
        break;
    }
    }

    if (answer.json.empty()) {
        nlohmann::ordered_json out;
        switch (shape) {
        case AnswerShape::List: {
            auto items = nlohmann::ordered_json::array();
            for (const auto &item : answer.items) {
                items.push_back({{"text", item.text}, {"owner", item.owner}, {"due", item.due}});
            }
            out["items"] = std::move(items);
            break;
        }
        case AnswerShape::Labels: {
            auto labels = nlohmann::ordered_json::array();
            for (const auto &label : answer.labels) {
                nlohmann::ordered_json one{{"code", label.code}};
                if (label.confidence.has_value()) {
                    one["confidence"] = *label.confidence;
                }
                labels.push_back(std::move(one));
            }
            out["labels"] = std::move(labels);
            break;
        }
        case AnswerShape::Score: {
            out["score"] = *answer.score;
            out["rationale"] = answer.rationale;
            auto evidence = nlohmann::ordered_json::array();
            for (const auto &one : answer.evidence) {
                nlohmann::ordered_json entry{{"quote", one.quote}};
                if (one.atMs.has_value()) {
                    entry["at_ms"] = *one.atMs;
                }
                evidence.push_back(std::move(entry));
            }
            out["evidence"] = std::move(evidence);
            break;
        }
        case AnswerShape::Rubric: {
            out["overall"] = *answer.score;
            out["rationale"] = answer.rationale;
            auto criteria = nlohmann::ordered_json::array();
            for (const auto &one : answer.criteria) {
                criteria.push_back(
                    {{"name", one.name}, {"verdict", one.verdict}, {"note", one.note}});
            }
            out["criteria"] = std::move(criteria);
            break;
        }
        case AnswerShape::Flags: {
            auto flags = nlohmann::ordered_json::array();
            for (const auto &one : answer.flags) {
                nlohmann::ordered_json entry{{"code", one.code},
                                             {"severity", std::string{toString(one.severity)}},
                                             {"evidence", one.evidence}};
                if (one.atMs.has_value()) {
                    entry["at_ms"] = *one.atMs;
                }
                flags.push_back(std::move(entry));
            }
            out["flags"] = std::move(flags);
            break;
        }
        case AnswerShape::Qa: {
            out["answer"] = answer.text;
            auto citations = nlohmann::ordered_json::array();
            for (const auto &one : answer.evidence) {
                nlohmann::ordered_json entry{{"quote", one.quote}};
                if (one.atMs.has_value()) {
                    entry["at_ms"] = *one.atMs;
                }
                citations.push_back(std::move(entry));
            }
            out["citations"] = std::move(citations);
            break;
        }
        case AnswerShape::Plain:
        case AnswerShape::Json:
        case AnswerShape::Moments:
            break;
        }
        answer.json = out.dump(2);
    }

    return answer;
}

namespace {

[[nodiscard]] std::string named(const AnswerWords &words, std::string_view kind,
                                const std::string &code)
{
    return words.titleOf ? words.titleOf(kind, code) : code;
}

[[nodiscard]] std::string howBad(const AnswerWords &words, Severity severity)
{
    return words.severityOf ? words.severityOf(severity) : std::string{toString(severity)};
}

[[nodiscard]] std::size_t rowsIn(const Answer &answer)
{
    switch (answer.shape) {
    case AnswerShape::List:
        return answer.items.size();
    case AnswerShape::Labels:
        return answer.labels.size();
    case AnswerShape::Rubric:
        return answer.criteria.size();
    case AnswerShape::Flags:
        return answer.flags.size();
    case AnswerShape::Score:
    case AnswerShape::Qa:
        return answer.evidence.size();
    case AnswerShape::Plain:
    case AnswerShape::Json:
    case AnswerShape::Moments:
        break;
    }
    return 0;
}

}

std::string renderAnswerEntry(const Answer &answer, int index, const AnswerWords &words)
{
    if (index < 0 || static_cast<std::size_t>(index) >= rowsIn(answer)) {
        return {};
    }
    const auto at = static_cast<std::size_t>(index);

    switch (answer.shape) {
    case AnswerShape::List: {
        const auto &item = answer.items[at];
        std::string text = "• " + item.text;
        if (!item.owner.empty()) {
            text += " — " + item.owner;
        }
        if (!item.due.empty()) {
            text += " (" + item.due + ")";
        }
        return text;
    }

    case AnswerShape::Labels:

        return words.labelKind.empty() ? answer.labels[at].code
                                       : named(words, words.labelKind, answer.labels[at].code);

    case AnswerShape::Rubric: {
        const auto &one = answer.criteria[at];
        std::string text =
            one.name + ": " + (words.verdictOf ? words.verdictOf(one.verdict) : one.verdict);
        if (!one.note.empty()) {
            text += " — " + one.note;
        }
        return text;
    }

    case AnswerShape::Flags: {
        const auto &one = answer.flags[at];
        std::string text =
            named(words, "red_flag", one.code) + " (" + howBad(words, one.severity) + ")";
        if (!one.evidence.empty()) {
            text += ": " + one.evidence;
        }
        if (one.atMs.has_value()) {
            text += " — " + atText(*one.atMs);
        }
        return text;
    }

    case AnswerShape::Score:
    case AnswerShape::Qa: {
        const auto &one = answer.evidence[at];
        std::string text = "“" + one.quote + "”";
        if (one.atMs.has_value()) {
            text += " — " + atText(*one.atMs);
        }
        return text;
    }

    case AnswerShape::Plain:
    case AnswerShape::Json:
    case AnswerShape::Moments:
        break;
    }
    return {};
}

std::string renderAnswer(const Answer &answer, const AnswerWords &words)
{

    const auto row = [&answer, &words](std::size_t at) {
        return renderAnswerEntry(answer, static_cast<int>(at), words);
    };
    const auto rows = rowsIn(answer);

    switch (answer.shape) {
    case AnswerShape::Plain:

    case AnswerShape::Moments:
        return answer.text;

    case AnswerShape::Json:
        return answer.json;

    case AnswerShape::List: {
        std::string text;
        for (std::size_t at = 0; at < rows; ++at) {
            text += row(at) + '\n';
        }
        return text;
    }

    case AnswerShape::Labels: {
        std::string text;
        for (std::size_t at = 0; at < rows; ++at) {
            if (!text.empty()) {
                text += ", ";
            }
            text += row(at);
        }
        return text;
    }

    case AnswerShape::Score: {
        std::string text = std::to_string(answer.score.value_or(0));
        if (!answer.rationale.empty()) {
            text += "\n\n" + answer.rationale;
        }
        for (std::size_t at = 0; at < rows; ++at) {
            text += "\n\n" + row(at);
        }
        return text;
    }

    case AnswerShape::Rubric: {
        std::string text = std::to_string(answer.score.value_or(0));
        if (!answer.rationale.empty()) {
            text += "\n\n" + answer.rationale;
        }
        text += '\n';
        for (std::size_t at = 0; at < rows; ++at) {
            text += "\n" + row(at);
        }
        return text;
    }

    case AnswerShape::Flags: {
        std::string text;
        for (std::size_t at = 0; at < rows; ++at) {
            text += row(at) + '\n';
        }
        return text;
    }

    case AnswerShape::Qa: {
        std::string text = answer.text;
        for (std::size_t at = 0; at < rows; ++at) {
            text += "\n\n" + row(at);
        }
        return text;
    }
    }
    return {};
}

namespace {

[[nodiscard]] bool defines(const std::vector<TaxonomyEntry> &known, const std::string &code)
{
    return std::any_of(known.begin(), known.end(),
                       [&code](const TaxonomyEntry &entry) { return entry.code == code; });
}

}

std::vector<std::string> unknownCodes(const std::vector<AnswerLabel> &labels,
                                      const std::vector<TaxonomyEntry> &known)
{
    std::vector<std::string> out;
    for (const auto &label : labels) {
        if (!defines(known, label.code) &&
            std::find(out.begin(), out.end(), label.code) == out.end()) {
            out.push_back(label.code);
        }
    }
    return out;
}

std::vector<std::string> unknownCodes(const std::vector<AnswerFlag> &flags,
                                      const std::vector<TaxonomyEntry> &known)
{
    std::vector<std::string> out;
    for (const auto &flag : flags) {
        if (!defines(known, flag.code) &&
            std::find(out.begin(), out.end(), flag.code) == out.end()) {
            out.push_back(flag.code);
        }
    }
    return out;
}

}
