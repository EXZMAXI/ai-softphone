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
#include <softphone/core/artifacts.hpp>
#include <softphone/core/automations.hpp>
#include <softphone/core/templates.hpp>
#include <softphone/core/usage.hpp>

#include <string_view>

namespace softphone::plugins::processing {

class ProcessingPlugin final : public container::IPlugin {
public:
    bool initialize(container::PluginContext &context, std::string &problem) override;

private:
    core::ITemplateStore *templates_{nullptr};
    core::IAutomationStore *automations_{nullptr};
    core::IArtifactStore *artifacts_{nullptr};
    core::IUsageStore *usage_{nullptr};
};

[[nodiscard]] container::Manifest processingManifest();

inline constexpr std::string_view kOwner = "processing";

inline constexpr std::string_view kTemplateStore = "processing.templates";
inline constexpr std::string_view kAutomationStore = "processing.automations";
inline constexpr std::string_view kArtifactStore = "processing.artifacts";
inline constexpr std::string_view kUsageStore = "processing.usage";

[[nodiscard]] inline std::string_view collectionsDdl()
{
    return R"(
        CREATE TABLE IF NOT EXISTS prompt_templates (
            id              TEXT    PRIMARY KEY,
            code            TEXT    NOT NULL DEFAULT '',
            name            TEXT    NOT NULL DEFAULT '',
            kind            TEXT    NOT NULL DEFAULT 'custom',
            shape           TEXT    NOT NULL DEFAULT 'plain',
            system          TEXT    NOT NULL DEFAULT '',
            body            TEXT    NOT NULL DEFAULT '',
            language_policy TEXT    NOT NULL DEFAULT 'as_spoken',
            language        TEXT    NOT NULL DEFAULT '',
            params          TEXT    NOT NULL DEFAULT '',
            built_in        INTEGER NOT NULL DEFAULT 0,
            enabled         INTEGER NOT NULL DEFAULT 1,
            version         INTEGER NOT NULL DEFAULT 1,
            created_at      INTEGER NOT NULL DEFAULT 0,
            updated_at      INTEGER NOT NULL DEFAULT 0,

            provider_id     TEXT    NOT NULL DEFAULT '',
            instructions    TEXT    NOT NULL DEFAULT '',

            realtime_mode    TEXT    NOT NULL DEFAULT 'subtitles',
            realtime_near    INTEGER NOT NULL DEFAULT 0,
            asr_provider_id  TEXT    NOT NULL DEFAULT '',
            turn_policy      TEXT    NOT NULL DEFAULT 'provider',

            position         INTEGER NOT NULL DEFAULT 0
        );
        CREATE TABLE IF NOT EXISTS artifacts (
            id               TEXT    PRIMARY KEY,
            recording_id     TEXT    NOT NULL REFERENCES recordings(id) ON DELETE CASCADE,
            transcript_id    TEXT    NOT NULL DEFAULT '',
            template_id      TEXT    NOT NULL DEFAULT '',
            template_code    TEXT    NOT NULL DEFAULT '',
            template_name    TEXT    NOT NULL DEFAULT '',
            template_version INTEGER NOT NULL DEFAULT 1,
            provider_id      TEXT    NOT NULL DEFAULT '',
            provider_type    TEXT    NOT NULL DEFAULT '',
            model            TEXT    NOT NULL DEFAULT '',
            kind             TEXT    NOT NULL DEFAULT 'custom',
            shape            TEXT    NOT NULL DEFAULT 'plain',
            status           TEXT    NOT NULL DEFAULT 'queued',
            content          TEXT    NOT NULL DEFAULT '',
            structured       TEXT    NOT NULL DEFAULT '',
            raw              TEXT    NOT NULL DEFAULT '',
            language         TEXT    NOT NULL DEFAULT '',
            parts            INTEGER NOT NULL DEFAULT 1,
            requested_at     INTEGER NOT NULL DEFAULT 0,
            completed_at     INTEGER NOT NULL DEFAULT 0,
            error            TEXT    NOT NULL DEFAULT '',
            prompt_tokens    INTEGER NOT NULL DEFAULT 0,
            answer_tokens    INTEGER NOT NULL DEFAULT 0,
            cost_micros      INTEGER NOT NULL DEFAULT 0,
            currency         TEXT    NOT NULL DEFAULT '',
            supersedes       TEXT    NOT NULL DEFAULT '',
            superseded       INTEGER NOT NULL DEFAULT 0,
            question         TEXT    NOT NULL DEFAULT '',

            turn_from        INTEGER NOT NULL DEFAULT -1,
            turn_to          INTEGER NOT NULL DEFAULT -1
        );

        /* **What an artifact says, indexed** — collections version 2. Every
           summary, every list of actions, every answer to a question is a
           `content` somebody will look for words in, and until this existed the
           library's search reached the *one-line* summary on the recording and
           nothing else: a person who remembered a sentence from the long
           summary could not find the conversation it was in. Mirrors
           `transcript_search` exactly — external content, the same tokeniser,
           the same three triggers — because the two answer the same question
           about the same conversation and a second shape would be a second set
           of surprises.

           `rebuild` rather than a conditional backfill: this whole declaration
           is re-executed whenever the version rises, so the step has to be
           idempotent, and rebuilding from the content table is the one form
           that is right whether the index is absent, empty or already full. */
        CREATE VIRTUAL TABLE IF NOT EXISTS artifact_search USING fts5(
            content,
            content='artifacts',
            content_rowid='rowid',
            tokenize='unicode61 remove_diacritics 2'
        );

        CREATE TRIGGER IF NOT EXISTS artifacts_indexed AFTER INSERT ON artifacts BEGIN
            INSERT INTO artifact_search(rowid, content) VALUES (new.rowid, new.content);
        END;

        CREATE TRIGGER IF NOT EXISTS artifacts_unindexed AFTER DELETE ON artifacts BEGIN
            INSERT INTO artifact_search(artifact_search, rowid, content)
            VALUES ('delete', old.rowid, old.content);
        END;

        CREATE TRIGGER IF NOT EXISTS artifacts_reindexed AFTER UPDATE ON artifacts BEGIN
            INSERT INTO artifact_search(artifact_search, rowid, content)
            VALUES ('delete', old.rowid, old.content);
            INSERT INTO artifact_search(rowid, content) VALUES (new.rowid, new.content);
        END;

        INSERT INTO artifact_search(artifact_search) VALUES ('rebuild');

        CREATE TABLE IF NOT EXISTS code_suggestions (
            id           TEXT    PRIMARY KEY,
            kind         TEXT    NOT NULL DEFAULT 'category',
            code         TEXT    NOT NULL DEFAULT '',
            recording_id TEXT    NOT NULL DEFAULT '',
            artifact_id  TEXT    NOT NULL DEFAULT '',
            seen         INTEGER NOT NULL DEFAULT 1,
            last_at      INTEGER NOT NULL DEFAULT 0
        );
        CREATE TABLE IF NOT EXISTS automations (
            id            TEXT    PRIMARY KEY,
            code          TEXT    NOT NULL DEFAULT '',
            name          TEXT    NOT NULL DEFAULT '',
            enabled       INTEGER NOT NULL DEFAULT 1,
            ordinal       INTEGER NOT NULL DEFAULT 0,
            conditions    TEXT    NOT NULL DEFAULT '',
            action        TEXT    NOT NULL DEFAULT 'generate',
            template_code TEXT    NOT NULL DEFAULT '',
            tag_code      TEXT    NOT NULL DEFAULT '',
            built_in      INTEGER NOT NULL DEFAULT 0,
            created_at    INTEGER NOT NULL DEFAULT 0,
            updated_at    INTEGER NOT NULL DEFAULT 0
        );
        CREATE TABLE IF NOT EXISTS automation_runs (
            automation_id TEXT    NOT NULL,
            recording_id  TEXT    NOT NULL REFERENCES recordings(id) ON DELETE CASCADE,
            fired_at      INTEGER NOT NULL DEFAULT 0,
            job_id        TEXT    NOT NULL DEFAULT '',
            PRIMARY KEY (automation_id, recording_id)
        );
        CREATE TABLE IF NOT EXISTS usage_ledger (
            id            TEXT    PRIMARY KEY,
            at            INTEGER NOT NULL DEFAULT 0,
            provider_id   TEXT    NOT NULL DEFAULT '',
            provider_type TEXT    NOT NULL DEFAULT '',
            model         TEXT    NOT NULL DEFAULT '',
            kind          TEXT    NOT NULL DEFAULT 'generate',
            job_id        TEXT    NOT NULL DEFAULT '',
            recording_id  TEXT    NOT NULL DEFAULT '',
            prompt_tokens INTEGER NOT NULL DEFAULT 0,
            answer_tokens INTEGER NOT NULL DEFAULT 0,
            cost_micros   INTEGER NOT NULL DEFAULT 0,
            currency      TEXT    NOT NULL DEFAULT ''
        );
        CREATE UNIQUE INDEX IF NOT EXISTS automations_by_code ON automations(code) WHERE code <> '';
        CREATE INDEX IF NOT EXISTS automation_runs_by_recording ON automation_runs(recording_id);
        CREATE UNIQUE INDEX IF NOT EXISTS templates_by_code ON prompt_templates(code) WHERE code <> '';
        CREATE INDEX IF NOT EXISTS artifacts_by_recording ON artifacts(recording_id, kind, requested_at DESC);
        CREATE UNIQUE INDEX IF NOT EXISTS suggestions_by_code ON code_suggestions(kind, code);
        CREATE INDEX IF NOT EXISTS usage_by_time ON usage_ledger(at);
        CREATE INDEX IF NOT EXISTS usage_by_recording ON usage_ledger(recording_id);
    )";
}

inline constexpr int kCollectionsVersion = 2;

}
