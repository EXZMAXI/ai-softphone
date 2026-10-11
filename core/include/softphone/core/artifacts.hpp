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

#include <softphone/core/store.hpp>
#include <softphone/core/templates.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace softphone::core {

enum class ArtifactStatus {
    Queued,
    Running,
    Done,
    Failed,
};

[[nodiscard]] std::string_view toString(ArtifactStatus status) noexcept;
[[nodiscard]] std::optional<ArtifactStatus> parseArtifactStatus(std::string_view text) noexcept;

struct Artifact {
    std::string id;
    std::string recordingId;

    std::string transcriptId;

    std::string templateId;

    std::string templateCode;
    std::string templateName;
    int templateVersion{1};

    std::string providerId;
    std::string providerType;
    std::string model;

    ArtifactKind kind{ArtifactKind::Custom};
    AnswerShape shape{AnswerShape::Plain};
    ArtifactStatus status{ArtifactStatus::Queued};

    std::string content;

    std::string structured;

    std::string raw;

    std::string language;

    std::string question;

    int turnFrom{-1};
    int turnTo{-1};

    int parts{1};

    std::chrono::system_clock::time_point requestedAt{};
    std::chrono::system_clock::time_point completedAt{};

    std::string error;

    int promptTokens{0};
    int answerTokens{0};
    std::int64_t costMicros{0};
    std::string currency;

    std::string supersedes;

    bool superseded{false};
};

struct PromptedAnswer {
    std::string text;

    std::size_t from{0};
    std::size_t to{0};

    std::chrono::milliseconds at{0};
};

[[nodiscard]] std::optional<Artifact>
asRealtimeSuggestions(const std::string &recordingId, const std::string &transcriptId,
                      const std::vector<PromptedAnswer> &answers, const PromptTemplate &prompt);

struct ArtifactQuery {
    std::string recordingId;

    std::optional<ArtifactKind> kind;

    bool currentOnly{true};
};

struct CodeSuggestion {
    std::string id;
    TaxonomyKind kind{TaxonomyKind::Category};
    std::string code;
    std::string recordingId;
    std::string artifactId;
    int seen{1};
    std::chrono::system_clock::time_point lastAt{};
};

class IArtifactStore {
public:
    IArtifactStore() = default;
    IArtifactStore(const IArtifactStore &) = delete;
    IArtifactStore &operator=(const IArtifactStore &) = delete;
    IArtifactStore(IArtifactStore &&) = delete;
    IArtifactStore &operator=(IArtifactStore &&) = delete;
    virtual ~IArtifactStore() = default;

    [[nodiscard]] virtual StoreStatus save(Artifact &artifact) = 0;

    [[nodiscard]] virtual StoreStatus artifacts(const ArtifactQuery &query,
                                                std::vector<Artifact> &out) = 0;

    [[nodiscard]] virtual StoreStatus artifact(const std::string &id, Artifact &out) = 0;

    [[nodiscard]] virtual StoreStatus removeArtifact(const std::string &id) = 0;

    [[nodiscard]] virtual StoreStatus suggestCode(CodeSuggestion &suggestion) = 0;

    [[nodiscard]] virtual StoreStatus suggestions(TaxonomyKind kind,
                                                  std::vector<CodeSuggestion> &out) = 0;

    [[nodiscard]] virtual StoreStatus forgetSuggestion(const std::string &code,
                                                       TaxonomyKind kind) = 0;
};

}
