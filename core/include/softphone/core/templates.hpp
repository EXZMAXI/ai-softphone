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
#include <softphone/core/store.hpp>
#include <softphone/core/taxonomy.hpp>

#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::core {

enum class ArtifactKind {
    Summary,
    SuperSummary,
    ActionItems,
    Topics,
    Entities,
    Categorise,

    Tags,
    RedFlags,
    Translation,

    Qa,

    Score,

    Custom,

    Realtime,
};

[[nodiscard]] std::string_view toString(ArtifactKind kind) noexcept;
[[nodiscard]] std::optional<ArtifactKind> parseArtifactKind(std::string_view text) noexcept;

enum class AnswerShape {

    Plain,

    List,

    Labels,

    Score,

    Rubric,

    Flags,

    Qa,

    Json,

    Moments,
};

[[nodiscard]] std::string_view toString(AnswerShape shape) noexcept;
[[nodiscard]] std::optional<AnswerShape> parseAnswerShape(std::string_view text) noexcept;

[[nodiscard]] constexpr bool isStructured(AnswerShape shape) noexcept
{
    return shape != AnswerShape::Plain;
}

enum class LanguagePolicy {

    AsSpoken,

    Interface,

    Fixed,
};

[[nodiscard]] std::string_view toString(LanguagePolicy policy) noexcept;
[[nodiscard]] std::optional<LanguagePolicy> parseLanguagePolicy(std::string_view text) noexcept;

enum class RealtimeMode {

    Subtitles,

    Prompt,

    Translate,

    Knowledge,
};

[[nodiscard]] std::string_view toString(RealtimeMode mode) noexcept;
[[nodiscard]] std::optional<RealtimeMode> parseRealtimeMode(std::string_view text) noexcept;

enum class TurnPolicy {

    Provider,

    Silence,

    Manual,
};

[[nodiscard]] std::string_view toString(TurnPolicy policy) noexcept;
[[nodiscard]] std::optional<TurnPolicy> parseTurnPolicy(std::string_view text) noexcept;

struct PromptTemplate {
    std::string id;

    std::string code;

    std::string name;
    ArtifactKind kind{ArtifactKind::Custom};
    AnswerShape shape{AnswerShape::Plain};

    std::string system;

    std::string body;

    LanguagePolicy languagePolicy{LanguagePolicy::Interface};

    std::string language;

    std::vector<std::pair<std::string, std::string>> params;

    std::string providerId;

    std::vector<std::string> extraInstructions;

    RealtimeMode realtimeMode{RealtimeMode::Subtitles};

    bool realtimeNear{false};

    std::string asrProviderId;

    TurnPolicy turnPolicy{TurnPolicy::Provider};

    bool builtIn{false};

    bool enabled{true};

    int position{0};

    int version{1};

    std::chrono::system_clock::time_point createdAt{};
    std::chrono::system_clock::time_point updatedAt{};
};

[[nodiscard]] std::vector<std::string> knownPlaceholders();

[[nodiscard]] bool asksAQuestion(const PromptTemplate &prompt);

[[nodiscard]] constexpr bool isRealtime(const PromptTemplate &prompt) noexcept
{
    return prompt.kind == ArtifactKind::Realtime;
}

[[nodiscard]] bool wantsAModel(const PromptTemplate &prompt) noexcept;

[[nodiscard]] RealtimeMode modeFor(const PromptTemplate &prompt) noexcept;

struct PromptContext {

    std::string transcript;

    std::string nearSide;
    std::string farSide;

    std::chrono::milliseconds duration{};
    std::string direction;
    std::string party;
    std::string account;

    std::string language;

    std::string reply;

    std::string conversation;

    std::vector<TaxonomyEntry> categories;
    std::vector<TaxonomyEntry> tags;
    std::vector<TaxonomyEntry> redFlags;

    std::vector<TaxonomyEntry> instructions;

    std::map<std::string, std::string> artifacts;

    std::string question;
};

struct RenderedPrompt {
    std::vector<LlmMessage> messages;

    std::vector<std::string> unknown;

    std::vector<std::string> empty;

    [[nodiscard]] bool usable() const noexcept
    {
        return unknown.empty() && !messages.empty();
    }
};

[[nodiscard]] RenderedPrompt renderPrompt(const PromptTemplate &prompt,
                                          const PromptContext &context,
                                          std::string_view interfaceLanguage);

[[nodiscard]] std::string_view languageName(std::string_view code) noexcept;

[[nodiscard]] std::string answerLanguage(const PromptTemplate &prompt, const PromptContext &context,
                                         std::string_view interfaceLanguage);

[[nodiscard]] std::string renderDictionary(const std::vector<TaxonomyEntry> &entries);

[[nodiscard]] std::string shapeInstruction(AnswerShape shape);

[[nodiscard]] std::string_view instructionCode(AnswerShape shape) noexcept;

[[nodiscard]] std::vector<PromptTemplate> shippedTemplates(std::string_view language = "en");

[[nodiscard]] std::optional<PromptTemplate> shippedTemplate(std::string_view code,
                                                            std::string_view language = "en");

class ITemplateStore {
public:
    ITemplateStore() = default;
    ITemplateStore(const ITemplateStore &) = delete;
    ITemplateStore &operator=(const ITemplateStore &) = delete;
    ITemplateStore(ITemplateStore &&) = delete;
    ITemplateStore &operator=(ITemplateStore &&) = delete;
    virtual ~ITemplateStore() = default;

    [[nodiscard]] virtual StoreStatus save(PromptTemplate &prompt) = 0;

    [[nodiscard]] virtual StoreStatus templates(std::vector<PromptTemplate> &out) = 0;

    [[nodiscard]] virtual StoreStatus
    orderTemplates(const std::vector<std::pair<std::string, int>> &places) = 0;

    [[nodiscard]] virtual StoreStatus prompt(const std::string &id, PromptTemplate &out) = 0;

    [[nodiscard]] virtual StoreStatus removeTemplate(const std::string &id) = 0;

    [[nodiscard]] virtual StoreStatus restoreShippedTemplates() = 0;
};

}
