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
#include <softphone/core/jobs.hpp>
#include <softphone/core/providers.hpp>
#include <softphone/core/transcripts.hpp>

#include <string_view>

namespace softphone::plugins::transcription {

class TranscriptionPlugin final : public container::IPlugin {
public:
    bool initialize(container::PluginContext &context, std::string &problem) override;

private:
    core::ITranscriptStore *transcripts_{nullptr};
    core::IProviderStore *providers_{nullptr};
    core::IJobStore *jobs_{nullptr};
};

[[nodiscard]] container::Manifest transcriptionManifest();

inline constexpr std::string_view kOwner = "transcription";

inline constexpr std::string_view kTranscriptStore = "transcription.transcripts";
inline constexpr std::string_view kProviderStore = "transcription.providers";
inline constexpr std::string_view kJobStore = "transcription.jobs";

[[nodiscard]] inline std::string_view collectionsDdl()
{
    return R"(
        CREATE TABLE IF NOT EXISTS providers (
            id         TEXT    PRIMARY KEY,
            kind       TEXT    NOT NULL DEFAULT 'asr',
            type       TEXT    NOT NULL DEFAULT '',
            name       TEXT    NOT NULL DEFAULT '',
            endpoint   TEXT    NOT NULL DEFAULT '',
            model      TEXT    NOT NULL DEFAULT '',
            region     TEXT    NOT NULL DEFAULT '',
            params     TEXT    NOT NULL DEFAULT '',
            has_secret INTEGER NOT NULL DEFAULT 0,
            enabled    INTEGER NOT NULL DEFAULT 1,
            is_default INTEGER NOT NULL DEFAULT 0,
            created_at INTEGER NOT NULL DEFAULT 0,
            updated_at INTEGER NOT NULL DEFAULT 0,

            in_price_micros  INTEGER NOT NULL DEFAULT 0,
            out_price_micros INTEGER NOT NULL DEFAULT 0,
            currency         TEXT    NOT NULL DEFAULT '',

            read_timeout_seconds INTEGER NOT NULL DEFAULT 0,

            realtime_endpoint   TEXT    NOT NULL DEFAULT '',
            is_realtime_default INTEGER NOT NULL DEFAULT 0,

            realtime_model      TEXT    NOT NULL DEFAULT '',

            realtime_price_micros INTEGER NOT NULL DEFAULT 0,

            language TEXT NOT NULL DEFAULT '',

            models_seen TEXT NOT NULL DEFAULT ''
        );

        CREATE TABLE IF NOT EXISTS jobs (
            id              TEXT    PRIMARY KEY,
            kind            TEXT    NOT NULL DEFAULT 'transcribe',
            recording_id    TEXT    NOT NULL DEFAULT '',
            transcript_id   TEXT    NOT NULL DEFAULT '',
            provider_id     TEXT    NOT NULL DEFAULT '',
            template_id     TEXT    NOT NULL DEFAULT '',
            automation_id   TEXT    NOT NULL DEFAULT '',
            state           TEXT    NOT NULL DEFAULT 'queued',
            attempts        INTEGER NOT NULL DEFAULT 0,
            next_attempt_at INTEGER NOT NULL DEFAULT 0,
            error           TEXT    NOT NULL DEFAULT '',
            priority        INTEGER NOT NULL DEFAULT 0,
            params          TEXT    NOT NULL DEFAULT '',
            cost_micros     INTEGER NOT NULL DEFAULT 0,
            created_at      INTEGER NOT NULL DEFAULT 0,
            updated_at      INTEGER NOT NULL DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS transcripts (
            id                 TEXT    PRIMARY KEY,
            recording_id       TEXT    NOT NULL REFERENCES recordings(id) ON DELETE CASCADE,
            provider_id        TEXT    NOT NULL DEFAULT '',
            provider_type      TEXT    NOT NULL DEFAULT '',
            model              TEXT    NOT NULL DEFAULT '',
            requested_language TEXT    NOT NULL DEFAULT '',
            detected_language  TEXT    NOT NULL DEFAULT '',
            status             TEXT    NOT NULL DEFAULT 'queued',
            is_primary         INTEGER NOT NULL DEFAULT 0,
            requested_at       INTEGER NOT NULL DEFAULT 0,
            completed_at       INTEGER NOT NULL DEFAULT 0,
            error              TEXT    NOT NULL DEFAULT '',
            word_count         INTEGER NOT NULL DEFAULT 0,
            audio_ms           INTEGER NOT NULL DEFAULT 0,
            cost_micros        INTEGER NOT NULL DEFAULT 0,
            raw_path           TEXT    NOT NULL DEFAULT '',

            source             TEXT    NOT NULL DEFAULT 'batch'
        );

        CREATE TABLE IF NOT EXISTS transcript_segments (
            transcript_id TEXT    NOT NULL REFERENCES transcripts(id) ON DELETE CASCADE,
            ordinal       INTEGER NOT NULL,
            start_ms      INTEGER NOT NULL DEFAULT 0,
            end_ms        INTEGER NOT NULL DEFAULT 0,
            voice         TEXT    NOT NULL DEFAULT '',
            speaker       TEXT    NOT NULL DEFAULT '',
            text          TEXT    NOT NULL DEFAULT '',
            edited_text   TEXT    NOT NULL DEFAULT '',
            confidence    REAL    NOT NULL DEFAULT 0,
            words         TEXT    NOT NULL DEFAULT '',
            PRIMARY KEY (transcript_id, ordinal)
        );

        CREATE VIRTUAL TABLE IF NOT EXISTS transcript_search USING fts5(
            text,
            content='transcript_segments',
            content_rowid='rowid',
            tokenize='unicode61 remove_diacritics 2'
        );

        -- **The triggers are part of the table, not decoration on it.** The
        -- index is external content: nothing writes `transcript_search`
        -- directly, so a segment inserted without these three is a segment no
        -- search will ever find. They travel with the DDL that makes the table
        -- for exactly the reason the dictionaries' index does.
        CREATE TRIGGER IF NOT EXISTS transcript_segments_indexed
        AFTER INSERT ON transcript_segments BEGIN
            INSERT INTO transcript_search(rowid, text) VALUES (new.rowid, new.text);
        END;

        CREATE TRIGGER IF NOT EXISTS transcript_segments_unindexed
        AFTER DELETE ON transcript_segments BEGIN
            INSERT INTO transcript_search(transcript_search, rowid, text)
            VALUES ('delete', old.rowid, old.text);
        END;

        CREATE TRIGGER IF NOT EXISTS transcript_segments_reindexed
        AFTER UPDATE ON transcript_segments BEGIN
            INSERT INTO transcript_search(transcript_search, rowid, text)
            VALUES ('delete', old.rowid, old.text);
            INSERT INTO transcript_search(rowid, text) VALUES (new.rowid, new.text);
        END;

        CREATE INDEX IF NOT EXISTS providers_by_kind ON providers(kind, created_at);
        CREATE INDEX IF NOT EXISTS jobs_runnable
            ON jobs(state, next_attempt_at, priority DESC, created_at);
        CREATE INDEX IF NOT EXISTS jobs_by_recording ON jobs(recording_id);
        CREATE INDEX IF NOT EXISTS transcripts_by_recording
            ON transcripts(recording_id, requested_at DESC);
    )";
}

inline constexpr int kCollectionsVersion = 1;

}
