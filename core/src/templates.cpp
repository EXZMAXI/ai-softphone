// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/templates.hpp>

#include "shipped_content.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>

namespace softphone::core {
namespace {

constexpr std::string_view kArtifactPrefix = "artifact:";

[[nodiscard]] std::string minutesAndSeconds(std::chrono::milliseconds duration)
{
    const auto whole = duration.count() < 0 ? 0 : duration.count() / 1000;
    std::array<char, 32> text{};
    std::snprintf(text.data(), text.size(), "%lld:%02lld", static_cast<long long>(whole / 60),
                  static_cast<long long>(whole % 60));
    return std::string{text.data()};
}

[[nodiscard]] std::string instructionText(const PromptContext &context, std::string_view code,
                                          const std::string &shipped)
{
    const auto found = std::find_if(context.instructions.begin(), context.instructions.end(),
                                    [code](const TaxonomyEntry &one) { return one.code == code; });
    return found == context.instructions.end() ? shipped : found->description;
}

[[nodiscard]] std::string askedLanguage(const PromptTemplate &prompt,
                                        std::string_view interfaceLanguage)
{
    switch (prompt.languagePolicy) {
    case LanguagePolicy::Interface:
        return std::string{interfaceLanguage};
    case LanguagePolicy::Fixed:
        return prompt.language;
    case LanguagePolicy::AsSpoken:
        break;
    }
    return {};
}

[[nodiscard]] std::string languageInstruction(const PromptTemplate &prompt,
                                              const PromptContext &context,
                                              std::string_view interfaceLanguage)
{

    const auto named = [&context](std::string_view code, const char *shipped,
                                  const std::string &language) {
        auto text = instructionText(context, code, shipped);
        for (auto at = text.find("{language}"); at != std::string::npos;
             at = text.find("{language}", at)) {
            text.replace(at, std::strlen("{language}"), language);
            at += language.size();
        }
        return text;
    };

    if (const auto asked = askedLanguage(prompt, interfaceLanguage); !asked.empty()) {

        return named("language-named", "Write your answer in this language: {language}.",
                     std::string{languageName(asked)});
    }

    if (context.language.empty()) {
        return instructionText(context, "language-as-spoken",
                               "Write your answer in the same language the conversation was in.");
    }
    return named("language-as-spoken-named",
                 "Write your answer in the same language the conversation was in, which is "
                 "{language}.",
                 std::string{languageName(context.language)});
}

}

std::string_view toString(ArtifactKind kind) noexcept
{
    switch (kind) {
    case ArtifactKind::Summary:
        return "summary";
    case ArtifactKind::SuperSummary:
        return "super_summary";
    case ArtifactKind::ActionItems:
        return "action_items";
    case ArtifactKind::Topics:
        return "topics";
    case ArtifactKind::Entities:
        return "entities";
    case ArtifactKind::Categorise:
        return "categorise";
    case ArtifactKind::Tags:
        return "tags";
    case ArtifactKind::RedFlags:
        return "red_flags";
    case ArtifactKind::Translation:
        return "translation";
    case ArtifactKind::Qa:
        return "qa";
    case ArtifactKind::Score:
        return "score";
    case ArtifactKind::Realtime:
        return "realtime";
    case ArtifactKind::Custom:
        return "custom";
    }
    return "custom";
}

std::optional<ArtifactKind> parseArtifactKind(std::string_view text) noexcept
{
    static constexpr std::array<std::pair<std::string_view, ArtifactKind>, 13> kAll{{
        {"realtime", ArtifactKind::Realtime},
        {"summary", ArtifactKind::Summary},
        {"super_summary", ArtifactKind::SuperSummary},
        {"action_items", ArtifactKind::ActionItems},
        {"topics", ArtifactKind::Topics},
        {"entities", ArtifactKind::Entities},
        {"categorise", ArtifactKind::Categorise},
        {"tags", ArtifactKind::Tags},
        {"red_flags", ArtifactKind::RedFlags},
        {"translation", ArtifactKind::Translation},
        {"qa", ArtifactKind::Qa},
        {"score", ArtifactKind::Score},
        {"custom", ArtifactKind::Custom},
    }};
    for (const auto &[name, kind] : kAll) {
        if (text == name) {
            return kind;
        }
    }
    return std::nullopt;
}

std::string_view toString(AnswerShape shape) noexcept
{
    switch (shape) {
    case AnswerShape::Plain:
        return "plain";
    case AnswerShape::List:
        return "list";
    case AnswerShape::Labels:
        return "labels";
    case AnswerShape::Score:
        return "score";
    case AnswerShape::Rubric:
        return "rubric";
    case AnswerShape::Flags:
        return "flags";
    case AnswerShape::Qa:
        return "qa";
    case AnswerShape::Json:
        return "json";
    case AnswerShape::Moments:
        return "moments";
    }
    return "plain";
}

std::optional<AnswerShape> parseAnswerShape(std::string_view text) noexcept
{
    static constexpr std::array<std::pair<std::string_view, AnswerShape>, 9> kAll{{
        {"plain", AnswerShape::Plain},
        {"list", AnswerShape::List},
        {"labels", AnswerShape::Labels},
        {"score", AnswerShape::Score},
        {"rubric", AnswerShape::Rubric},
        {"flags", AnswerShape::Flags},
        {"qa", AnswerShape::Qa},
        {"json", AnswerShape::Json},
        {"moments", AnswerShape::Moments},
    }};
    for (const auto &[name, shape] : kAll) {
        if (text == name) {
            return shape;
        }
    }
    return std::nullopt;
}

std::string_view toString(LanguagePolicy policy) noexcept
{
    switch (policy) {
    case LanguagePolicy::AsSpoken:
        return "as_spoken";
    case LanguagePolicy::Interface:
        return "interface";
    case LanguagePolicy::Fixed:
        return "fixed";
    }
    return "as_spoken";
}

std::optional<LanguagePolicy> parseLanguagePolicy(std::string_view text) noexcept
{
    if (text == "as_spoken") {
        return LanguagePolicy::AsSpoken;
    }
    if (text == "interface") {
        return LanguagePolicy::Interface;
    }
    if (text == "fixed") {
        return LanguagePolicy::Fixed;
    }
    return std::nullopt;
}

std::string_view toString(RealtimeMode mode) noexcept
{
    switch (mode) {
    case RealtimeMode::Translate:
        return "translate";
    case RealtimeMode::Prompt:
        return "prompt";
    case RealtimeMode::Knowledge:
        return "knowledge";
    case RealtimeMode::Subtitles:
        break;
    }
    return "subtitles";
}

std::optional<RealtimeMode> parseRealtimeMode(std::string_view text) noexcept
{
    static constexpr std::array<std::pair<std::string_view, RealtimeMode>, 4> kAll{{
        {"subtitles", RealtimeMode::Subtitles},
        {"translate", RealtimeMode::Translate},
        {"prompt", RealtimeMode::Prompt},
        {"knowledge", RealtimeMode::Knowledge},
    }};
    for (const auto &[name, mode] : kAll) {
        if (name == text) {
            return mode;
        }
    }
    return std::nullopt;
}

std::string_view toString(TurnPolicy policy) noexcept
{
    switch (policy) {
    case TurnPolicy::Silence:
        return "silence";
    case TurnPolicy::Manual:
        return "manual";
    case TurnPolicy::Provider:
        break;
    }
    return "provider";
}

std::optional<TurnPolicy> parseTurnPolicy(std::string_view text) noexcept
{
    static constexpr std::array<std::pair<std::string_view, TurnPolicy>, 3> kAll{{
        {"provider", TurnPolicy::Provider},
        {"silence", TurnPolicy::Silence},
        {"manual", TurnPolicy::Manual},
    }};
    for (const auto &[name, policy] : kAll) {
        if (name == text) {
            return policy;
        }
    }
    return std::nullopt;
}

bool wantsAModel(const PromptTemplate &prompt) noexcept
{
    if (!isRealtime(prompt)) {

        return true;
    }

    return !prompt.body.empty();
}

RealtimeMode modeFor(const PromptTemplate &prompt) noexcept
{
    return prompt.body.empty() ? RealtimeMode::Subtitles : RealtimeMode::Prompt;
}

std::vector<std::string> knownPlaceholders()
{

    return {"transcript", "transcript.near", "transcript.far",  "duration",   "direction",
            "party",      "account",         "language",        "categories", "tags",
            "red_flags",  "question",        "artifact:<kind>", "reply",      "conversation"};
}

bool asksAQuestion(const PromptTemplate &prompt)
{

    std::size_t at = 0;
    while (at < prompt.body.size()) {
        const auto opening = prompt.body.find("{{", at);
        if (opening == std::string::npos) {
            return false;
        }
        const auto closing = prompt.body.find("}}", opening + 2);
        if (closing == std::string::npos) {
            return false;
        }
        auto name = prompt.body.substr(opening + 2, closing - opening - 2);
        while (!name.empty() && (name.front() == ' ' || name.front() == '\t')) {
            name.erase(name.begin());
        }
        while (!name.empty() && (name.back() == ' ' || name.back() == '\t')) {
            name.pop_back();
        }
        if (name == "question") {
            return true;
        }
        at = closing + 2;
    }
    return false;
}

std::string renderDictionary(const std::vector<TaxonomyEntry> &entries)
{
    std::string text;
    for (const auto &entry : entries) {

        if (entry.retired()) {
            continue;
        }
        text += entry.code;
        if (!entry.title.empty()) {
            text += " — " + entry.title;
        }
        if (!entry.description.empty()) {
            text += ": " + entry.description;
        }
        text += '\n';
    }
    return text;
}

std::string_view instructionCode(AnswerShape shape) noexcept
{
    switch (shape) {
    case AnswerShape::Plain:
        return "shape-plain";
    case AnswerShape::List:
        return "shape-list";
    case AnswerShape::Labels:
        return "shape-labels";
    case AnswerShape::Score:
        return "shape-score";
    case AnswerShape::Rubric:
        return "shape-rubric";
    case AnswerShape::Flags:
        return "shape-flags";
    case AnswerShape::Qa:
        return "shape-qa";
    case AnswerShape::Json:
        return "shape-json";
    case AnswerShape::Moments:

        return "shape-plain";
    }
    return "shape-plain";
}

std::string shapeInstruction(AnswerShape shape)
{
    switch (shape) {
    case AnswerShape::Plain:
        return "Answer in plain prose. Do not use JSON, lists or headings.";

    case AnswerShape::List:
        return R"(Answer with JSON only, of exactly this shape:
{"items": [{"text": "what is to be done", "owner": "who, or empty", "due": "when, or empty"}]}
Use an empty array when there is nothing to list. Do not write anything outside the JSON.)";

    case AnswerShape::Labels:
        return R"(Answer with JSON only, of exactly this shape:
{"labels": [{"code": "a code from the list you were given", "confidence": 0.0}]}
Use only codes from the list. Use an empty array when none of them fits. Do not
invent a code, and do not write anything outside the JSON.)";

    case AnswerShape::Score:
        return R"(Answer with JSON only, of exactly this shape:
{"score": 3, "rationale": "why that number and not one higher",
 "evidence": [{"at_ms": 0, "quote": "the words you based it on"}]}
The score is a whole number from 1 to 5. Do not write anything outside the JSON.)";

    case AnswerShape::Rubric:
        return R"(Answer with JSON only, of exactly this shape:
{"overall": 3, "rationale": "why that number and not one higher",
 "criteria": [{"name": "the criterion as it was given to you",
               "verdict": "pass" or "weak" or "fail" or "n/a",
               "note": "one sentence"}]}
Judge every criterion you were given and no others. The overall score is a whole
number from 1 to 5. Do not write anything outside the JSON.)";

    case AnswerShape::Flags:
        return R"(Answer with JSON only, of exactly this shape:
{"flags": [{"code": "a code from the list you were given",
            "severity": "low" or "medium" or "high",
            "evidence": "the words you based it on",
            "at_ms": 0}]}
Use only codes from the list, and raise each one at most once. Use an empty
array when nothing applies. Do not write anything outside the JSON.)";

    case AnswerShape::Qa:
        return R"(Answer with JSON only, of exactly this shape:
{"answer": "your answer, or say plainly that the conversation does not say",
 "citations": [{"at_ms": 0, "quote": "the words your answer rests on"}]}
Do not answer from anything but the conversation. Do not write anything outside
the JSON.)";

    case AnswerShape::Json:
        return "Answer with JSON only, in the shape asked for above. Do not write anything "
               "outside the JSON.";

    case AnswerShape::Moments:

        return {};
    }
    return {};
}

std::string_view languageName(std::string_view code) noexcept
{

    struct Named {
        std::string_view code;
        std::string_view name;
    };

    static constexpr Named kNames[] = {
        {"en", "English"},
        {"ru", "Russian"},
        {"de", "German"},
        {"fr", "French"},
        {"es", "Spanish"},
        {"it", "Italian"},
        {"pt", "Portuguese"},
        {"nl", "Dutch"},
        {"sv", "Swedish"},
        {"da", "Danish"},
        {"nb", "Norwegian"},
        {"fi", "Finnish"},
        {"pl", "Polish"},
        {"cs", "Czech"},
        {"sk", "Slovak"},
        {"hu", "Hungarian"},
        {"ro", "Romanian"},
        {"bg", "Bulgarian"},
        {"hr", "Croatian"},
        {"sl", "Slovenian"},
        {"et", "Estonian"},
        {"lv", "Latvian"},
        {"lt", "Lithuanian"},
        {"el", "Greek"},
        {"tr", "Turkish"},
        {"uk", "Ukrainian"},
        {"id", "Indonesian"},
        {"vi", "Vietnamese"},
        {"sr", "Serbian written in Cyrillic"},
        {"sr_Latn", "Serbian written in the Latin alphabet"},
        {"zh", "Chinese"},
        {"ja", "Japanese"},
        {"ko", "Korean"},
        {"ar", "Arabic"},
        {"he", "Hebrew"},
        {"hi", "Hindi"},
    };

    for (const auto &named : kNames) {
        if (named.code == code) {
            return named.name;
        }
    }

    return code;
}

std::string answerLanguage(const PromptTemplate &prompt, const PromptContext &context,
                           std::string_view interfaceLanguage)
{

    auto asked = askedLanguage(prompt, interfaceLanguage);
    return asked.empty() ? context.language : asked;
}

RenderedPrompt renderPrompt(const PromptTemplate &prompt, const PromptContext &context,
                            std::string_view interfaceLanguage)
{
    RenderedPrompt out;

    const auto valueOf = [&prompt, &context, interfaceLanguage](const std::string &name,
                                                                bool &known) -> std::string {
        known = true;
        if (name == "transcript") {
            return context.transcript;
        }
        if (name == "transcript.near") {
            return context.nearSide;
        }
        if (name == "transcript.far") {
            return context.farSide;
        }
        if (name == "duration") {
            return minutesAndSeconds(context.duration);
        }
        if (name == "direction") {
            return context.direction;
        }
        if (name == "party") {
            return context.party;
        }
        if (name == "account") {
            return context.account;
        }
        if (name == "language") {

            const auto answering = answerLanguage(prompt, context, interfaceLanguage);
            const auto named = languageName(answering);
            return std::string{named.data(), named.size()};
        }
        if (name == "question") {
            return context.question;
        }

        if (name == "reply") {
            return context.reply;
        }
        if (name == "conversation") {
            return context.conversation;
        }
        if (name == "categories") {
            return renderDictionary(context.categories);
        }
        if (name == "tags") {
            return renderDictionary(context.tags);
        }
        if (name == "red_flags") {
            return renderDictionary(context.redFlags);
        }
        if (name.rfind(kArtifactPrefix, 0) == 0) {
            const auto kind = name.substr(kArtifactPrefix.size());

            if (!parseArtifactKind(kind).has_value()) {
                known = false;
                return {};
            }
            const auto found = context.artifacts.find(kind);
            return found == context.artifacts.end() ? std::string{} : found->second;
        }
        static_cast<void>(prompt);
        known = false;
        return {};
    };

    std::string body;
    body.reserve(prompt.body.size());

    std::size_t at = 0;
    while (at < prompt.body.size()) {
        const auto opening = prompt.body.find("{{", at);
        if (opening == std::string::npos) {
            body.append(prompt.body, at, std::string::npos);
            break;
        }
        const auto closing = prompt.body.find("}}", opening + 2);
        if (closing == std::string::npos) {

            body.append(prompt.body, at, std::string::npos);
            break;
        }

        body.append(prompt.body, at, opening - at);

        auto name = prompt.body.substr(opening + 2, closing - opening - 2);

        while (!name.empty() && (name.front() == ' ' || name.front() == '\t')) {
            name.erase(name.begin());
        }
        while (!name.empty() && (name.back() == ' ' || name.back() == '\t')) {
            name.pop_back();
        }

        bool known = false;
        auto value = valueOf(name, known);
        if (!known) {

            if (std::find(out.unknown.begin(), out.unknown.end(), name) == out.unknown.end()) {
                out.unknown.push_back(name);
            }
            body += "{{" + name + "}}";
        } else {
            if (value.empty() &&
                std::find(out.empty.begin(), out.empty.end(), name) == out.empty.end()) {
                out.empty.push_back(name);
            }
            body += value;
        }

        at = closing + 2;
    }

    if (!out.unknown.empty()) {
        return out;
    }

    std::string system = prompt.system;
    const auto append = [&system](const std::string &sentence) {
        if (sentence.empty()) {
            return;
        }
        if (!system.empty()) {
            system += "\n\n";
        }
        system += sentence;
    };

    append(languageInstruction(prompt, context, interfaceLanguage));
    append(instructionText(context, instructionCode(prompt.shape), shapeInstruction(prompt.shape)));

    for (const auto &code : prompt.extraInstructions) {
        const auto found =
            std::find_if(context.instructions.begin(), context.instructions.end(),
                         [&code](const TaxonomyEntry &one) { return one.code == code; });
        if (found == context.instructions.end()) {
            continue;
        }
        auto text = found->description;
        const auto spoken = context.language;
        for (auto where = text.find("{language}"); where != std::string::npos;
             where = text.find("{language}", where)) {
            text.replace(where, std::strlen("{language}"), spoken);
            where += spoken.size();
        }
        append(text);
    }

    if (!system.empty()) {
        out.messages.push_back({LlmRole::System, std::move(system)});
    }
    out.messages.push_back({LlmRole::User, std::move(body)});
    return out;
}

std::vector<PromptTemplate> shippedTemplates(std::string_view language)
{
    const auto &rows = content::listOf(language, "prompts");

    std::vector<PromptTemplate> all;
    all.reserve(rows.size());

    for (const auto &row : rows) {
        if (!row.is_object()) {
            continue;
        }
        const auto kind = parseArtifactKind(row.value("kind", std::string{}));
        const auto shape = parseAnswerShape(row.value("shape", std::string{}));
        auto code = row.value("code", std::string{});
        if (code.empty() || !kind.has_value() || !shape.has_value()) {
            continue;
        }

        PromptTemplate prompt;
        prompt.code = std::move(code);
        prompt.name = row.value("name", std::string{});
        prompt.kind = *kind;
        prompt.shape = *shape;
        prompt.system = row.value("system", std::string{});
        prompt.body = row.value("body", std::string{});

        if (const auto policy = parseLanguagePolicy(row.value("languagePolicy", std::string{}));
            policy.has_value()) {
            prompt.languagePolicy = *policy;
        }
        prompt.language = row.value("language", std::string{});
        prompt.providerId = row.value("model", std::string{});
        prompt.enabled = row.value("enabled", true);
        if (const auto extras = row.find("instructions");
            extras != row.end() && extras->is_array()) {
            for (const auto &one : *extras) {
                if (one.is_string()) {
                    prompt.extraInstructions.push_back(one.get<std::string>());
                }
            }
        }

        if (const auto mode = parseRealtimeMode(row.value("realtimeMode", std::string{}));
            mode.has_value()) {
            prompt.realtimeMode = *mode;
        }
        prompt.realtimeNear = row.value("realtimeNear", false);
        prompt.asrProviderId = row.value("recogniser", std::string{});
        if (const auto policy = parseTurnPolicy(row.value("turnPolicy", std::string{}));
            policy.has_value()) {
            prompt.turnPolicy = *policy;
        }

        prompt.builtIn = true;

        prompt.position = static_cast<int>(all.size());
        all.push_back(std::move(prompt));
    }
    return all;
}

std::optional<PromptTemplate> shippedTemplate(std::string_view code, std::string_view language)
{
    for (auto &prompt : shippedTemplates(language)) {
        if (prompt.code == code) {
            return prompt;
        }
    }
    return std::nullopt;
}

}
