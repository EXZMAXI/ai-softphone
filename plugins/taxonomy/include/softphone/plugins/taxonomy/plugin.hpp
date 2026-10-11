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

#include <softphone/container/manifest.hpp>
#include <softphone/container/plugin.hpp>
#include <softphone/core/taxonomy.hpp>

#include <string_view>

namespace softphone::plugins::taxonomy {

class IDictionaryAnnouncer {
public:
    IDictionaryAnnouncer() = default;
    IDictionaryAnnouncer(const IDictionaryAnnouncer &) = delete;
    IDictionaryAnnouncer &operator=(const IDictionaryAnnouncer &) = delete;
    IDictionaryAnnouncer(IDictionaryAnnouncer &&) = delete;
    IDictionaryAnnouncer &operator=(IDictionaryAnnouncer &&) = delete;
    virtual ~IDictionaryAnnouncer() = default;

    virtual void announceChange() = 0;
};

class TaxonomyPlugin final : public container::IPlugin, public IDictionaryAnnouncer {
public:
    bool initialize(container::PluginContext &context, std::string &problem) override;

    void announceChange() override;

private:
    core::ITaxonomyStore *store_{nullptr};
    container::PluginContext *context_{nullptr};
};

[[nodiscard]] container::Manifest taxonomyManifest();

inline constexpr std::string_view kOwner = "taxonomy";

inline constexpr std::string_view kAnnouncer = "taxonomy.announcer";

inline constexpr std::string_view kTaxonomyStore = "taxonomy.store";

[[nodiscard]] inline std::string_view collectionsDdl()
{
    return R"(
        CREATE TABLE IF NOT EXISTS taxonomy_entries (
            id          TEXT    PRIMARY KEY,
            kind        TEXT    NOT NULL DEFAULT 'category',
            code        TEXT    NOT NULL DEFAULT '',
            title       TEXT    NOT NULL DEFAULT '',
            description TEXT    NOT NULL DEFAULT '',
            severity    TEXT    NOT NULL DEFAULT '',
            colour      TEXT    NOT NULL DEFAULT '',
            ordinal     INTEGER NOT NULL DEFAULT 0,
            built_in    INTEGER NOT NULL DEFAULT 0,
            retired_at  INTEGER NOT NULL DEFAULT 0,
            created_at  INTEGER NOT NULL DEFAULT 0,
            updated_at  INTEGER NOT NULL DEFAULT 0
        );

        -- **The index belongs with the table.** Leaving it behind in the
        -- product's schema has been paid for once: the table was gone and the
        -- `CREATE UNIQUE INDEX` on it stayed, so every open of a fresh database
        -- failed with *no such table* — and the plugin's own declaration then
        -- failed too, because a store that would not open is unavailable to
        -- everybody.
        CREATE UNIQUE INDEX IF NOT EXISTS taxonomy_by_code
            ON taxonomy_entries(kind, code);
    )";
}

inline constexpr int kCollectionsVersion = 1;

}
