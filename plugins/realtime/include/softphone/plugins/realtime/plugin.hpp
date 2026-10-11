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

#include <string_view>

namespace softphone::plugins::realtime {

class RealtimePlugin final : public container::IPlugin {
public:
    bool initialize(container::PluginContext &context, std::string &problem) override;
};

[[nodiscard]] container::Manifest realtimeManifest();

inline constexpr std::string_view kOwner = "realtime";

[[nodiscard]] inline std::string_view collectionsDdl()
{
    return R"(
        CREATE TABLE IF NOT EXISTS realtime_sessions (
            id                TEXT    PRIMARY KEY,
            seance_id         TEXT    NOT NULL DEFAULT '',
            recording_id      TEXT    NOT NULL DEFAULT '',
            transcript_id     TEXT    NOT NULL DEFAULT '',
            template_id       TEXT    NOT NULL DEFAULT '',
            template_code     TEXT    NOT NULL DEFAULT '',
            realtime_mode     TEXT    NOT NULL DEFAULT 'subtitles',
            asr_provider_id   TEXT    NOT NULL DEFAULT '',
            asr_provider_type TEXT    NOT NULL DEFAULT '',
            provider_id       TEXT    NOT NULL DEFAULT '',
            model             TEXT    NOT NULL DEFAULT '',
            language          TEXT    NOT NULL DEFAULT '',
            into_language     TEXT    NOT NULL DEFAULT '',
            turn_policy       TEXT    NOT NULL DEFAULT 'provider',
            backlog           TEXT    NOT NULL DEFAULT 'coalesce',
            started_at        INTEGER NOT NULL DEFAULT 0,
            ended_at          INTEGER NOT NULL DEFAULT 0,
            listened_ms       INTEGER NOT NULL DEFAULT 0,
            drops_ms          INTEGER NOT NULL DEFAULT 0,
            turns             INTEGER NOT NULL DEFAULT 0,
            asked             INTEGER NOT NULL DEFAULT 0,
            asr_cost_micros   INTEGER NOT NULL DEFAULT 0,
            model_cost_micros INTEGER NOT NULL DEFAULT 0,
            currency          TEXT    NOT NULL DEFAULT '',
            status            TEXT    NOT NULL DEFAULT 'running',
            error             TEXT    NOT NULL DEFAULT ''
        );

        CREATE INDEX IF NOT EXISTS realtime_sessions_by_seance
            ON realtime_sessions(seance_id, started_at DESC);
        CREATE INDEX IF NOT EXISTS realtime_sessions_by_month
            ON realtime_sessions(started_at);
    )";
}

inline constexpr int kCollectionsVersion = 1;

}
