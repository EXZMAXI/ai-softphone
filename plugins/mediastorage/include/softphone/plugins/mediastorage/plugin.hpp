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
#include <softphone/core/recordings.hpp>

#include <string_view>

namespace softphone::plugins::mediastorage {

class MediaStoragePlugin final : public container::IPlugin {
public:
    bool initialize(container::PluginContext &context, std::string &problem) override;

private:
    core::IRecordingStore *library_{nullptr};
};

[[nodiscard]] container::Manifest mediaStorageManifest();

inline constexpr std::string_view kOwner = "mediastorage";

inline constexpr std::string_view kRecordingStore = "mediastorage.recordings";

[[nodiscard]] inline std::string_view collectionsDdl()
{
    return R"(
        CREATE TABLE IF NOT EXISTS recordings (
            id                 TEXT    PRIMARY KEY,
            source             TEXT    NOT NULL DEFAULT 'native',
            account_id         TEXT    NOT NULL DEFAULT '',
            direction          TEXT    NOT NULL DEFAULT 'out',
            started_at         INTEGER NOT NULL DEFAULT 0,
            duration_ms        INTEGER NOT NULL DEFAULT 0,
            party_count        INTEGER NOT NULL DEFAULT 0,
            remote_number      TEXT    NOT NULL DEFAULT '',
            number_key         TEXT    NOT NULL DEFAULT '',
            remote_uri         TEXT    NOT NULL DEFAULT '',
            remote_name        TEXT    NOT NULL DEFAULT '',
            file_path          TEXT    NOT NULL DEFAULT '',
            format             TEXT    NOT NULL DEFAULT 'mp3',
            sample_rate        INTEGER NOT NULL DEFAULT 0,
            channels           INTEGER NOT NULL DEFAULT 2,
            channel_layout     TEXT    NOT NULL DEFAULT 'near-far',
            bytes              INTEGER NOT NULL DEFAULT 0,
            has_gaps           INTEGER NOT NULL DEFAULT 0,
            consent            TEXT    NOT NULL DEFAULT 'announce',
            consent_at         INTEGER NOT NULL DEFAULT 0,
            pinned             INTEGER NOT NULL DEFAULT 0,
            category_code      TEXT    NOT NULL DEFAULT '',
            super_summary      TEXT    NOT NULL DEFAULT '',
            content_language   TEXT    NOT NULL DEFAULT '',
            red_flag_count     INTEGER NOT NULL DEFAULT 0,
            created_at         INTEGER NOT NULL DEFAULT 0,
            updated_at         INTEGER NOT NULL DEFAULT 0,
            deleted_at         INTEGER NOT NULL DEFAULT 0,

            transcript_count   INTEGER NOT NULL DEFAULT 0,
            score              INTEGER NOT NULL DEFAULT 0,
            title              TEXT    NOT NULL DEFAULT '',
            unfinished         INTEGER NOT NULL DEFAULT 0,
            seance_id          TEXT    NOT NULL DEFAULT ''
        );
        CREATE TABLE IF NOT EXISTS recording_calls (
            recording_id  TEXT    NOT NULL REFERENCES recordings(id) ON DELETE CASCADE,
            call_id       TEXT    NOT NULL,
            joined_ms     INTEGER NOT NULL DEFAULT 0,
            left_ms       INTEGER NOT NULL DEFAULT 0,
            remote_number TEXT    NOT NULL DEFAULT '',
            number_key    TEXT    NOT NULL DEFAULT '',
            remote_uri    TEXT    NOT NULL DEFAULT '',
            remote_name   TEXT    NOT NULL DEFAULT '',
            PRIMARY KEY (recording_id, call_id)
        );
        CREATE TABLE IF NOT EXISTS recording_stems (
            id            TEXT    PRIMARY KEY,
            recording_id  TEXT    NOT NULL REFERENCES recordings(id) ON DELETE CASCADE,
            call_id       TEXT    NOT NULL DEFAULT '',
            file_path     TEXT    NOT NULL DEFAULT '',
            offset_ms     INTEGER NOT NULL DEFAULT 0,
            duration_ms   INTEGER NOT NULL DEFAULT 0,
            bytes         INTEGER NOT NULL DEFAULT 0,
            remote_number TEXT    NOT NULL DEFAULT '',
            remote_name   TEXT    NOT NULL DEFAULT '',
            dropped_at    INTEGER NOT NULL DEFAULT 0
        );
        CREATE TABLE IF NOT EXISTS access_log (
            id           TEXT    PRIMARY KEY,
            recording_id TEXT    NOT NULL DEFAULT '',
            what         TEXT    NOT NULL DEFAULT '',
            detail       TEXT    NOT NULL DEFAULT '',
            at           INTEGER NOT NULL DEFAULT 0
        );
        CREATE TABLE IF NOT EXISTS recording_tags (
            recording_id TEXT    NOT NULL REFERENCES recordings(id) ON DELETE CASCADE,
            tag_code     TEXT    NOT NULL,
            source       TEXT    NOT NULL DEFAULT 'llm',
            artifact_id  TEXT    NOT NULL DEFAULT '',
            created_at   INTEGER NOT NULL DEFAULT 0,
            PRIMARY KEY (recording_id, tag_code)
        );
        CREATE TABLE IF NOT EXISTS red_flags (
            id           TEXT    PRIMARY KEY,
            recording_id TEXT    NOT NULL REFERENCES recordings(id) ON DELETE CASCADE,
            artifact_id  TEXT    NOT NULL DEFAULT '',
            flag_code    TEXT    NOT NULL DEFAULT '',
            severity     TEXT    NOT NULL DEFAULT 'medium',
            evidence     TEXT    NOT NULL DEFAULT '',
            at_ms        INTEGER NOT NULL DEFAULT -1,
            dismissed    INTEGER NOT NULL DEFAULT 0,
            created_at   INTEGER NOT NULL DEFAULT 0
        );
        CREATE INDEX IF NOT EXISTS recordings_by_time ON recordings(started_at DESC);
        CREATE INDEX IF NOT EXISTS recordings_by_number ON recordings(number_key);
        CREATE INDEX IF NOT EXISTS recording_calls_by_call ON recording_calls(call_id);
        CREATE INDEX IF NOT EXISTS recording_stems_by_recording ON recording_stems(recording_id);
        CREATE INDEX IF NOT EXISTS access_log_by_recording ON access_log(recording_id, at DESC);
        CREATE INDEX IF NOT EXISTS recording_tags_by_code ON recording_tags(tag_code);
        CREATE INDEX IF NOT EXISTS red_flags_by_recording ON red_flags(recording_id);
        CREATE INDEX IF NOT EXISTS red_flags_by_code ON red_flags(flag_code);
        CREATE INDEX IF NOT EXISTS recordings_by_seance ON recordings(seance_id);
    )";
}

inline constexpr int kCollectionsVersion = 1;

}
