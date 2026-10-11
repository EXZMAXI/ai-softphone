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

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

enum class TaxonomyKind {

    Category,

    Tag,

    RedFlag,

    Instruction,
};

[[nodiscard]] std::string_view toString(TaxonomyKind kind) noexcept;
[[nodiscard]] std::optional<TaxonomyKind> parseTaxonomyKind(std::string_view text) noexcept;

enum class Severity {
    Low,
    Medium,
    High,
};

[[nodiscard]] std::string_view toString(Severity severity) noexcept;
[[nodiscard]] std::optional<Severity> parseSeverity(std::string_view text) noexcept;

struct TaxonomyEntry {
    std::string id;
    TaxonomyKind kind{TaxonomyKind::Category};

    std::string code;

    std::string title;

    std::string description;

    std::optional<Severity> severity;

    std::string colour;

    int ordinal{0};

    bool builtIn{false};

    std::optional<std::chrono::system_clock::time_point> retiredAt;

    std::chrono::system_clock::time_point createdAt{};
    std::chrono::system_clock::time_point updatedAt{};

    [[nodiscard]] bool retired() const noexcept
    {
        return retiredAt.has_value();
    }
};

[[nodiscard]] bool validTaxonomyCode(std::string_view code) noexcept;

[[nodiscard]] std::string suggestTaxonomyCode(std::string_view title);

[[nodiscard]] std::string joinCodes(const std::vector<std::string> &codes);
[[nodiscard]] std::vector<std::string> splitCodes(std::string_view text);

[[nodiscard]] std::vector<TaxonomyEntry> starterTaxonomy(std::string_view language = "en");

enum class TagSource {

    Llm,

    Manual,
};

[[nodiscard]] std::string_view toString(TagSource source) noexcept;
[[nodiscard]] std::optional<TagSource> parseTagSource(std::string_view text) noexcept;

struct RecordingTag {
    std::string code;
    TagSource source{TagSource::Llm};

    std::string artifactId;
};

struct RedFlagHit {
    std::string id;
    std::string recordingId;
    std::string artifactId;
    std::string code;

    Severity severity{Severity::Medium};

    std::string evidence;

    std::optional<std::int64_t> atMs;

    bool dismissed{false};

    std::chrono::system_clock::time_point createdAt{};
};

class ITaxonomyStore {
public:
    ITaxonomyStore() = default;
    ITaxonomyStore(const ITaxonomyStore &) = delete;
    ITaxonomyStore &operator=(const ITaxonomyStore &) = delete;
    ITaxonomyStore(ITaxonomyStore &&) = delete;
    ITaxonomyStore &operator=(ITaxonomyStore &&) = delete;
    virtual ~ITaxonomyStore() = default;

    [[nodiscard]] virtual StoreStatus save(TaxonomyEntry &entry) = 0;

    [[nodiscard]] virtual StoreStatus entries(TaxonomyKind kind, bool includeRetired,
                                              std::vector<TaxonomyEntry> &out) = 0;

    [[nodiscard]] virtual StoreStatus entry(TaxonomyKind kind, const std::string &code,
                                            TaxonomyEntry &out) = 0;

    [[nodiscard]] virtual StoreStatus retireEntry(const std::string &id, bool retired) = 0;

    [[nodiscard]] virtual StoreStatus restoreShippedTaxonomy() = 0;

    [[nodiscard]] virtual StoreStatus removeEntry(const std::string &id, bool &retiredInstead) = 0;

    [[nodiscard]] virtual StoreStatus usedCodes(TaxonomyKind kind,
                                                std::vector<std::string> &out) = 0;

    [[nodiscard]] virtual StoreStatus tags(const std::string &recordingId,
                                           std::vector<RecordingTag> &out) = 0;

    [[nodiscard]] virtual StoreStatus setTag(const std::string &recordingId,
                                             const RecordingTag &tag) = 0;

    [[nodiscard]] virtual StoreStatus clearTag(const std::string &recordingId,
                                               const std::string &code) = 0;

    [[nodiscard]] virtual StoreStatus clearTagsFrom(const std::string &recordingId,
                                                    const std::string &templateCode) = 0;

    [[nodiscard]] virtual StoreStatus redFlags(const std::string &recordingId,
                                               std::vector<RedFlagHit> &out) = 0;

    [[nodiscard]] virtual StoreStatus saveRedFlag(RedFlagHit &hit) = 0;

    [[nodiscard]] virtual StoreStatus dismissRedFlag(const std::string &id, bool dismissed) = 0;

    [[nodiscard]] virtual StoreStatus clearRedFlagsFrom(const std::string &recordingId,
                                                        const std::string &templateCode) = 0;
};

class NoDictionaries final : public ITaxonomyStore {
public:
    [[nodiscard]] StoreStatus save(TaxonomyEntry &) override
    {
        return StoreStatus::Unavailable;
    }

    [[nodiscard]] StoreStatus entries(TaxonomyKind, bool, std::vector<TaxonomyEntry> &out) override
    {
        out.clear();
        return StoreStatus::Unavailable;
    }

    [[nodiscard]] StoreStatus entry(TaxonomyKind, const std::string &, TaxonomyEntry &) override
    {
        return StoreStatus::Unavailable;
    }

    [[nodiscard]] StoreStatus retireEntry(const std::string &, bool) override
    {
        return StoreStatus::Unavailable;
    }

    [[nodiscard]] StoreStatus restoreShippedTaxonomy() override
    {
        return StoreStatus::Unavailable;
    }

    [[nodiscard]] StoreStatus removeEntry(const std::string &, bool &retiredInstead) override
    {
        retiredInstead = false;
        return StoreStatus::Unavailable;
    }

    [[nodiscard]] StoreStatus usedCodes(TaxonomyKind, std::vector<std::string> &out) override
    {
        out.clear();
        return StoreStatus::Unavailable;
    }

    [[nodiscard]] StoreStatus tags(const std::string &, std::vector<RecordingTag> &out) override
    {
        out.clear();
        return StoreStatus::Unavailable;
    }

    [[nodiscard]] StoreStatus setTag(const std::string &, const RecordingTag &) override
    {
        return StoreStatus::Unavailable;
    }

    [[nodiscard]] StoreStatus clearTag(const std::string &, const std::string &) override
    {
        return StoreStatus::Unavailable;
    }

    [[nodiscard]] StoreStatus clearTagsFrom(const std::string &, const std::string &) override
    {
        return StoreStatus::Unavailable;
    }

    [[nodiscard]] StoreStatus redFlags(const std::string &, std::vector<RedFlagHit> &out) override
    {
        out.clear();
        return StoreStatus::Unavailable;
    }

    [[nodiscard]] StoreStatus saveRedFlag(RedFlagHit &) override
    {
        return StoreStatus::Unavailable;
    }

    [[nodiscard]] StoreStatus dismissRedFlag(const std::string &, bool) override
    {
        return StoreStatus::Unavailable;
    }

    [[nodiscard]] StoreStatus clearRedFlagsFrom(const std::string &, const std::string &) override
    {
        return StoreStatus::Unavailable;
    }
};

}
