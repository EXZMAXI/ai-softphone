// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/softphone/plugin.hpp>

#include <softphone/core/version.hpp>

#include <softphone/plugins/storage/storage_access.hpp>

#include <string>

namespace softphone::plugins::softphone {

std::string_view collectionsDdl()
{
    return R"(
        CREATE TABLE IF NOT EXISTS calls (
            id               TEXT    PRIMARY KEY,
            started_at       INTEGER NOT NULL,
            duration_seconds INTEGER NOT NULL DEFAULT 0,
            direction        TEXT    NOT NULL,
            answered         INTEGER NOT NULL DEFAULT 0,
            end_reason       TEXT    NOT NULL DEFAULT '',
            number           TEXT    NOT NULL DEFAULT '',
            number_key       TEXT    NOT NULL DEFAULT '',
            remote_uri       TEXT    NOT NULL DEFAULT '',
            name             TEXT    NOT NULL DEFAULT '',
            account_id       TEXT    NOT NULL DEFAULT '',
            account_address  TEXT    NOT NULL DEFAULT '',
            dialed_number    TEXT    NOT NULL DEFAULT '',
            auto_answer      TEXT    NOT NULL DEFAULT 'no',
            attempts         INTEGER NOT NULL DEFAULT 1,
            recording_count  INTEGER NOT NULL DEFAULT 0,
            call_id          TEXT    NOT NULL DEFAULT '',
            seance_id        TEXT    NOT NULL DEFAULT ''
        );

        -- **The indexes belong with the table**, and leaving one behind has
        -- been paid for once: a `CREATE INDEX` left in the product's schema
        -- names a table that is no longer there, and a store that will not open
        -- is unavailable to everybody rather than to one plugin.
        --
        -- `calls_by_call` is on `call_id` — the name the *switch* gave the call
        -- — and not on `id`, which names the row. The two were joined on values
        -- that could not match once, and the index is what made the fix cheap.
        CREATE INDEX IF NOT EXISTS calls_by_time ON calls(started_at DESC);
        CREATE INDEX IF NOT EXISTS calls_by_number ON calls(number_key);
        CREATE INDEX IF NOT EXISTS calls_by_call ON calls(call_id);
        CREATE INDEX IF NOT EXISTS calls_by_seance ON calls(seance_id);
    )";
}

bool SoftphonePlugin::initialize(container::PluginContext &context, std::string &problem)
{
    auto *const database = context.find<storage::IStorageAccess>(storage::kStorageAccess);
    if (database == nullptr) {

        problem = "the storage service is not there";
        return false;
    }

    database->declareCollections(std::string{kOwner}, kCollectionsVersion,
                                 std::string{collectionsDdl()});

    log_ = database->history();
    if (log_ == nullptr) {
        problem = "the store offers no call log";
        return false;
    }

    if (!context.offer<core::ICallLog>(std::string{kCallLog}, container::Version{1, 0, 0}, log_)) {
        problem = "something else already offers " + std::string{kCallLog};
        return false;
    }

    if (engine_ != nullptr &&
        !context.offer<EngineHost>(std::string{kEngine}, container::Version{1, 0, 0}, engine_)) {
        problem = "something else already offers " + std::string{kEngine};
        return false;
    }

    if (power_) {
        power_(true);
    }

    return true;
}

void SoftphonePlugin::leaving() noexcept
{

    if (power_) {
        power_(false);
    }
}

container::Manifest softphoneManifest()
{
    container::Manifest manifest;
    manifest.id = std::string{kOwner};
    manifest.name = "Telephone";
    manifest.summary = "SIP, accounts, calls, the dialler and the call log";

    manifest.version =
        container::parseVersion(core::versionString()).value_or(container::Version{});
    manifest.requirements.push_back({"storage", {}});

    manifest.suggestions.push_back({"directory", {}});

    manifest.ui = true;
    return manifest;
}

}
