// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/storage/sqlite_store.hpp>

#include <softphone/core/contacts.hpp>
#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>
#include <softphone/core/text_search.hpp>
#include <softphone/core/uuid.hpp>

#include <sqlite3.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::storage {
namespace {

using core::StoreStatus;

class Statement {
public:
    Statement(sqlite3 *database, std::string_view sql)
    {
        code_ = sqlite3_prepare_v2(database, sql.data(), static_cast<int>(sql.size()), &handle_,
                                   nullptr);
    }

    Statement(const Statement &) = delete;
    Statement &operator=(const Statement &) = delete;
    Statement(Statement &&) = delete;
    Statement &operator=(Statement &&) = delete;

    ~Statement()
    {
        sqlite3_finalize(handle_);
    }

    [[nodiscard]] int prepareCode() const noexcept
    {
        return code_;
    }

    [[nodiscard]] sqlite3_stmt *handle() const noexcept
    {
        return handle_;
    }

    void bindText(int index, const std::string &value) const
    {
        sqlite3_bind_text(handle_, index, value.c_str(), static_cast<int>(value.size()), nullptr);
    }

    void bindInt(int index, std::int64_t value) const
    {
        sqlite3_bind_int64(handle_, index, value);
    }

private:
    sqlite3_stmt *handle_{nullptr};
    int code_{SQLITE_OK};
};

[[nodiscard]] std::string textAt(sqlite3_stmt *statement, int column)
{
    const auto *text = sqlite3_column_text(statement, column);
    if (text == nullptr) {
        return {};
    }
    return std::string{reinterpret_cast<const char *>(text)};
}

[[nodiscard]] std::chrono::system_clock::time_point timeAt(sqlite3_stmt *statement, int column)
{
    return std::chrono::system_clock::time_point{
        std::chrono::seconds{sqlite3_column_int64(statement, column)}};
}

[[nodiscard]] std::int64_t secondsOf(std::chrono::system_clock::time_point instant)
{
    return std::chrono::duration_cast<std::chrono::seconds>(instant.time_since_epoch()).count();
}

[[nodiscard]] StoreStatus statusOf(int resultCode)
{
    switch (resultCode & 0xFF) {
    case SQLITE_OK:
    case SQLITE_ROW:
    case SQLITE_DONE:
        return StoreStatus::Ok;
    case SQLITE_BUSY:
    case SQLITE_LOCKED:
        return StoreStatus::Busy;
    case SQLITE_READONLY:
    case SQLITE_CANTOPEN:
    case SQLITE_PERM:
    case SQLITE_AUTH:
    case SQLITE_FULL:
    case SQLITE_IOERR:
        return StoreStatus::Unwritable;
    case SQLITE_NOTADB:
    case SQLITE_CORRUPT:
    case SQLITE_FORMAT:
        return StoreStatus::Malformed;
    default:
        return StoreStatus::Failed;
    }
}

[[nodiscard]] bool hasTable(sqlite3 *database, const char *name)
{
    const Statement asking{database, "SELECT 1 FROM sqlite_master WHERE type IN ('table','view') "
                                     "AND name = ? LIMIT 1;"};
    if (asking.prepareCode() != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(asking.handle(), 1, name, -1, SQLITE_STATIC);
    return sqlite3_step(asking.handle()) == SQLITE_ROW;
}

[[nodiscard]] bool hasColumn(sqlite3 *database, const char *table, const char *column)
{
    const std::string sql =
        std::string{"SELECT 1 FROM pragma_table_info('"} + table + "') WHERE name = ? LIMIT 1;";
    const Statement asking{database, sql};
    if (asking.prepareCode() != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(asking.handle(), 1, column, -1, SQLITE_STATIC);
    return sqlite3_step(asking.handle()) == SQLITE_ROW;
}

[[nodiscard]] std::string asMatchQuery(std::string_view words)
{
    std::string query;
    std::string term;
    bool quoted = false;

    const auto flush = [&query, &term](bool exact) {
        if (term.empty()) {
            return;
        }
        if (!query.empty()) {
            query += " AND ";
        }
        query += '"';
        for (const auto letter : term) {

            if (letter == '"') {
                query += '"';
            }
            query += letter;
        }
        query += '"';
        if (!exact) {
            query += '*';
        }
        term.clear();
    };

    for (const auto letter : words) {
        if (letter == '"') {
            quoted = !quoted;
            if (!quoted) {
                flush(true);
            }
            continue;
        }
        if (!quoted && std::isspace(static_cast<unsigned char>(letter)) != 0) {
            flush(false);
            continue;
        }
        term += letter;
    }

    flush(false);
    return query;
}

constexpr auto kSchema =

    R"sql(
    -- What this database has already been offered out of what the build ships,
    -- so that a prompt added in a later build reaches a database that already
    -- exists and one somebody deleted does not come back. See
    -- `installShippedPrompts` and `installShippedTaxonomy`, which the plugins
    -- that own those kinds ask for.
    CREATE TABLE seeded (
        kind      TEXT    NOT NULL,
        code      TEXT    NOT NULL,
        seeded_at INTEGER NOT NULL DEFAULT 0,
        PRIMARY KEY (kind, code)
    );

)sql";

struct Step {

    int to;

    const char *sql;

    bool (*when)(sqlite3 *database) = nullptr;
};

constexpr Step kSteps[] = {

    {4, "ALTER TABLE calls ADD COLUMN attempts INTEGER NOT NULL DEFAULT 1;"},

    {5, R"sql(
        CREATE TABLE recordings (
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
            subcategory_code   TEXT    NOT NULL DEFAULT '',
            super_summary      TEXT    NOT NULL DEFAULT '',
            content_language   TEXT    NOT NULL DEFAULT '',
            red_flag_count     INTEGER NOT NULL DEFAULT 0,
            created_at         INTEGER NOT NULL DEFAULT 0,
            updated_at         INTEGER NOT NULL DEFAULT 0,
            deleted_at         INTEGER NOT NULL DEFAULT 0
        );

        CREATE TABLE recording_calls (
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

        CREATE TABLE recording_stems (
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

        CREATE TABLE access_log (
            id           TEXT    PRIMARY KEY,
            recording_id TEXT    NOT NULL DEFAULT '',
            what         TEXT    NOT NULL DEFAULT '',
            detail       TEXT    NOT NULL DEFAULT '',
            at           INTEGER NOT NULL DEFAULT 0
        );

        CREATE INDEX recordings_by_time ON recordings(started_at DESC);
        CREATE INDEX recordings_by_number ON recordings(number_key);
        CREATE INDEX recording_calls_by_call ON recording_calls(call_id);
        CREATE INDEX recording_stems_by_recording ON recording_stems(recording_id);
        CREATE INDEX access_log_by_recording ON access_log(recording_id, at DESC);

        ALTER TABLE calls ADD COLUMN recording_count INTEGER NOT NULL DEFAULT 0;
    )sql"},

    {6, R"sql(
        ALTER TABLE calls ADD COLUMN call_id TEXT NOT NULL DEFAULT '';
        CREATE INDEX calls_by_call ON calls(call_id);
    )sql"},

    {7, R"sql(
        CREATE TABLE providers (
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
            updated_at INTEGER NOT NULL DEFAULT 0
        );

        CREATE TABLE jobs (
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

        CREATE INDEX providers_by_kind ON providers(kind, created_at);
        CREATE INDEX jobs_runnable ON jobs(state, next_attempt_at, priority DESC, created_at);
        CREATE INDEX jobs_by_recording ON jobs(recording_id);
    )sql"},

    {8, R"sql(
        CREATE TABLE transcripts (
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
            raw_path           TEXT    NOT NULL DEFAULT ''
        );

        CREATE TABLE transcript_segments (
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

        CREATE VIRTUAL TABLE transcript_search USING fts5(
            text,
            content='transcript_segments',
            content_rowid='rowid',
            tokenize='unicode61 remove_diacritics 2'
        );

        CREATE TRIGGER transcript_segments_indexed AFTER INSERT ON transcript_segments BEGIN
            INSERT INTO transcript_search(rowid, text) VALUES (new.rowid, new.text);
        END;

        CREATE TRIGGER transcript_segments_unindexed AFTER DELETE ON transcript_segments BEGIN
            INSERT INTO transcript_search(transcript_search, rowid, text)
            VALUES ('delete', old.rowid, old.text);
        END;

        CREATE TRIGGER transcript_segments_reindexed AFTER UPDATE ON transcript_segments BEGIN
            INSERT INTO transcript_search(transcript_search, rowid, text)
            VALUES ('delete', old.rowid, old.text);
            INSERT INTO transcript_search(rowid, text) VALUES (new.rowid, new.text);
        END;

        CREATE INDEX transcripts_by_recording ON transcripts(recording_id, requested_at DESC);

        ALTER TABLE recordings ADD COLUMN transcript_count INTEGER NOT NULL DEFAULT 0;
    )sql"},

    {9, R"sql(
        CREATE TABLE taxonomy_entries (
            id          TEXT    PRIMARY KEY,
            kind        TEXT    NOT NULL DEFAULT 'category',
            code        TEXT    NOT NULL DEFAULT '',
            parent_code TEXT    NOT NULL DEFAULT '',
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

        CREATE TABLE recording_tags (
            recording_id TEXT    NOT NULL REFERENCES recordings(id) ON DELETE CASCADE,
            tag_code     TEXT    NOT NULL,
            source       TEXT    NOT NULL DEFAULT 'llm',
            artifact_id  TEXT    NOT NULL DEFAULT '',
            created_at   INTEGER NOT NULL DEFAULT 0,
            PRIMARY KEY (recording_id, tag_code)
        );

        CREATE TABLE red_flags (
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

        CREATE UNIQUE INDEX taxonomy_by_code ON taxonomy_entries(kind, code);
        CREATE INDEX recording_tags_by_code ON recording_tags(tag_code);
        CREATE INDEX red_flags_by_recording ON red_flags(recording_id);
        CREATE INDEX red_flags_by_code ON red_flags(flag_code);
    )sql"},

    {10, R"sql(
        ALTER TABLE providers ADD COLUMN in_price_micros INTEGER NOT NULL DEFAULT 0;
        ALTER TABLE providers ADD COLUMN out_price_micros INTEGER NOT NULL DEFAULT 0;
        ALTER TABLE providers ADD COLUMN currency TEXT NOT NULL DEFAULT '';

        CREATE TABLE usage_ledger (
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

        CREATE INDEX usage_by_time ON usage_ledger(at);
        CREATE INDEX usage_by_recording ON usage_ledger(recording_id);
    )sql"},

    {11, R"sql(
        CREATE TABLE prompt_templates (
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
            updated_at      INTEGER NOT NULL DEFAULT 0
        );

        CREATE TABLE artifacts (
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
            superseded       INTEGER NOT NULL DEFAULT 0
        );

        CREATE TABLE code_suggestions (
            id           TEXT    PRIMARY KEY,
            kind         TEXT    NOT NULL DEFAULT 'category',
            code         TEXT    NOT NULL DEFAULT '',
            recording_id TEXT    NOT NULL DEFAULT '',
            artifact_id  TEXT    NOT NULL DEFAULT '',
            seen         INTEGER NOT NULL DEFAULT 1,
            last_at      INTEGER NOT NULL DEFAULT 0
        );

        -- A shipped template answers to a code and a hand-written one does not, so
        -- the uniqueness is over the ones that have one.
        CREATE UNIQUE INDEX templates_by_code ON prompt_templates(code) WHERE code <> '';
        CREATE INDEX artifacts_by_recording ON artifacts(recording_id, kind, requested_at DESC);
        CREATE UNIQUE INDEX suggestions_by_code ON code_suggestions(kind, code);
    )sql"},

    {12, R"sql(
        CREATE TABLE automations (
            id            TEXT    PRIMARY KEY,
            code          TEXT    NOT NULL DEFAULT '',
            name          TEXT    NOT NULL DEFAULT '',
            enabled       INTEGER NOT NULL DEFAULT 1,
            ordinal       INTEGER NOT NULL DEFAULT 0,
            fires_on      TEXT    NOT NULL DEFAULT 'transcript_finished',
            conditions    TEXT    NOT NULL DEFAULT '',
            action        TEXT    NOT NULL DEFAULT 'generate',
            provider_id   TEXT    NOT NULL DEFAULT '',
            template_code TEXT    NOT NULL DEFAULT '',
            tag_code      TEXT    NOT NULL DEFAULT '',
            built_in      INTEGER NOT NULL DEFAULT 0,
            created_at    INTEGER NOT NULL DEFAULT 0,
            updated_at    INTEGER NOT NULL DEFAULT 0
        );

        CREATE TABLE automation_runs (
            automation_id TEXT    NOT NULL,
            recording_id  TEXT    NOT NULL REFERENCES recordings(id) ON DELETE CASCADE,
            fired_at      INTEGER NOT NULL DEFAULT 0,
            job_id        TEXT    NOT NULL DEFAULT '',
            PRIMARY KEY (automation_id, recording_id)
        );

        CREATE UNIQUE INDEX automations_by_code ON automations(code) WHERE code <> '';
        CREATE INDEX automation_runs_by_recording ON automation_runs(recording_id);
    )sql"},

    {13, R"sql(
        CREATE TABLE seeded (
            kind      TEXT    NOT NULL,
            code      TEXT    NOT NULL,
            seeded_at INTEGER NOT NULL DEFAULT 0,
            PRIMARY KEY (kind, code)
        );

        -- The dictionary entry's own kind travels in the code, because a
        -- category and a tag may both be spelt `sales` and only the pair
        -- names one of them.
        INSERT OR IGNORE INTO seeded (kind, code)
            SELECT 'taxonomy', kind || '/' || code FROM taxonomy_entries
            WHERE built_in <> 0 AND code <> '';

        INSERT OR IGNORE INTO seeded (kind, code)
            SELECT 'template', code FROM prompt_templates
            WHERE built_in <> 0 AND code <> '';

        INSERT OR IGNORE INTO seeded (kind, code)
            SELECT 'automation', code FROM automations
            WHERE built_in <> 0 AND code <> '';
    )sql"},

    {14, "ALTER TABLE prompt_templates ADD COLUMN provider_id TEXT NOT NULL DEFAULT '';"},

    {15, "ALTER TABLE artifacts ADD COLUMN question TEXT NOT NULL DEFAULT '';"},

    {16, "ALTER TABLE recordings ADD COLUMN score INTEGER NOT NULL DEFAULT 0;"},

    {17, R"sql(
        ALTER TABLE automations DROP COLUMN fires_on;
        DELETE FROM automation_runs WHERE automation_id IN
            (SELECT id FROM automations WHERE built_in <> 0);
        DELETE FROM automations WHERE built_in <> 0;
        DELETE FROM seeded WHERE kind = 'automation';

        DELETE FROM prompt_templates WHERE built_in <> 0 AND code IN
            ('translation', 'sales-quality-score', 'sales-quality-notes',
             'support-quality-score', 'support-quality-notes');
        DELETE FROM seeded WHERE kind = 'template' AND code IN
            ('translation', 'sales-quality-score', 'sales-quality-notes',
             'support-quality-score', 'support-quality-notes');

        -- Four flags whose names were sentences. A dictionary entry's title is
        -- what the library's second column draws, and the column has room for
        -- two lines of everything a conversation was filed under. `built_in`
        -- still being set is what says nobody has edited this one: editing a
        -- title clears it.
        UPDATE taxonomy_entries SET title = 'Churn risk'
            WHERE built_in <> 0 AND code = 'churn-risk';
        UPDATE taxonomy_entries SET title = 'Promise made'
            WHERE built_in <> 0 AND code = 'promise-made';
        UPDATE taxonomy_entries SET title = 'Sensitive data'
            WHERE built_in <> 0 AND code = 'sensitive-data';
        UPDATE taxonomy_entries SET title = 'Competitor'
            WHERE built_in <> 0 AND code = 'competitor';
    )sql"},

    {18, R"sql(
        UPDATE prompt_templates SET language_policy = 'interface'
            WHERE built_in <> 0 AND language_policy = 'as_spoken';
    )sql"},

    {19, "ALTER TABLE prompt_templates ADD COLUMN instructions TEXT NOT NULL DEFAULT '';"},

    {20, "ALTER TABLE automations DROP COLUMN provider_id;"},

    {21, R"sql(
        ALTER TABLE recordings DROP COLUMN subcategory_code;
        ALTER TABLE taxonomy_entries DROP COLUMN parent_code;
    )sql"},

    {22, R"sql(
        ALTER TABLE recordings ADD COLUMN title TEXT NOT NULL DEFAULT '';
    )sql"},

    {23, R"sql(
        ALTER TABLE recordings ADD COLUMN unfinished INTEGER NOT NULL DEFAULT 0;
    )sql"},

    {24, R"sql(
        ALTER TABLE providers ADD COLUMN read_timeout_seconds INTEGER NOT NULL DEFAULT 0;
    )sql"},

    {25, R"sql(
        DELETE FROM red_flags WHERE rowid NOT IN (
            SELECT rowid FROM (
                SELECT rowid, ROW_NUMBER() OVER (
                           PARTITION BY recording_id, flag_code
                           ORDER BY dismissed DESC, created_at DESC, rowid DESC) AS standing
                  FROM red_flags)
             WHERE standing = 1);

        UPDATE recordings SET red_flag_count = (
            SELECT COUNT(*) FROM red_flags
             WHERE red_flags.recording_id = recordings.id AND red_flags.dismissed = 0);
    )sql"},

    {26, R"sql(
        ALTER TABLE recordings DROP COLUMN digest_artifact_id;
    )sql",
     [](sqlite3 *database) { return hasColumn(database, "recordings", "digest_artifact_id"); }},

    {27, R"sql(
        ALTER TABLE calls ADD COLUMN seance_id TEXT NOT NULL DEFAULT '';
        CREATE INDEX IF NOT EXISTS calls_by_seance ON calls(seance_id);
        ALTER TABLE recordings ADD COLUMN seance_id TEXT NOT NULL DEFAULT '';
        CREATE INDEX IF NOT EXISTS recordings_by_seance ON recordings(seance_id);
    )sql",
     [](sqlite3 *database) { return !hasColumn(database, "calls", "seance_id"); }},

    {28, R"sql(
        ALTER TABLE prompt_templates ADD COLUMN realtime_mode TEXT NOT NULL DEFAULT 'subtitles';
        ALTER TABLE prompt_templates ADD COLUMN realtime_near INTEGER NOT NULL DEFAULT 0;
        ALTER TABLE prompt_templates ADD COLUMN asr_provider_id TEXT NOT NULL DEFAULT '';
        ALTER TABLE prompt_templates ADD COLUMN turn_policy TEXT NOT NULL DEFAULT 'provider';
    )sql",
     [](sqlite3 *database) {
         return hasTable(database, "prompt_templates") &&
                !hasColumn(database, "prompt_templates", "realtime_mode");
     }},

    {29, R"sql(
        ALTER TABLE transcripts ADD COLUMN source TEXT NOT NULL DEFAULT 'batch';
    )sql",
     [](sqlite3 *database) {
         return hasTable(database, "transcripts") && !hasColumn(database, "transcripts", "source");
     }},

    {29, R"sql(
        ALTER TABLE artifacts ADD COLUMN turn_from INTEGER NOT NULL DEFAULT -1;
        ALTER TABLE artifacts ADD COLUMN turn_to INTEGER NOT NULL DEFAULT -1;
    )sql",
     [](sqlite3 *database) {
         return hasTable(database, "artifacts") && !hasColumn(database, "artifacts", "turn_from");
     }},

    {30, R"sql(
        ALTER TABLE providers ADD COLUMN realtime_endpoint TEXT NOT NULL DEFAULT '';
        ALTER TABLE providers ADD COLUMN is_realtime_default INTEGER NOT NULL DEFAULT 0;
    )sql",
     [](sqlite3 *database) {
         return hasTable(database, "providers") &&
                !hasColumn(database, "providers", "realtime_endpoint");
     }},

    {31, R"sql(
        ALTER TABLE providers ADD COLUMN realtime_model TEXT NOT NULL DEFAULT '';
    )sql",
     [](sqlite3 *database) {
         return hasTable(database, "providers") &&
                !hasColumn(database, "providers", "realtime_model");
     }},

    {32, R"sql(
        ALTER TABLE providers ADD COLUMN realtime_price_micros INTEGER NOT NULL DEFAULT 0;
    )sql",
     [](sqlite3 *database) {
         return hasTable(database, "providers") &&
                !hasColumn(database, "providers", "realtime_price_micros");
     }},

    {33, R"sql(
        ALTER TABLE providers ADD COLUMN language TEXT NOT NULL DEFAULT '';
    )sql",
     [](sqlite3 *database) {
         return hasTable(database, "providers") && !hasColumn(database, "providers", "language");
     }},

    {34, R"sql(
        ALTER TABLE prompt_templates ADD COLUMN position INTEGER NOT NULL DEFAULT 0;
    )sql",
     [](sqlite3 *database) {
         return hasTable(database, "prompt_templates") &&
                !hasColumn(database, "prompt_templates", "position");
     }},

    {35, R"sql(
        ALTER TABLE providers ADD COLUMN models_seen TEXT NOT NULL DEFAULT '';
    )sql",
     [](sqlite3 *database) {
         return hasTable(database, "providers") && !hasColumn(database, "providers", "models_seen");
     }},
};

}

std::filesystem::path defaultDatabaseFile()
{

    return databaseFileIn(core::dataDirectory());
}

std::filesystem::path databaseFileIn(const std::filesystem::path &dataRoot)
{
    return dataRoot / "ai-softphone.db";
}

SqliteStore::SqliteStore() = default;

SqliteStore::~SqliteStore()
{
    close();
}

StoreStatus SqliteStore::remember(int resultCode)
{
    const auto status = statusOf(resultCode);
    if (status != StoreStatus::Ok) {
        lastError_ = database_ != nullptr ? sqlite3_errmsg(database_) : sqlite3_errstr(resultCode);
    }
    return status;
}

StoreStatus SqliteStore::execute(const char *sql)
{
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }
    return remember(sqlite3_exec(database_, sql, nullptr, nullptr, nullptr));
}

namespace {

template<auto Answer>
void oneOfOurs(sqlite3_context *context, int count, sqlite3_value **values)
{
    if (count != 1 || sqlite3_value_type(values[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }

    const auto *text = reinterpret_cast<const char *>(sqlite3_value_text(values[0]));
    const auto length = static_cast<std::size_t>(sqlite3_value_bytes(values[0]));
    const auto answered =
        Answer(std::string_view{text == nullptr ? "" : text, text == nullptr ? 0 : length});

    sqlite3_result_text(context, answered.c_str(), static_cast<int>(answered.size()),
                        SQLITE_TRANSIENT);
}

}

StoreStatus SqliteStore::open(const std::filesystem::path &file)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    close();
    file_ = file;

    const bool inMemory = file == ":memory:";
    if (!inMemory && file.has_parent_path()) {
        std::error_code failure;
        std::filesystem::create_directories(file.parent_path(), failure);
        if (failure) {
            lastError_ = failure.message();
            return StoreStatus::Unwritable;
        }
    }

    const auto opened = sqlite3_open_v2(core::asUtf8(file).c_str(), &database_,
                                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (opened != SQLITE_OK) {
        const auto status = remember(opened);
        close();
        return status;
    }

    sqlite3_busy_timeout(database_, 5000);

    using SqlFunction = void (*)(sqlite3_context *, int, sqlite3_value **);
    const std::pair<const char *, SqlFunction> ours[] = {
        {"casefold", &oneOfOurs<&core::casefolded>},
        {"numberkey", &oneOfOurs<&core::numberKey>},
    };
    for (const auto &[name, function] : ours) {
        if (const auto made =
                sqlite3_create_function_v2(database_, name, 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                                           nullptr, function, nullptr, nullptr, nullptr);
            made != SQLITE_OK) {
            const auto status = remember(made);
            close();
            return status;
        }
    }

    if (const auto status = execute("PRAGMA foreign_keys = ON;"); status != StoreStatus::Ok) {
        close();
        return status;
    }

    if (!inMemory) {

        if (const auto status = execute("PRAGMA journal_mode = WAL;"); status != StoreStatus::Ok) {
            close();
            return status;
        }
    }

    if (const auto status = migrate(); status != StoreStatus::Ok) {
        close();
        return status;
    }

    return StoreStatus::Ok;
}

void SqliteStore::setContentLanguage(std::string language)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    contentLanguage_ = std::move(language);
}

std::string SqliteStore::lastError() const
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    return lastError_;
}

bool SqliteStore::isOpen() const
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    return database_ != nullptr;
}

void SqliteStore::close()
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ != nullptr) {
        sqlite3_close(database_);
        database_ = nullptr;
    }
}

int SqliteStore::schemaVersion() const
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return 0;
    }

    const Statement statement{database_, "PRAGMA user_version;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return 0;
    }
    if (sqlite3_step(statement.handle()) != SQLITE_ROW) {
        return 0;
    }
    return sqlite3_column_int(statement.handle(), 0);
}

StoreStatus SqliteStore::declareCollections(const std::string &owner, int version,
                                            const std::string &ddl)
{
    const std::lock_guard lock{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (const auto status = execute("CREATE TABLE IF NOT EXISTS schema_versions ("
                                    "    owner   TEXT PRIMARY KEY,"
                                    "    version INTEGER NOT NULL"
                                    ");");
        status != StoreStatus::Ok) {
        return status;
    }

    int at = 0;
    {
        const Statement current{database_, "SELECT version FROM schema_versions WHERE owner = ?;"};
        if (current.prepareCode() != SQLITE_OK) {
            return remember(current.prepareCode());
        }
        sqlite3_bind_text(current.handle(), 1, owner.c_str(), -1, SQLITE_TRANSIENT);
        if (const auto stepped = sqlite3_step(current.handle()); stepped == SQLITE_ROW) {
            at = sqlite3_column_int(current.handle(), 0);
        } else if (stepped != SQLITE_DONE) {
            return remember(stepped);
        }
    }

    if (at >= version) {
        return StoreStatus::Ok;
    }

    if (const auto status = execute("BEGIN IMMEDIATE;"); status != StoreStatus::Ok) {
        return status;
    }
    if (const auto status = execute(ddl.c_str()); status != StoreStatus::Ok) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    }

    {
        const Statement note{database_,
                             "INSERT INTO schema_versions (owner, version) VALUES (?, ?) "
                             "ON CONFLICT(owner) DO UPDATE SET version = excluded.version;"};
        if (note.prepareCode() != SQLITE_OK) {
            static_cast<void>(execute("ROLLBACK;"));
            return remember(note.prepareCode());
        }
        sqlite3_bind_text(note.handle(), 1, owner.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(note.handle(), 2, version);
        if (const auto stepped = sqlite3_step(note.handle()); stepped != SQLITE_DONE) {
            static_cast<void>(execute("ROLLBACK;"));
            return remember(stepped);
        }
    }

    return execute("COMMIT;");
}

StoreStatus SqliteStore::installShippedTaxonomy()
{
    const std::lock_guard lock{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (const auto status = execute("BEGIN IMMEDIATE;"); status != StoreStatus::Ok) {
        return status;
    }

    std::vector<std::string> written;
    if (const auto status = installTaxonomy(written); status != StoreStatus::Ok) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    }
    if (const auto status = rememberSeeded("taxonomy", written); status != StoreStatus::Ok) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    }

    if (!written.empty()) {
        core::Logger::instance().log(core::LogLevel::Info, "storage",
                                     "offered " + std::to_string(written.size()) +
                                         " shipped dictionary entries this database had not seen");
    }

    return execute("COMMIT;");
}

StoreStatus SqliteStore::migrate()
{
    int found = 0;
    {

        const Statement version{database_, "PRAGMA user_version;"};
        if (version.prepareCode() != SQLITE_OK) {
            return remember(version.prepareCode());
        }
        if (const auto stepped = sqlite3_step(version.handle()); stepped != SQLITE_ROW) {

            return remember(stepped);
        }
        found = sqlite3_column_int(version.handle(), 0);
    }

    if (found > kSchemaVersion) {

        lastError_ = "database schema is newer than this build understands";
        return StoreStatus::UnsupportedSchema;
    }
    if (found == kSchemaVersion) {
        return StoreStatus::Ok;
    }
    if (found != 0 && found < kOldestUpgradableSchema) {

        lastError_ = "database was made by a build older than the first release; "
                     "delete the file and it will be made again";
        return StoreStatus::UnsupportedSchema;
    }

    if (const auto status = execute("BEGIN IMMEDIATE;"); status != StoreStatus::Ok) {
        return status;
    }

    if (found == 0) {
        if (const auto status = execute(kSchema); status != StoreStatus::Ok) {
            static_cast<void>(execute("ROLLBACK;"));
            return status;
        }
    } else {
        core::Logger::instance().log(core::LogLevel::Info, "storage",
                                     "raising the database from schema " + std::to_string(found) +
                                         " to " + std::to_string(kSchemaVersion));

        for (const auto &step : kSteps) {
            if (step.to <= found) {
                continue;
            }
            if (step.when != nullptr && !step.when(database_)) {
                continue;
            }
            if (const auto status = execute(step.sql); status != StoreStatus::Ok) {

                core::Logger::instance().log(core::LogLevel::Error, "storage",
                                             "the step to schema " + std::to_string(step.to) +
                                                 " failed: " + lastError_);
                static_cast<void>(execute("ROLLBACK;"));
                return status;
            }
        }
    }

    const auto stamp = "PRAGMA user_version = " + std::to_string(kSchemaVersion) + ";";
    if (const auto status = execute(stamp.c_str()); status != StoreStatus::Ok) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    }

    return execute("COMMIT;");
}

StoreStatus SqliteStore::recreate()
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (file_.empty() || file_ == ":memory:") {

        return open(file_.empty() ? std::filesystem::path{":memory:"} : file_);
    }

    close();

    std::error_code failure;
    for (const auto suffix : {"", "-wal", "-shm"}) {
        auto companion = file_;
        companion += suffix;
        static_cast<void>(std::filesystem::remove(companion, failure));
    }

    core::Logger::instance().log(core::LogLevel::Warning, "storage",
                                 "the database was thrown away and made again: " +
                                     core::asUtf8(file_));

    return open(file_);
}

StoreStatus SqliteStore::contacts(std::vector<core::Contact> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, R"sql(
        SELECT id, name, number, account_id, account_address, created_at, updated_at
        FROM contacts
        ORDER BY name COLLATE NOCASE, id;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    std::vector<core::Contact> found;
    int stepped = SQLITE_OK;
    while ((stepped = sqlite3_step(statement.handle())) == SQLITE_ROW) {
        core::Contact contact;
        contact.id = textAt(statement.handle(), 0);
        contact.name = textAt(statement.handle(), 1);
        contact.number = textAt(statement.handle(), 2);
        contact.accountId = textAt(statement.handle(), 3);
        contact.accountAddress = textAt(statement.handle(), 4);
        contact.createdAt = timeAt(statement.handle(), 5);
        contact.updatedAt = timeAt(statement.handle(), 6);
        found.push_back(std::move(contact));
    }

    if (stepped != SQLITE_DONE) {
        return remember(stepped);
    }

    out = std::move(found);
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::save(core::Contact &contact)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto key = core::numberKey(contact.number);
    const auto now = std::chrono::system_clock::now();
    const auto stamped = std::chrono::time_point_cast<std::chrono::seconds>(now);

    if (contact.id.empty()) {
        contact.createdAt = stamped;
        contact.updatedAt = stamped;

        contact.id = core::makeUuid();

        const Statement statement{database_, R"sql(
            INSERT INTO contacts (id, name, number, number_key, account_id, account_address,
                                  created_at, updated_at)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?);
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }

        statement.bindText(1, contact.id);
        statement.bindText(2, contact.name);
        statement.bindText(3, contact.number);
        statement.bindText(4, key);
        statement.bindText(5, contact.accountId);
        statement.bindText(6, contact.accountAddress);
        statement.bindInt(7, secondsOf(contact.createdAt));
        statement.bindInt(8, secondsOf(contact.updatedAt));

        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return remember(stepped);
        }

        return StoreStatus::Ok;
    }

    contact.updatedAt = stamped;

    const Statement statement{database_, R"sql(
        UPDATE contacts
        SET name = ?, number = ?, number_key = ?, account_id = ?, account_address = ?,
            updated_at = ?
        WHERE id = ?;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    statement.bindText(1, contact.name);
    statement.bindText(2, contact.number);
    statement.bindText(3, key);
    statement.bindText(4, contact.accountId);
    statement.bindText(5, contact.accountAddress);
    statement.bindInt(6, secondsOf(contact.updatedAt));
    statement.bindText(7, contact.id);

    if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
        return remember(stepped);
    }

    return sqlite3_changes(database_) == 0 ? StoreStatus::Missing : StoreStatus::Ok;
}

StoreStatus SqliteStore::removeContact(const std::string &id)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, "DELETE FROM contacts WHERE id = ?;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, id);

    if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
        return remember(stepped);
    }
    return sqlite3_changes(database_) == 0 ? StoreStatus::Missing : StoreStatus::Ok;
}

StoreStatus SqliteStore::record(core::CallRecord &entry)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto key = core::numberKey(entry.number);
    const std::string direction{core::toString(entry.direction)};
    const std::string reason{core::toString(entry.endReason)};
    const std::string answeredBy{core::toString(entry.autoAnswer)};

    if (entry.id.empty()) {
        entry.id = core::makeUuid();
    }

    const Statement statement{database_, R"sql(
        INSERT INTO calls (id, started_at, duration_seconds, direction, answered, end_reason,
                           number, number_key, remote_uri, name, account_id, account_address,
                           dialed_number, auto_answer, attempts, call_id, seance_id)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    statement.bindText(1, entry.id);
    statement.bindInt(2, secondsOf(entry.startedAt));
    statement.bindInt(3, entry.duration.count());
    statement.bindText(4, direction);
    statement.bindInt(5, entry.answered ? 1 : 0);
    statement.bindText(6, reason);
    statement.bindText(7, entry.number);
    statement.bindText(8, key);
    statement.bindText(9, entry.remoteUri);
    statement.bindText(10, entry.name);
    statement.bindText(11, entry.accountId);
    statement.bindText(12, entry.accountAddress);
    statement.bindText(13, entry.dialedNumber);
    statement.bindText(14, answeredBy);
    statement.bindInt(15, entry.attempts);
    statement.bindText(16, entry.callId);
    statement.bindText(17, entry.seanceId);

    if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
        return remember(stepped);
    }

    return countRecordingsOn(entry.callId);
}

StoreStatus SqliteStore::calls(const core::CallLogQuery &query, std::vector<core::CallRecord> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    std::string sql = R"sql(
        SELECT id, started_at, duration_seconds, direction, answered, end_reason,
               number, remote_uri, name, account_id, account_address, dialed_number, auto_answer,
               attempts, recording_count, call_id, rowid, seance_id
        FROM calls
    )sql";

    std::string where;
    const auto narrow = [&where](std::string_view clause) {
        where += where.empty() ? " WHERE " : " AND ";
        where += clause;
    };

    if (query.direction.has_value()) {
        narrow("direction = ?");
    }

    std::string missed;
    if (query.missedOnly) {
        missed = "answered = 0 AND direction = '" +
                 std::string{core::toString(core::CallDirection::Incoming)} +
                 "' AND end_reason IN (";
        bool first = true;
        for (const auto reason : core::reasonsMeaningMissed()) {
            missed += first ? "'" : ", '";
            missed += core::toString(reason);
            missed += "'";
            first = false;
        }
        missed += ")";
        narrow(missed);
    }

    std::string declined;
    if (query.declinedOnly) {
        declined = "answered = 0 AND direction = '" +
                   std::string{core::toString(core::CallDirection::Incoming)} +
                   "' AND end_reason IN (";
        bool first = true;
        for (const auto reason : core::reasonsMeaningDeclined()) {
            declined += first ? "'" : ", '";
            declined += core::toString(reason);
            declined += "'";
            first = false;
        }
        declined += ")";
        narrow(declined);
    }
    if (!query.numberKey.empty()) {
        narrow("number_key = ?");
    }
    if (!query.search.empty()) {

        narrow("(casefold(name) LIKE ?"
               " OR casefold(number) LIKE ?"
               " OR number_key IN (SELECT number_key FROM contacts"
               "                   WHERE casefold(name) LIKE ?)"
               " OR numberkey(dialed_number) IN (SELECT number_key FROM contacts"
               "                                 WHERE casefold(name) LIKE ?))");
    }

    if (query.before.has_value()) {
        narrow("(started_at < ? OR (started_at = ? AND rowid < ?))");
    }

    sql += where;

    sql += " ORDER BY started_at DESC, rowid DESC LIMIT ? OFFSET ?;";

    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    int parameter = 1;
    std::string direction;
    if (query.direction.has_value()) {
        direction = core::toString(*query.direction);
        statement.bindText(parameter++, direction);
    }
    if (!query.numberKey.empty()) {
        statement.bindText(parameter++, query.numberKey);
    }
    const auto like = "%" + core::casefolded(query.search) + "%";
    if (!query.search.empty()) {

        for (int which = 0; which < 4; ++which) {
            statement.bindText(parameter++, like);
        }
    }
    if (query.before.has_value()) {
        const auto second = secondsOf(query.before->startedAt);
        statement.bindInt(parameter++, second);
        statement.bindInt(parameter++, second);
        statement.bindInt(parameter++, query.before->sequence);
    }
    statement.bindInt(parameter++, query.limit);
    statement.bindInt(parameter, query.offset);

    std::vector<core::CallRecord> found;
    int stepped = SQLITE_OK;
    while ((stepped = sqlite3_step(statement.handle())) == SQLITE_ROW) {
        core::CallRecord entry;
        entry.id = textAt(statement.handle(), 0);
        entry.startedAt = timeAt(statement.handle(), 1);
        entry.duration = std::chrono::seconds{sqlite3_column_int64(statement.handle(), 2)};
        entry.direction =
            core::parseCallDirection(textAt(statement.handle(), 3)).value_or(entry.direction);
        entry.answered = sqlite3_column_int(statement.handle(), 4) != 0;
        entry.endReason =
            core::parseCallEndReason(textAt(statement.handle(), 5)).value_or(entry.endReason);
        entry.number = textAt(statement.handle(), 6);
        entry.remoteUri = textAt(statement.handle(), 7);
        entry.name = textAt(statement.handle(), 8);
        entry.accountId = textAt(statement.handle(), 9);
        entry.accountAddress = textAt(statement.handle(), 10);
        entry.dialedNumber = textAt(statement.handle(), 11);
        entry.autoAnswer =
            core::parseAutoAnswer(textAt(statement.handle(), 12)).value_or(entry.autoAnswer);

        entry.attempts = std::max(1, sqlite3_column_int(statement.handle(), 13));
        entry.recordingCount = sqlite3_column_int(statement.handle(), 14);
        entry.callId = textAt(statement.handle(), 15);
        entry.seanceId = textAt(statement.handle(), 17);

        entry.sequence = sqlite3_column_int64(statement.handle(), 16);
        found.push_back(std::move(entry));
    }

    if (stepped != SQLITE_DONE) {
        return remember(stepped);
    }

    out = std::move(found);
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::removeCall(const std::string &id)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, "DELETE FROM calls WHERE id = ?;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, id);

    if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
        return remember(stepped);
    }
    return sqlite3_changes(database_) == 0 ? StoreStatus::Missing : StoreStatus::Ok;
}

StoreStatus SqliteStore::clearCalls()
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    return execute("DELETE FROM calls;");
}

StoreStatus SqliteStore::removeCallsBefore(std::chrono::system_clock::time_point before,
                                           int &removed)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    removed = 0;

    const Statement statement{database_, "DELETE FROM calls WHERE started_at < ?;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindInt(1, secondsOf(before));

    if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
        return remember(stepped);
    }

    removed = sqlite3_changes(database_);
    return StoreStatus::Ok;
}

namespace {

constexpr auto kRecordingColumns = R"sql(
    id, source, account_id, direction, started_at, duration_ms, party_count,
    remote_number, number_key, remote_uri, remote_name, file_path, format,
    sample_rate, channels, channel_layout, bytes, has_gaps, consent, consent_at,
    pinned, category_code, super_summary, content_language,
    red_flag_count, transcript_count, created_at, updated_at, deleted_at,
    score, title, unfinished, seance_id
)sql";

[[nodiscard]] core::Recording recordingFrom(sqlite3_stmt *row)
{
    core::Recording recording;
    recording.id = textAt(row, 0);
    recording.source =
        core::parseRecordingSource(textAt(row, 1)).value_or(core::RecordingSource::Native);
    recording.accountId = textAt(row, 2);
    recording.direction =
        core::parseCallDirection(textAt(row, 3)).value_or(core::CallDirection::Outgoing);
    recording.startedAt = timeAt(row, 4);
    recording.duration = std::chrono::milliseconds{sqlite3_column_int64(row, 5)};
    recording.partyCount = static_cast<unsigned>(sqlite3_column_int(row, 6));
    recording.remoteNumber = textAt(row, 7);

    recording.remoteUri = textAt(row, 9);
    recording.remoteName = textAt(row, 10);
    recording.filePath = textAt(row, 11);
    recording.format = textAt(row, 12);
    recording.sampleRate = static_cast<unsigned>(sqlite3_column_int(row, 13));
    recording.channels = static_cast<unsigned>(sqlite3_column_int(row, 14));
    recording.channelLayout = textAt(row, 15);
    recording.bytes = static_cast<std::uint64_t>(sqlite3_column_int64(row, 16));
    recording.hasGaps = sqlite3_column_int(row, 17) != 0;
    recording.consent =
        core::parseConsentPosture(textAt(row, 18)).value_or(core::ConsentPosture::Announce);
    recording.consentAt = timeAt(row, 19);
    recording.pinned = sqlite3_column_int(row, 20) != 0;
    recording.categoryCode = textAt(row, 21);
    recording.superSummary = textAt(row, 22);
    recording.contentLanguage = textAt(row, 23);
    recording.redFlagCount = static_cast<unsigned>(sqlite3_column_int(row, 24));
    recording.transcriptCount = static_cast<unsigned>(sqlite3_column_int(row, 25));
    recording.createdAt = timeAt(row, 26);
    recording.updatedAt = timeAt(row, 27);
    recording.deletedAt = timeAt(row, 28);
    recording.score = sqlite3_column_int(row, 29);
    recording.title = textAt(row, 30);
    recording.unfinished = sqlite3_column_int(row, 31) != 0;
    recording.seanceId = textAt(row, 32);
    return recording;
}

}

StoreStatus SqliteStore::writeLegs(const core::Recording &recording)
{

    {
        const Statement clear{database_, "DELETE FROM recording_calls WHERE recording_id = ?;"};
        if (clear.prepareCode() != SQLITE_OK) {
            return remember(clear.prepareCode());
        }
        clear.bindText(1, recording.id);
        if (const auto stepped = sqlite3_step(clear.handle()); stepped != SQLITE_DONE) {
            return remember(stepped);
        }
    }

    for (const core::RecordingLeg &leg : recording.legs) {
        const auto key = core::numberKey(leg.remoteNumber);
        const Statement statement{database_, R"sql(
            INSERT INTO recording_calls (recording_id, call_id, joined_ms, left_ms,
                                         remote_number, number_key, remote_uri, remote_name)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?);
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }

        statement.bindText(1, recording.id);
        statement.bindText(2, leg.callId);
        statement.bindInt(3, leg.joined.count());
        statement.bindInt(4, leg.left.count());
        statement.bindText(5, leg.remoteNumber);
        statement.bindText(6, key);
        statement.bindText(7, leg.remoteUri);
        statement.bindText(8, leg.remoteName);

        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return remember(stepped);
        }
    }

    return StoreStatus::Ok;
}

StoreStatus SqliteStore::writeStems(core::Recording &recording)
{
    {
        const Statement clear{database_, "DELETE FROM recording_stems WHERE recording_id = ?;"};
        if (clear.prepareCode() != SQLITE_OK) {
            return remember(clear.prepareCode());
        }
        clear.bindText(1, recording.id);
        if (const auto stepped = sqlite3_step(clear.handle()); stepped != SQLITE_DONE) {
            return remember(stepped);
        }
    }

    for (core::RecordingStem &stem : recording.stems) {
        if (stem.id.empty()) {
            stem.id = core::makeUuid();
        }

        const Statement statement{database_, R"sql(
            INSERT INTO recording_stems (id, recording_id, call_id, file_path, offset_ms,
                                         duration_ms, bytes, remote_number, remote_name,
                                         dropped_at)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }

        statement.bindText(1, stem.id);
        statement.bindText(2, recording.id);
        statement.bindText(3, stem.callId);
        statement.bindText(4, stem.filePath);
        statement.bindInt(5, stem.offset.count());
        statement.bindInt(6, stem.duration.count());
        statement.bindInt(7, static_cast<std::int64_t>(stem.bytes));
        statement.bindText(8, stem.remoteNumber);
        statement.bindText(9, stem.remoteName);
        statement.bindInt(10, secondsOf(stem.droppedAt));

        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return remember(stepped);
        }
    }

    return StoreStatus::Ok;
}

StoreStatus SqliteStore::countRecordingsOn(const std::string &callId)
{
    if (callId.empty()) {
        return StoreStatus::Ok;
    }

    const Statement statement{database_, R"sql(
        UPDATE calls SET recording_count =
            (SELECT COUNT(*) FROM recording_calls
                JOIN recordings ON recordings.id = recording_calls.recording_id
                WHERE recording_calls.call_id = ? AND recordings.deleted_at = 0)
        WHERE call_id = ?;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    statement.bindText(1, callId);
    statement.bindText(2, callId);

    if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
        return remember(stepped);
    }
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::save(core::Recording &recording)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (recording.id.empty()) {
        recording.id = core::makeUuid();
    }

    const auto now = std::chrono::system_clock::now();
    if (recording.createdAt.time_since_epoch().count() == 0) {
        recording.createdAt = now;
    }
    recording.updatedAt = now;

    const auto key = core::numberKey(recording.remoteNumber);
    const std::string source{core::toString(recording.source)};
    const std::string direction{core::toString(recording.direction)};
    const std::string consent{core::toString(recording.consent)};

    std::vector<std::string> touched;
    for (const core::RecordingLeg &leg : recording.legs) {
        touched.push_back(leg.callId);
    }
    {
        const Statement previous{database_,
                                 "SELECT call_id FROM recording_calls WHERE recording_id = ?;"};
        if (previous.prepareCode() != SQLITE_OK) {
            return remember(previous.prepareCode());
        }
        previous.bindText(1, recording.id);
        while (sqlite3_step(previous.handle()) == SQLITE_ROW) {
            touched.push_back(textAt(previous.handle(), 0));
        }
    }

    if (const auto status = execute("BEGIN IMMEDIATE;"); status != StoreStatus::Ok) {
        return status;
    }

    const auto undo = [this](StoreStatus status) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    };

    {

        const Statement statement{database_, R"sql(
            INSERT INTO recordings (
                id, source, account_id, direction, started_at, duration_ms, party_count,
                remote_number, number_key, remote_uri, remote_name, file_path, format,
                sample_rate, channels, channel_layout, bytes, has_gaps, consent, consent_at,
                pinned, category_code, super_summary, content_language,
                red_flag_count, created_at, updated_at, deleted_at, score,
                title, unfinished, seance_id)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,
                    ?, ?, ?, ?, ?, ?, ?, ?)
            ON CONFLICT(id) DO UPDATE SET
                source = excluded.source, account_id = excluded.account_id,
                direction = excluded.direction, started_at = excluded.started_at,
                duration_ms = excluded.duration_ms, party_count = excluded.party_count,
                remote_number = excluded.remote_number, number_key = excluded.number_key,
                remote_uri = excluded.remote_uri, remote_name = excluded.remote_name,
                file_path = excluded.file_path, format = excluded.format,
                sample_rate = excluded.sample_rate, channels = excluded.channels,
                channel_layout = excluded.channel_layout, bytes = excluded.bytes,
                has_gaps = excluded.has_gaps, consent = excluded.consent,
                consent_at = excluded.consent_at, pinned = excluded.pinned,
                category_code = excluded.category_code,
                super_summary = excluded.super_summary,
                content_language = excluded.content_language,
                red_flag_count = excluded.red_flag_count,
                created_at = excluded.created_at, updated_at = excluded.updated_at,
                deleted_at = excluded.deleted_at, score = excluded.score,
                title = excluded.title, unfinished = excluded.unfinished,
                seance_id = excluded.seance_id;
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return undo(remember(statement.prepareCode()));
        }

        statement.bindText(1, recording.id);
        statement.bindText(2, source);
        statement.bindText(3, recording.accountId);
        statement.bindText(4, direction);
        statement.bindInt(5, secondsOf(recording.startedAt));
        statement.bindInt(6, recording.duration.count());
        statement.bindInt(7, recording.partyCount);
        statement.bindText(8, recording.remoteNumber);
        statement.bindText(9, key);
        statement.bindText(10, recording.remoteUri);
        statement.bindText(11, recording.remoteName);
        statement.bindText(12, recording.filePath);
        statement.bindText(13, recording.format);
        statement.bindInt(14, recording.sampleRate);
        statement.bindInt(15, recording.channels);
        statement.bindText(16, recording.channelLayout);
        statement.bindInt(17, static_cast<std::int64_t>(recording.bytes));
        statement.bindInt(18, recording.hasGaps ? 1 : 0);
        statement.bindText(19, consent);
        statement.bindInt(20, secondsOf(recording.consentAt));
        statement.bindInt(21, recording.pinned ? 1 : 0);
        statement.bindText(22, recording.categoryCode);
        statement.bindText(23, recording.superSummary);
        statement.bindText(24, recording.contentLanguage);
        statement.bindInt(25, recording.redFlagCount);
        statement.bindInt(26, secondsOf(recording.createdAt));
        statement.bindInt(27, secondsOf(recording.updatedAt));
        statement.bindInt(28, secondsOf(recording.deletedAt));
        statement.bindInt(29, recording.score);
        statement.bindText(30, recording.title);
        statement.bindInt(31, recording.unfinished ? 1 : 0);
        statement.bindText(32, recording.seanceId);

        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return undo(remember(stepped));
        }
    }

    if (const auto status = writeLegs(recording); status != StoreStatus::Ok) {
        return undo(status);
    }
    if (const auto status = writeStems(recording); status != StoreStatus::Ok) {
        return undo(status);
    }

    for (const std::string &callId : touched) {
        if (const auto status = countRecordingsOn(callId); status != StoreStatus::Ok) {
            return undo(status);
        }
    }

    return execute("COMMIT;");
}

StoreStatus SqliteStore::readChildren(core::Recording &recording)
{
    {
        const Statement statement{database_, R"sql(
            SELECT call_id, joined_ms, left_ms, remote_number, remote_uri, remote_name
            FROM recording_calls WHERE recording_id = ? ORDER BY joined_ms;
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }
        statement.bindText(1, recording.id);

        while (sqlite3_step(statement.handle()) == SQLITE_ROW) {
            core::RecordingLeg leg;
            leg.callId = textAt(statement.handle(), 0);
            leg.joined = std::chrono::milliseconds{sqlite3_column_int64(statement.handle(), 1)};
            leg.left = std::chrono::milliseconds{sqlite3_column_int64(statement.handle(), 2)};
            leg.remoteNumber = textAt(statement.handle(), 3);
            leg.remoteUri = textAt(statement.handle(), 4);
            leg.remoteName = textAt(statement.handle(), 5);
            recording.legs.push_back(std::move(leg));
        }
    }

    const Statement statement{database_, R"sql(
        SELECT id, call_id, file_path, offset_ms, duration_ms, bytes, remote_number,
               remote_name, dropped_at
        FROM recording_stems WHERE recording_id = ? ORDER BY offset_ms;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, recording.id);

    while (sqlite3_step(statement.handle()) == SQLITE_ROW) {
        core::RecordingStem stem;
        stem.id = textAt(statement.handle(), 0);
        stem.callId = textAt(statement.handle(), 1);
        stem.filePath = textAt(statement.handle(), 2);
        stem.offset = std::chrono::milliseconds{sqlite3_column_int64(statement.handle(), 3)};
        stem.duration = std::chrono::milliseconds{sqlite3_column_int64(statement.handle(), 4)};
        stem.bytes = static_cast<std::uint64_t>(sqlite3_column_int64(statement.handle(), 5));
        stem.remoteNumber = textAt(statement.handle(), 6);
        stem.remoteName = textAt(statement.handle(), 7);
        stem.droppedAt = timeAt(statement.handle(), 8);
        recording.stems.push_back(std::move(stem));
    }

    return StoreStatus::Ok;
}

StoreStatus SqliteStore::recordings(const core::RecordingQuery &query,
                                    std::vector<core::Recording> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    out.clear();

    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    std::string sql = std::string{"SELECT "} + kRecordingColumns + " FROM recordings";

    std::string where;
    const auto narrow = [&where](std::string_view clause) {
        where += where.empty() ? " WHERE " : " AND ";
        where += clause;
    };

    if (!query.includeDeleted) {
        narrow("deleted_at = 0");
    }
    if (!query.numberKey.empty()) {
        narrow("number_key = ?");
    }
    if (query.source.has_value()) {
        narrow("source = ?");
    }
    if (!query.callId.empty()) {

        narrow("id IN (SELECT recording_id FROM recording_calls WHERE call_id = ?)");
    }
    if (query.startedFrom.has_value()) {
        narrow("started_at >= ?");
    }
    if (query.startedBefore.has_value()) {

        narrow("started_at < ?");
    }
    if (query.direction.has_value()) {
        narrow("direction = ?");
    }
    if (query.pinned.has_value()) {
        narrow(*query.pinned ? "pinned <> 0" : "pinned = 0");
    }
    if (query.conference.has_value()) {
        narrow(*query.conference ? "party_count > 1" : "party_count <= 1");
    }
    if (query.transcribed.has_value()) {
        narrow(*query.transcribed ? "transcript_count > 0" : "transcript_count = 0");
    }
    if (query.processed.has_value()) {

        narrow(*query.processed
                   ? "EXISTS (SELECT 1 FROM artifacts WHERE artifacts.recording_id = "
                     "recordings.id AND artifacts.status = 'done')"
                   : "NOT EXISTS (SELECT 1 FROM artifacts WHERE artifacts.recording_id = "
                     "recordings.id AND artifacts.status = 'done')");
    }
    if (!query.language.empty()) {

        narrow("id IN (SELECT recording_id FROM transcripts WHERE is_primary <> 0 "
               "AND (detected_language = ? OR (detected_language = '' "
               "AND requested_language = ?)))");
    }

    if (!query.category.empty()) {
        narrow("category_code = ?");
    }
    if (!query.tag.empty()) {
        narrow("id IN (SELECT recording_id FROM recording_tags WHERE tag_code = ?)");
    }
    if (!query.redFlag.empty()) {
        narrow("id IN (SELECT recording_id FROM red_flags "
               "WHERE flag_code = ? AND dismissed = 0)");
    }
    if (query.hasRedFlags.has_value()) {

        narrow(*query.hasRedFlags ? "red_flag_count > 0" : "red_flag_count = 0");
    }

    const auto match = asMatchQuery(query.text);

    int wordsBound = 0;
    if (!query.text.empty()) {

        std::string clause = "(casefold(remote_name) LIKE ? OR casefold(remote_number) LIKE ? "
                             "OR casefold(super_summary) LIKE ? "
                             "OR casefold(category_code) LIKE ? "
                             "OR id IN (SELECT recording_id FROM recording_calls "
                             "WHERE casefold(remote_name) LIKE ? "
                             "OR casefold(remote_number) LIKE ?)";

        if (!match.empty() && hasTable(database_, "transcript_search")) {
            clause += " OR id IN (SELECT t.recording_id FROM transcript_search "
                      "JOIN transcript_segments s ON s.rowid = transcript_search.rowid "
                      "JOIN transcripts t ON t.id = s.transcript_id "
                      "WHERE transcript_search MATCH ?)";
            wordsBound++;
        }

        if (!match.empty() && hasTable(database_, "artifact_search")) {
            clause += " OR id IN (SELECT a.recording_id FROM artifact_search "
                      "JOIN artifacts a ON a.rowid = artifact_search.rowid "
                      "WHERE artifact_search MATCH ?)";
            wordsBound++;
        }
        clause += ')';
        narrow(clause);
    }

    sql += where;
    sql += " ORDER BY started_at DESC";
    if (query.limit > 0) {
        sql += " LIMIT ? OFFSET ?";
    }
    sql += ';';

    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    int at = 1;
    std::string source;
    if (!query.numberKey.empty()) {
        statement.bindText(at++, query.numberKey);
    }
    if (query.source.has_value()) {
        source = core::toString(*query.source);
        statement.bindText(at++, source);
    }
    if (!query.callId.empty()) {
        statement.bindText(at++, query.callId);
    }
    if (query.startedFrom.has_value()) {
        statement.bindInt(at++, secondsOf(*query.startedFrom));
    }
    if (query.startedBefore.has_value()) {
        statement.bindInt(at++, secondsOf(*query.startedBefore));
    }
    std::string direction;
    if (query.direction.has_value()) {
        direction = core::toString(*query.direction);
        statement.bindText(at++, direction);
    }
    if (!query.language.empty()) {
        statement.bindText(at++, query.language);
        statement.bindText(at++, query.language);
    }
    if (!query.category.empty()) {
        statement.bindText(at++, query.category);
    }
    if (!query.tag.empty()) {
        statement.bindText(at++, query.tag);
    }
    if (!query.redFlag.empty()) {
        statement.bindText(at++, query.redFlag);
    }
    const auto like = "%" + core::casefolded(query.text) + "%";
    if (!query.text.empty()) {
        for (int which = 0; which < 6; ++which) {
            statement.bindText(at++, like);
        }
        for (int which = 0; which < wordsBound; ++which) {
            statement.bindText(at++, match);
        }
    }
    if (query.limit > 0) {
        statement.bindInt(at++, static_cast<std::int64_t>(query.limit));
        statement.bindInt(at++, static_cast<std::int64_t>(query.offset));
    }

    while (true) {
        const auto stepped = sqlite3_step(statement.handle());
        if (stepped == SQLITE_DONE) {
            break;
        }
        if (stepped != SQLITE_ROW) {
            return remember(stepped);
        }
        out.push_back(recordingFrom(statement.handle()));
    }

    for (core::Recording &recording : out) {
        if (const auto status = readChildren(recording); status != StoreStatus::Ok) {
            return status;
        }
    }

    return readTags(out);
}

core::StoreStatus SqliteStore::readTags(std::vector<core::Recording> &page)
{

    if (page.empty()) {
        return StoreStatus::Ok;
    }

    const auto placeholders = [&page] {
        std::string list;
        for (std::size_t at = 0; at < page.size(); ++at) {
            list += at == 0 ? "?" : ", ?";
        }
        return list;
    }();

    const auto gather = [this,
                         &page](const std::string &sql,
                                std::vector<std::string> core::Recording::*into) -> StoreStatus {
        const Statement statement{database_, sql};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }
        int at = 1;
        for (const auto &recording : page) {
            statement.bindText(at++, recording.id);
        }

        for (;;) {
            const auto code = sqlite3_step(statement.handle());
            if (code == SQLITE_DONE) {
                break;
            }
            if (code != SQLITE_ROW) {
                return remember(code);
            }
            const auto id = textAt(statement.handle(), 0);
            const auto found =
                std::find_if(page.begin(), page.end(),
                             [&id](const core::Recording &one) { return one.id == id; });
            if (found != page.end()) {
                ((*found).*into).push_back(textAt(statement.handle(), 1));
            }
        }
        return StoreStatus::Ok;
    };

    if (const auto status =
            gather("SELECT recording_id, tag_code FROM recording_tags WHERE recording_id IN (" +
                       placeholders + ") ORDER BY tag_code;",
                   &core::Recording::tags);
        status != StoreStatus::Ok) {
        return status;
    }

    return gather("SELECT DISTINCT recording_id, flag_code FROM red_flags "
                  "WHERE dismissed = 0 AND recording_id IN (" +
                      placeholders + ") ORDER BY flag_code;",
                  &core::Recording::redFlags);
}

StoreStatus SqliteStore::recording(const std::string &id, core::Recording &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    out = core::Recording{};

    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const std::string sql =
        std::string{"SELECT "} + kRecordingColumns + " FROM recordings WHERE id = ?;";

    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, id);

    const auto stepped = sqlite3_step(statement.handle());
    if (stepped == SQLITE_DONE) {
        return StoreStatus::Missing;
    }
    if (stepped != SQLITE_ROW) {
        return remember(stepped);
    }

    out = recordingFrom(statement.handle());
    if (const auto status = readChildren(out); status != StoreStatus::Ok) {
        return status;
    }

    std::vector<core::Recording> one;
    one.push_back(std::move(out));
    const auto status = readTags(one);
    out = std::move(one.front());
    return status;
}

StoreStatus SqliteStore::removeRecording(const std::string &id, std::vector<std::string> &files)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    files.clear();

    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    core::Recording going;
    if (const auto status = recording(id, going); status != StoreStatus::Ok) {
        return status;
    }
    if (going.isDeleted()) {

        return StoreStatus::Ok;
    }

    if (!going.filePath.empty()) {
        files.push_back(going.filePath);
    }
    for (const core::RecordingStem &stem : going.stems) {
        if (!stem.filePath.empty() && stem.droppedAt.time_since_epoch().count() == 0) {
            files.push_back(stem.filePath);
        }
    }

    std::vector<std::string> touched;
    for (const core::RecordingLeg &leg : going.legs) {
        touched.push_back(leg.callId);
    }

    {
        const Statement statement{database_,
                                  "SELECT raw_path FROM transcripts WHERE recording_id = ?;"};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }
        statement.bindText(1, id);
        for (;;) {
            const auto code = sqlite3_step(statement.handle());
            if (code == SQLITE_DONE) {
                break;
            }
            if (code != SQLITE_ROW) {
                return remember(code);
            }
            if (auto path = textAt(statement.handle(), 0); !path.empty()) {
                files.push_back(std::move(path));
            }
        }
    }

    if (const auto status = execute("BEGIN IMMEDIATE;"); status != StoreStatus::Ok) {
        return status;
    }

    const auto undo = [this](StoreStatus status) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    };

    {
        const Statement statement{database_, R"sql(
            UPDATE recordings SET
                account_id = '', remote_number = '', number_key = '', remote_uri = '',
                remote_name = '', title = '', file_path = '', super_summary = '',
                category_code = '',
                content_language = '',
                duration_ms = 0, party_count = 0, bytes = 0, sample_rate = 0,
                red_flag_count = 0, pinned = 0, has_gaps = 0, transcript_count = 0,
                score = 0, deleted_at = ?, updated_at = ?
            WHERE id = ?;
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return undo(remember(statement.prepareCode()));
        }

        const auto now = secondsOf(std::chrono::system_clock::now());
        statement.bindInt(1, now);
        statement.bindInt(2, now);
        statement.bindText(3, id);

        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return undo(remember(stepped));
        }
    }

    for (const char *sql : {"DELETE FROM transcript_segments WHERE transcript_id IN "
                            "(SELECT id FROM transcripts WHERE recording_id = ?);",
                            "DELETE FROM transcripts WHERE recording_id = ?;",
                            "DELETE FROM recording_stems WHERE recording_id = ?;",
                            "DELETE FROM recording_calls WHERE recording_id = ?;",
                            "DELETE FROM recording_tags WHERE recording_id = ?;",
                            "DELETE FROM red_flags WHERE recording_id = ?;",
                            "DELETE FROM usage_ledger WHERE recording_id = ?;",
                            "DELETE FROM code_suggestions WHERE recording_id = ?;",
                            "DELETE FROM artifacts WHERE recording_id = ?;",
                            "DELETE FROM automation_runs WHERE recording_id = ?;"}) {
        const Statement statement{database_, sql};
        if (statement.prepareCode() != SQLITE_OK) {
            return undo(remember(statement.prepareCode()));
        }
        statement.bindText(1, id);
        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return undo(remember(stepped));
        }
    }

    for (const std::string &callId : touched) {
        if (const auto status = countRecordingsOn(callId); status != StoreStatus::Ok) {
            return undo(status);
        }
    }

    if (const auto status = execute("COMMIT;"); status != StoreStatus::Ok) {
        return status;
    }

    return noteAccess(id, core::RecordingAccess::Deleted, going.filePath);
}

StoreStatus SqliteStore::dropAudio(const std::string &id, std::vector<std::string> &files)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    files.clear();

    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    core::Recording going;
    if (const auto status = recording(id, going); status != StoreStatus::Ok) {
        return status;
    }
    if (!going.hasAudio()) {

        return StoreStatus::Ok;
    }

    files.push_back(going.filePath);
    for (const core::RecordingStem &stem : going.stems) {
        if (!stem.filePath.empty() && stem.droppedAt.time_since_epoch().count() == 0) {
            files.push_back(stem.filePath);
        }
    }

    if (const auto status = execute("BEGIN IMMEDIATE;"); status != StoreStatus::Ok) {
        return status;
    }

    const auto undo = [this](StoreStatus status) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    };

    {
        const Statement statement{database_, R"sql(
            UPDATE recordings SET file_path = '', bytes = 0, updated_at = ? WHERE id = ?;
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return undo(remember(statement.prepareCode()));
        }
        statement.bindInt(1, secondsOf(std::chrono::system_clock::now()));
        statement.bindText(2, id);

        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return undo(remember(stepped));
        }
    }

    {
        const Statement statement{database_, R"sql(
            UPDATE recording_stems SET dropped_at = ? WHERE recording_id = ? AND dropped_at = 0;
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return undo(remember(statement.prepareCode()));
        }
        statement.bindInt(1, secondsOf(std::chrono::system_clock::now()));
        statement.bindText(2, id);

        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return undo(remember(stepped));
        }
    }

    return execute("COMMIT;");
}

StoreStatus SqliteStore::dropStems(const std::string &recordingId, std::vector<std::string> &files)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    files.clear();

    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    {
        const Statement statement{database_, R"sql(
            SELECT file_path FROM recording_stems
            WHERE recording_id = ? AND dropped_at = 0 AND file_path <> '';
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }
        statement.bindText(1, recordingId);

        while (sqlite3_step(statement.handle()) == SQLITE_ROW) {
            files.push_back(textAt(statement.handle(), 0));
        }
    }

    if (files.empty()) {
        return StoreStatus::Ok;
    }

    const Statement statement{database_, R"sql(
        UPDATE recording_stems SET dropped_at = ? WHERE recording_id = ? AND dropped_at = 0;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    statement.bindInt(1, secondsOf(std::chrono::system_clock::now()));
    statement.bindText(2, recordingId);

    if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
        return remember(stepped);
    }
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::noteAccess(const std::string &recordingId, core::RecordingAccess what,
                                    const std::string &detail)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const std::string id = core::makeUuid();
    const std::string action{core::toString(what)};

    const Statement statement{database_, R"sql(
        INSERT INTO access_log (id, recording_id, what, detail, at) VALUES (?, ?, ?, ?, ?);
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    statement.bindText(1, id);
    statement.bindText(2, recordingId);
    statement.bindText(3, action);
    statement.bindText(4, detail);
    statement.bindInt(5, secondsOf(std::chrono::system_clock::now()));

    if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
        return remember(stepped);
    }
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::accessLog(const std::string &recordingId,
                                   std::vector<core::AccessEntry> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    out.clear();

    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    std::string sql = "SELECT id, recording_id, what, detail, at FROM access_log";
    if (!recordingId.empty()) {
        sql += " WHERE recording_id = ?";
    }
    sql += " ORDER BY at DESC, rowid DESC;";

    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    if (!recordingId.empty()) {
        statement.bindText(1, recordingId);
    }

    while (true) {
        const auto stepped = sqlite3_step(statement.handle());
        if (stepped == SQLITE_DONE) {
            break;
        }
        if (stepped != SQLITE_ROW) {
            return remember(stepped);
        }

        core::AccessEntry entry;
        entry.id = textAt(statement.handle(), 0);
        entry.recordingId = textAt(statement.handle(), 1);
        const auto what = textAt(statement.handle(), 2);
        entry.what = what == "created"    ? core::RecordingAccess::Created
                     : what == "exported" ? core::RecordingAccess::Exported
                     : what == "deleted"  ? core::RecordingAccess::Deleted
                     : what == "expired"  ? core::RecordingAccess::Expired
                                          : core::RecordingAccess::Played;
        entry.detail = textAt(statement.handle(), 3);
        entry.at = timeAt(statement.handle(), 4);
        out.push_back(std::move(entry));
    }

    return StoreStatus::Ok;
}

namespace {

constexpr auto kProviderColumns =
    "id, kind, type, name, endpoint, model, region, params, has_secret, enabled, is_default, "
    "created_at, updated_at, in_price_micros, out_price_micros, currency, read_timeout_seconds, "
    "realtime_endpoint, is_realtime_default, realtime_model, realtime_price_micros, language, "
    "models_seen";

[[nodiscard]] core::ProviderConfig providerFrom(sqlite3_stmt *statement)
{
    core::ProviderConfig provider;
    provider.id = textAt(statement, 0);
    provider.kind = core::parseProviderKind(textAt(statement, 1)).value_or(core::ProviderKind::Asr);
    provider.type = textAt(statement, 2);
    provider.name = textAt(statement, 3);
    provider.endpoint = textAt(statement, 4);
    provider.model = textAt(statement, 5);
    provider.region = textAt(statement, 6);
    provider.params = core::decodeParams(textAt(statement, 7));
    provider.hasSecret = sqlite3_column_int(statement, 8) != 0;
    provider.enabled = sqlite3_column_int(statement, 9) != 0;
    provider.isDefault = sqlite3_column_int(statement, 10) != 0;
    provider.createdAt = timeAt(statement, 11);
    provider.updatedAt = timeAt(statement, 12);
    provider.inPricePerMillion = sqlite3_column_int64(statement, 13);
    provider.outPricePerMillion = sqlite3_column_int64(statement, 14);
    provider.currency = textAt(statement, 15);
    provider.readTimeout = std::chrono::seconds{sqlite3_column_int64(statement, 16)};
    provider.realtimeEndpoint = textAt(statement, 17);
    provider.isRealtimeDefault = sqlite3_column_int(statement, 18) != 0;
    provider.realtimeModel = textAt(statement, 19);
    provider.realtimePriceMicros = sqlite3_column_int64(statement, 20);
    provider.language = textAt(statement, 21);
    provider.modelsSeen = core::decodeNames(textAt(statement, 22));
    return provider;
}

}

StoreStatus SqliteStore::save(core::ProviderConfig &provider)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }
    if (provider.id.empty()) {
        provider.id = core::makeUuid();
    }

    const auto now = std::chrono::system_clock::now();
    if (provider.createdAt.time_since_epoch().count() == 0) {
        provider.createdAt = now;
    }
    provider.updatedAt = now;

    const auto kind = std::string{core::toString(provider.kind)};
    const auto params = core::encodeParams(provider.params);
    const auto names = core::encodeNames(provider.modelsSeen);

    if (const auto status = execute("BEGIN IMMEDIATE;"); status != StoreStatus::Ok) {
        return status;
    }

    const auto give = [this](StoreStatus status) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    };

    {
        const Statement statement{database_, R"sql(
            INSERT INTO providers
                (id, kind, type, name, endpoint, model, region, params, has_secret, enabled,
                 is_default, created_at, updated_at, in_price_micros, out_price_micros, currency,
                 read_timeout_seconds, realtime_endpoint, is_realtime_default,
                 realtime_model, realtime_price_micros, language, models_seen)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ON CONFLICT(id) DO UPDATE SET
                kind = excluded.kind, type = excluded.type, name = excluded.name,
                endpoint = excluded.endpoint, model = excluded.model, region = excluded.region,
                params = excluded.params, has_secret = excluded.has_secret,
                enabled = excluded.enabled, is_default = excluded.is_default,
                updated_at = excluded.updated_at,
                in_price_micros = excluded.in_price_micros,
                out_price_micros = excluded.out_price_micros, currency = excluded.currency,
                read_timeout_seconds = excluded.read_timeout_seconds,
                realtime_endpoint = excluded.realtime_endpoint,
                is_realtime_default = excluded.is_realtime_default,
                realtime_model = excluded.realtime_model,
                realtime_price_micros = excluded.realtime_price_micros,
                language = excluded.language,
                models_seen = excluded.models_seen;
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return give(remember(statement.prepareCode()));
        }
        statement.bindText(1, provider.id);
        statement.bindText(2, kind);
        statement.bindText(3, provider.type);
        statement.bindText(4, provider.name);
        statement.bindText(5, provider.endpoint);
        statement.bindText(6, provider.model);
        statement.bindText(7, provider.region);
        statement.bindText(8, params);
        statement.bindInt(9, provider.hasSecret ? 1 : 0);
        statement.bindInt(10, provider.enabled ? 1 : 0);
        statement.bindInt(11, provider.isDefault ? 1 : 0);
        statement.bindInt(12, secondsOf(provider.createdAt));
        statement.bindInt(13, secondsOf(provider.updatedAt));
        statement.bindInt(14, provider.inPricePerMillion);
        statement.bindInt(15, provider.outPricePerMillion);
        statement.bindText(16, provider.currency);
        statement.bindInt(17, provider.readTimeout.count());
        statement.bindText(18, provider.realtimeEndpoint);
        statement.bindInt(19, provider.isRealtimeDefault ? 1 : 0);
        statement.bindText(20, provider.realtimeModel);
        statement.bindInt(21, provider.realtimePriceMicros);
        statement.bindText(22, provider.language);
        statement.bindText(23, names);
        if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
            return give(remember(code));
        }
    }

    const auto onlyOne = [&](const char *sql) {
        const Statement statement{database_, sql};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }
        statement.bindText(1, kind);
        statement.bindText(2, provider.id);
        if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
            return remember(code);
        }
        return StoreStatus::Ok;
    };

    if (provider.isDefault) {
        if (const auto status = onlyOne("UPDATE providers SET is_default = 0 "
                                        "WHERE kind = ? AND id <> ?;");
            status != StoreStatus::Ok) {
            return give(status);
        }
    }
    if (provider.isRealtimeDefault) {
        if (const auto status = onlyOne("UPDATE providers SET is_realtime_default = 0 "
                                        "WHERE kind = ? AND id <> ?;");
            status != StoreStatus::Ok) {
            return give(status);
        }
    }

    return execute("COMMIT;");
}

StoreStatus SqliteStore::providers(core::ProviderKind kind, std::vector<core::ProviderConfig> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto sql = std::string{"SELECT "} + kProviderColumns +
                     " FROM providers WHERE kind = ? ORDER BY created_at, id;";
    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    const auto text = std::string{core::toString(kind)};
    statement.bindText(1, text);

    for (;;) {
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            break;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        out.push_back(providerFrom(statement.handle()));
    }
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::provider(const std::string &id, core::ProviderConfig &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto sql = std::string{"SELECT "} + kProviderColumns + " FROM providers WHERE id = ?;";
    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, id);

    const auto code = sqlite3_step(statement.handle());
    if (code == SQLITE_DONE) {
        return StoreStatus::Missing;
    }
    if (code != SQLITE_ROW) {
        return remember(code);
    }
    out = providerFrom(statement.handle());
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::defaultProvider(core::ProviderKind kind, core::ProviderConfig &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto sql = std::string{"SELECT "} + kProviderColumns +
                     " FROM providers WHERE kind = ? AND is_default <> 0 AND enabled <> 0 "
                     "ORDER BY created_at, id LIMIT 1;";
    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    const auto text = std::string{core::toString(kind)};
    statement.bindText(1, text);

    const auto code = sqlite3_step(statement.handle());
    if (code == SQLITE_DONE) {
        return StoreStatus::Missing;
    }
    if (code != SQLITE_ROW) {
        return remember(code);
    }
    out = providerFrom(statement.handle());
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::removeProvider(const std::string &id)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, "DELETE FROM providers WHERE id = ?;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, id);
    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return sqlite3_changes(database_) > 0 ? StoreStatus::Ok : StoreStatus::Missing;
}

namespace {

constexpr auto kJobColumns =
    "id, kind, recording_id, transcript_id, provider_id, template_id, automation_id, state, "
    "attempts, next_attempt_at, error, priority, params, cost_micros, created_at, updated_at";

[[nodiscard]] core::Job jobFrom(sqlite3_stmt *statement)
{
    core::Job job;
    job.id = textAt(statement, 0);
    job.kind = core::parseJobKind(textAt(statement, 1)).value_or(core::JobKind::Transcribe);
    job.recordingId = textAt(statement, 2);
    job.transcriptId = textAt(statement, 3);
    job.providerId = textAt(statement, 4);
    job.templateId = textAt(statement, 5);
    job.automationId = textAt(statement, 6);
    job.state = core::parseJobState(textAt(statement, 7)).value_or(core::JobState::Queued);
    job.attempts = static_cast<unsigned>(sqlite3_column_int(statement, 8));
    job.nextAttemptAt = timeAt(statement, 9);
    job.error = textAt(statement, 10);
    job.priority = sqlite3_column_int(statement, 11);
    job.params = textAt(statement, 12);
    job.costMicros = sqlite3_column_int64(statement, 13);
    job.createdAt = timeAt(statement, 14);
    job.updatedAt = timeAt(statement, 15);
    return job;
}

}

StoreStatus SqliteStore::save(core::Job &job)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }
    if (job.id.empty()) {
        job.id = core::makeUuid();
    }

    const auto kind = std::string{core::toString(job.kind)};
    const auto state = std::string{core::toString(job.state)};

    const Statement statement{database_, R"sql(
        INSERT INTO jobs
            (id, kind, recording_id, transcript_id, provider_id, template_id, automation_id,
             state, attempts, next_attempt_at, error, priority, params, cost_micros,
             created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(id) DO UPDATE SET
            kind = excluded.kind, recording_id = excluded.recording_id,
            transcript_id = excluded.transcript_id, provider_id = excluded.provider_id,
            template_id = excluded.template_id, automation_id = excluded.automation_id,
            state = excluded.state, attempts = excluded.attempts,
            next_attempt_at = excluded.next_attempt_at, error = excluded.error,
            priority = excluded.priority, params = excluded.params,
            cost_micros = excluded.cost_micros, updated_at = excluded.updated_at;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, job.id);
    statement.bindText(2, kind);
    statement.bindText(3, job.recordingId);
    statement.bindText(4, job.transcriptId);
    statement.bindText(5, job.providerId);
    statement.bindText(6, job.templateId);
    statement.bindText(7, job.automationId);
    statement.bindText(8, state);
    statement.bindInt(9, job.attempts);
    statement.bindInt(10, secondsOf(job.nextAttemptAt));
    statement.bindText(11, job.error);
    statement.bindInt(12, job.priority);
    statement.bindText(13, job.params);
    statement.bindInt(14, job.costMicros);
    statement.bindInt(15, secondsOf(job.createdAt));
    statement.bindInt(16, secondsOf(job.updatedAt));

    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::job(const std::string &id, core::Job &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto sql = std::string{"SELECT "} + kJobColumns + " FROM jobs WHERE id = ?;";
    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, id);

    const auto code = sqlite3_step(statement.handle());
    if (code == SQLITE_DONE) {
        return StoreStatus::Missing;
    }
    if (code != SQLITE_ROW) {
        return remember(code);
    }
    out = jobFrom(statement.handle());
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::jobs(const core::JobQuery &query, std::vector<core::Job> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    auto sql = std::string{"SELECT "} + kJobColumns + " FROM jobs WHERE 1 = 1";
    if (query.kind.has_value()) {
        sql += " AND kind = ?";
    }
    if (query.state.has_value()) {
        sql += " AND state = ?";
    }
    if (!query.recordingId.empty()) {
        sql += " AND recording_id = ?";
    }
    sql += query.newestFirst ? " ORDER BY created_at DESC, id DESC" : " ORDER BY created_at, id";
    sql += " LIMIT ?;";

    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    int index = 1;
    const auto kind =
        query.kind.has_value() ? std::string{core::toString(*query.kind)} : std::string{};
    const auto state =
        query.state.has_value() ? std::string{core::toString(*query.state)} : std::string{};
    if (query.kind.has_value()) {
        statement.bindText(index++, kind);
    }
    if (query.state.has_value()) {
        statement.bindText(index++, state);
    }
    if (!query.recordingId.empty()) {
        statement.bindText(index++, query.recordingId);
    }
    statement.bindInt(index, static_cast<std::int64_t>(query.limit));

    for (;;) {
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            break;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        out.push_back(jobFrom(statement.handle()));
    }
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::claim(std::chrono::system_clock::time_point now, core::Job &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (const auto status = execute("BEGIN IMMEDIATE;"); status != StoreStatus::Ok) {
        return status;
    }

    const auto give = [this](StoreStatus status) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    };

    std::string id;
    {
        const Statement statement{database_, R"sql(
            SELECT id FROM jobs
            WHERE state = 'queued' AND next_attempt_at <= ?
            ORDER BY priority DESC, created_at, id
            LIMIT 1;
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return give(remember(statement.prepareCode()));
        }
        statement.bindInt(1, secondsOf(now));

        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            static_cast<void>(execute("COMMIT;"));
            return StoreStatus::Missing;
        }
        if (code != SQLITE_ROW) {
            return give(remember(code));
        }
        id = textAt(statement.handle(), 0);
    }

    {
        const Statement statement{database_, R"sql(
            UPDATE jobs SET state = 'running', attempts = attempts + 1, updated_at = ?
            WHERE id = ?;
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return give(remember(statement.prepareCode()));
        }
        statement.bindInt(1, secondsOf(now));
        statement.bindText(2, id);
        if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
            return give(remember(code));
        }
    }

    if (const auto status = execute("COMMIT;"); status != StoreStatus::Ok) {
        return status;
    }
    return job(id, out);
}

StoreStatus SqliteStore::reviveRunning(std::size_t &revived)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    revived = 0;
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, R"sql(
        UPDATE jobs SET state = 'queued', next_attempt_at = 0 WHERE state = 'running';
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    revived = static_cast<std::size_t>(std::max(sqlite3_changes(database_), 0));
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::cancelJob(const std::string &id)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, R"sql(
        UPDATE jobs SET state = 'cancelled'
        WHERE id = ? AND state IN ('queued', 'running');
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, id);
    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return sqlite3_changes(database_) > 0 ? StoreStatus::Ok : StoreStatus::Missing;
}

StoreStatus SqliteStore::forgetDone(std::chrono::system_clock::time_point before,
                                    std::size_t &removed)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    removed = 0;
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, R"sql(
        DELETE FROM jobs WHERE state IN ('done', 'cancelled') AND updated_at < ?;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindInt(1, secondsOf(before));
    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    removed = static_cast<std::size_t>(std::max(sqlite3_changes(database_), 0));
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::forgetJob(const std::string &id)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};

    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, "DELETE FROM jobs WHERE id = ?;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, id);
    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return sqlite3_changes(database_) > 0 ? StoreStatus::Ok : StoreStatus::Missing;
}

namespace {

constexpr auto kTranscriptColumns =
    "id, recording_id, provider_id, provider_type, model, requested_language, "
    "detected_language, status, is_primary, requested_at, completed_at, error, word_count, "
    "audio_ms, cost_micros, raw_path, source";

[[nodiscard]] core::Transcript transcriptFrom(sqlite3_stmt *row)
{
    core::Transcript transcript;
    transcript.id = textAt(row, 0);
    transcript.recordingId = textAt(row, 1);
    transcript.providerId = textAt(row, 2);
    transcript.providerType = textAt(row, 3);
    transcript.model = textAt(row, 4);
    transcript.requestedLanguage = textAt(row, 5);
    transcript.detectedLanguage = textAt(row, 6);
    transcript.status =
        core::parseTranscriptStatus(textAt(row, 7)).value_or(core::TranscriptStatus::Queued);
    transcript.isPrimary = sqlite3_column_int(row, 8) != 0;
    transcript.requestedAt = timeAt(row, 9);
    transcript.completedAt = timeAt(row, 10);
    transcript.error = textAt(row, 11);
    transcript.wordCount = static_cast<std::size_t>(sqlite3_column_int64(row, 12));
    transcript.audio = std::chrono::milliseconds{sqlite3_column_int64(row, 13)};
    transcript.costMicros = sqlite3_column_int64(row, 14);
    transcript.rawPath = textAt(row, 15);
    transcript.source = textAt(row, 16);
    if (transcript.source.empty()) {

        transcript.source = "batch";
    }
    return transcript;
}

}

StoreStatus SqliteStore::writeSegments(const core::Transcript &transcript)
{
    {
        const Statement clear{database_,
                              "DELETE FROM transcript_segments WHERE transcript_id = ?;"};
        if (clear.prepareCode() != SQLITE_OK) {
            return remember(clear.prepareCode());
        }
        clear.bindText(1, transcript.id);
        if (const auto stepped = sqlite3_step(clear.handle()); stepped != SQLITE_DONE) {
            return remember(stepped);
        }
    }

    for (const auto &segment : transcript.segments) {
        const Statement statement{database_, R"sql(
            INSERT INTO transcript_segments
                (transcript_id, ordinal, start_ms, end_ms, voice, speaker, text, edited_text,
                 confidence, words)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }
        const auto words = core::encodeWords(segment.words);
        statement.bindText(1, transcript.id);
        statement.bindInt(2, segment.ordinal);
        statement.bindInt(3, segment.start.count());
        statement.bindInt(4, segment.end.count());
        statement.bindText(5, segment.voice);
        statement.bindText(6, segment.speaker);
        statement.bindText(7, segment.text);
        statement.bindText(8, segment.editedText);
        sqlite3_bind_double(statement.handle(), 9, segment.confidence);
        statement.bindText(10, words);
        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return remember(stepped);
        }
    }
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::readSegments(core::Transcript &transcript)
{
    const Statement statement{database_, R"sql(
        SELECT ordinal, start_ms, end_ms, voice, speaker, text, edited_text, confidence, words
        FROM transcript_segments WHERE transcript_id = ? ORDER BY ordinal;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, transcript.id);

    for (;;) {
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            break;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        core::TranscriptSegment segment;
        segment.ordinal = sqlite3_column_int(statement.handle(), 0);
        segment.start = std::chrono::milliseconds{sqlite3_column_int64(statement.handle(), 1)};
        segment.end = std::chrono::milliseconds{sqlite3_column_int64(statement.handle(), 2)};
        segment.voice = textAt(statement.handle(), 3);
        segment.speaker = textAt(statement.handle(), 4);
        segment.text = textAt(statement.handle(), 5);
        segment.editedText = textAt(statement.handle(), 6);
        segment.confidence = sqlite3_column_double(statement.handle(), 7);
        segment.words = core::decodeWords(textAt(statement.handle(), 8));
        transcript.segments.push_back(std::move(segment));
    }
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::countTranscriptsOn(const std::string &recordingId)
{
    if (recordingId.empty()) {
        return StoreStatus::Ok;
    }

    const Statement statement{database_, R"sql(
        UPDATE recordings SET transcript_count =
            (SELECT COUNT(*) FROM transcripts WHERE transcripts.recording_id = ?)
        WHERE id = ?;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, recordingId);
    statement.bindText(2, recordingId);
    if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
        return remember(stepped);
    }
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::save(core::Transcript &transcript)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }
    if (transcript.id.empty()) {
        transcript.id = core::makeUuid();
    }

    const auto status = std::string{core::toString(transcript.status)};

    const auto source = transcript.source.empty() ? std::string{"batch"} : transcript.source;

    if (const auto began = execute("BEGIN IMMEDIATE;"); began != StoreStatus::Ok) {
        return began;
    }
    const auto undo = [this](StoreStatus failed) {
        static_cast<void>(execute("ROLLBACK;"));
        return failed;
    };

    if (transcript.source == "realtime") {
        transcript.isPrimary = false;
    } else if (transcript.status == core::TranscriptStatus::Done && !transcript.isPrimary) {
        const Statement asking{database_, R"sql(
            SELECT COUNT(*) FROM transcripts
            WHERE recording_id = ? AND is_primary <> 0 AND id <> ?;
        )sql"};
        if (asking.prepareCode() != SQLITE_OK) {
            return undo(remember(asking.prepareCode()));
        }
        asking.bindText(1, transcript.recordingId);
        asking.bindText(2, transcript.id);
        if (sqlite3_step(asking.handle()) == SQLITE_ROW) {
            transcript.isPrimary = sqlite3_column_int(asking.handle(), 0) == 0;
        }
    }

    {
        const Statement statement{database_, R"sql(
            INSERT INTO transcripts
                (id, recording_id, provider_id, provider_type, model, requested_language,
                 detected_language, status, is_primary, requested_at, completed_at, error,
                 word_count, audio_ms, cost_micros, raw_path, source)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ON CONFLICT(id) DO UPDATE SET
                recording_id = excluded.recording_id, provider_id = excluded.provider_id,
                provider_type = excluded.provider_type, model = excluded.model,
                requested_language = excluded.requested_language,
                detected_language = excluded.detected_language, status = excluded.status,
                is_primary = excluded.is_primary, requested_at = excluded.requested_at,
                completed_at = excluded.completed_at, error = excluded.error,
                word_count = excluded.word_count, audio_ms = excluded.audio_ms,
                cost_micros = excluded.cost_micros, raw_path = excluded.raw_path,
                source = excluded.source;
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return undo(remember(statement.prepareCode()));
        }
        statement.bindText(1, transcript.id);
        statement.bindText(2, transcript.recordingId);
        statement.bindText(3, transcript.providerId);
        statement.bindText(4, transcript.providerType);
        statement.bindText(5, transcript.model);
        statement.bindText(6, transcript.requestedLanguage);
        statement.bindText(7, transcript.detectedLanguage);
        statement.bindText(8, status);
        statement.bindInt(9, transcript.isPrimary ? 1 : 0);
        statement.bindInt(10, secondsOf(transcript.requestedAt));
        statement.bindInt(11, secondsOf(transcript.completedAt));
        statement.bindText(12, transcript.error);
        statement.bindInt(13, static_cast<std::int64_t>(transcript.wordCount));
        statement.bindInt(14, transcript.audio.count());
        statement.bindInt(15, transcript.costMicros);
        statement.bindText(16, transcript.rawPath);
        statement.bindText(17, source);
        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return undo(remember(stepped));
        }
    }

    if (transcript.isPrimary) {
        const Statement statement{database_, R"sql(
            UPDATE transcripts SET is_primary = 0 WHERE recording_id = ? AND id <> ?;
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return undo(remember(statement.prepareCode()));
        }
        statement.bindText(1, transcript.recordingId);
        statement.bindText(2, transcript.id);
        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return undo(remember(stepped));
        }
    }

    if (const auto wrote = writeSegments(transcript); wrote != StoreStatus::Ok) {
        return undo(wrote);
    }
    if (const auto counted = countTranscriptsOn(transcript.recordingId);
        counted != StoreStatus::Ok) {
        return undo(counted);
    }

    if (transcript.isPrimary && transcript.status == core::TranscriptStatus::Done) {
        const auto spoken = transcript.detectedLanguage.empty() ? transcript.requestedLanguage
                                                                : transcript.detectedLanguage;
        const Statement statement{database_,
                                  "UPDATE recordings SET content_language = ? WHERE id = ?;"};
        if (statement.prepareCode() != SQLITE_OK) {
            return undo(remember(statement.prepareCode()));
        }
        statement.bindText(1, spoken);
        statement.bindText(2, transcript.recordingId);
        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return undo(remember(stepped));
        }
    }

    return execute("COMMIT;");
}

StoreStatus SqliteStore::transcripts(const std::string &recordingId,
                                     std::vector<core::Transcript> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto sql = std::string{"SELECT "} + kTranscriptColumns +
                     " FROM transcripts WHERE recording_id = ? "
                     "ORDER BY requested_at DESC, id DESC;";
    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, recordingId);

    for (;;) {
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            break;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        out.push_back(transcriptFrom(statement.handle()));
    }
    return StoreStatus::Ok;
}

StoreStatus SqliteStore::transcript(const std::string &id, core::Transcript &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto sql =
        std::string{"SELECT "} + kTranscriptColumns + " FROM transcripts WHERE id = ?;";
    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, id);

    const auto code = sqlite3_step(statement.handle());
    if (code == SQLITE_DONE) {
        return StoreStatus::Missing;
    }
    if (code != SQLITE_ROW) {
        return remember(code);
    }
    out = transcriptFrom(statement.handle());
    return readSegments(out);
}

StoreStatus SqliteStore::primaryTranscript(const std::string &recordingId, core::Transcript &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto sql = std::string{"SELECT "} + kTranscriptColumns +
                     " FROM transcripts WHERE recording_id = ? AND is_primary <> 0 LIMIT 1;";
    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, recordingId);

    const auto code = sqlite3_step(statement.handle());
    if (code == SQLITE_DONE) {
        return StoreStatus::Missing;
    }
    if (code != SQLITE_ROW) {
        return remember(code);
    }
    out = transcriptFrom(statement.handle());
    return readSegments(out);
}

StoreStatus SqliteStore::makePrimary(const std::string &id)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    core::Transcript chosen;
    {
        const auto sql =
            std::string{"SELECT "} + kTranscriptColumns + " FROM transcripts WHERE id = ?;";
        const Statement statement{database_, sql};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }
        statement.bindText(1, id);
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            return StoreStatus::Missing;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        chosen = transcriptFrom(statement.handle());
    }

    if (const auto began = execute("BEGIN IMMEDIATE;"); began != StoreStatus::Ok) {
        return began;
    }
    const auto undo = [this](StoreStatus failed) {
        static_cast<void>(execute("ROLLBACK;"));
        return failed;
    };

    {
        const Statement statement{database_,
                                  "UPDATE transcripts SET is_primary = 0 WHERE recording_id = ?;"};
        if (statement.prepareCode() != SQLITE_OK) {
            return undo(remember(statement.prepareCode()));
        }
        statement.bindText(1, chosen.recordingId);
        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return undo(remember(stepped));
        }
    }
    {
        const Statement statement{database_, "UPDATE transcripts SET is_primary = 1 WHERE id = ?;"};
        if (statement.prepareCode() != SQLITE_OK) {
            return undo(remember(statement.prepareCode()));
        }
        statement.bindText(1, id);
        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return undo(remember(stepped));
        }
    }

    return execute("COMMIT;");
}

StoreStatus SqliteStore::removeTranscript(const std::string &id, std::vector<std::string> &files)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    files.clear();

    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    core::Transcript going;
    {
        const auto sql =
            std::string{"SELECT "} + kTranscriptColumns + " FROM transcripts WHERE id = ?;";
        const Statement statement{database_, sql};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }
        statement.bindText(1, id);
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            return StoreStatus::Missing;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        going = transcriptFrom(statement.handle());
    }

    if (!going.rawPath.empty()) {
        files.push_back(going.rawPath);
    }

    if (const auto began = execute("BEGIN IMMEDIATE;"); began != StoreStatus::Ok) {
        return began;
    }
    const auto undo = [this](StoreStatus failed) {
        static_cast<void>(execute("ROLLBACK;"));
        return failed;
    };

    for (const char *sql : {"DELETE FROM transcript_segments WHERE transcript_id = ?;",
                            "DELETE FROM transcripts WHERE id = ?;"}) {
        const Statement statement{database_, sql};
        if (statement.prepareCode() != SQLITE_OK) {
            return undo(remember(statement.prepareCode()));
        }
        statement.bindText(1, id);
        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return undo(remember(stepped));
        }
    }

    {
        const Statement statement{database_, R"sql(
            UPDATE transcripts SET is_primary = 1
            WHERE id = (SELECT id FROM transcripts
                        WHERE recording_id = ? AND status = 'done'
                        ORDER BY requested_at, id LIMIT 1)
              AND NOT EXISTS (SELECT 1 FROM transcripts
                              WHERE recording_id = ? AND is_primary <> 0);
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return undo(remember(statement.prepareCode()));
        }
        statement.bindText(1, going.recordingId);
        statement.bindText(2, going.recordingId);
        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return undo(remember(stepped));
        }
    }

    if (const auto counted = countTranscriptsOn(going.recordingId); counted != StoreStatus::Ok) {
        return undo(counted);
    }

    return execute("COMMIT;");
}

StoreStatus SqliteStore::search(const core::TranscriptSearch &query,
                                std::vector<core::TranscriptHit> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto match = asMatchQuery(query.words);
    if (match.empty()) {
        return StoreStatus::Ok;
    }

    auto sql = std::string{R"sql(
        SELECT t.recording_id, s.transcript_id, s.ordinal, s.start_ms, s.voice,
               snippet(transcript_search, 0, char(2), char(3), '…', 24)
        FROM transcript_search
        JOIN transcript_segments s ON s.rowid = transcript_search.rowid
        JOIN transcripts t ON t.id = s.transcript_id
        JOIN recordings r ON r.id = t.recording_id
        WHERE transcript_search MATCH ? AND t.is_primary <> 0 AND r.deleted_at = 0
    )sql"};
    if (!query.recordingId.empty()) {
        sql += " AND t.recording_id = ?";
    }
    sql += " ORDER BY rank LIMIT ?;";

    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    int index = 1;
    statement.bindText(index++, match);
    if (!query.recordingId.empty()) {
        statement.bindText(index++, query.recordingId);
    }
    statement.bindInt(index, static_cast<std::int64_t>(query.limit));

    for (;;) {
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            break;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        core::TranscriptHit hit;
        hit.recordingId = textAt(statement.handle(), 0);
        hit.transcriptId = textAt(statement.handle(), 1);
        hit.ordinal = sqlite3_column_int(statement.handle(), 2);
        hit.start = std::chrono::milliseconds{sqlite3_column_int64(statement.handle(), 3)};
        hit.voice = textAt(statement.handle(), 4);
        hit.text = textAt(statement.handle(), 5);
        out.push_back(std::move(hit));
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::transcriptLanguages(std::vector<std::string> &languages)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    languages.clear();

    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{
        database_, "SELECT DISTINCT CASE WHEN detected_language <> '' THEN detected_language "
                   "ELSE requested_language END AS spoken FROM transcripts "
                   "WHERE is_primary <> 0 ORDER BY spoken;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    for (;;) {
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            break;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        if (auto value = textAt(statement.handle(), 0); !value.empty()) {
            languages.push_back(std::move(value));
        }
    }
    return StoreStatus::Ok;
}

namespace {

constexpr const char *kTaxonomyColumns = "id, kind, code, title, description, "
                                         "severity, colour, ordinal, built_in, retired_at, "
                                         "created_at, updated_at";

[[nodiscard]] core::TaxonomyEntry taxonomyFrom(sqlite3_stmt *statement)
{
    core::TaxonomyEntry entry;
    entry.id = textAt(statement, 0);
    entry.kind = core::parseTaxonomyKind(textAt(statement, 1)).value_or(entry.kind);
    entry.code = textAt(statement, 2);
    entry.title = textAt(statement, 3);
    entry.description = textAt(statement, 4);
    entry.severity = core::parseSeverity(textAt(statement, 5));
    entry.colour = textAt(statement, 6);
    entry.ordinal = sqlite3_column_int(statement, 7);
    entry.builtIn = sqlite3_column_int(statement, 8) != 0;
    if (const auto retired = sqlite3_column_int64(statement, 9); retired != 0) {
        entry.retiredAt = std::chrono::system_clock::time_point{std::chrono::seconds{retired}};
    }
    entry.createdAt = timeAt(statement, 10);
    entry.updatedAt = timeAt(statement, 11);
    return entry;
}

}

core::StoreStatus SqliteStore::alreadySeeded(std::string_view kind, std::set<std::string> &codes)
{
    const Statement statement{database_, "SELECT code FROM seeded WHERE kind = ?;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    const auto named = std::string{kind};
    statement.bindText(1, named);

    for (;;) {
        const auto stepped = sqlite3_step(statement.handle());
        if (stepped == SQLITE_DONE) {
            return StoreStatus::Ok;
        }
        if (stepped != SQLITE_ROW) {
            return remember(stepped);
        }
        codes.insert(textAt(statement.handle(), 0));
    }
}

core::StoreStatus SqliteStore::rememberSeeded(std::string_view kind,
                                              const std::vector<std::string> &codes)
{
    if (codes.empty()) {
        return StoreStatus::Ok;
    }

    const Statement statement{database_, R"sql(
        INSERT OR IGNORE INTO seeded (kind, code, seeded_at) VALUES (?, ?, ?);
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    const auto named = std::string{kind};
    const auto now = secondsOf(std::chrono::system_clock::now());
    for (const auto &code : codes) {
        sqlite3_reset(statement.handle());
        statement.bindText(1, named);
        statement.bindText(2, code);
        statement.bindInt(3, now);
        if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
            return remember(stepped);
        }
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::installShippedPrompts()
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (const auto status = execute("BEGIN IMMEDIATE;"); status != StoreStatus::Ok) {
        return status;
    }

    struct Part {
        const char *kind;
        core::StoreStatus (SqliteStore::*install)(std::vector<std::string> &);
    };

    constexpr Part kParts[] = {
        {"template", &SqliteStore::installTemplates},
        {"automation", &SqliteStore::installAutomations},
    };

    int installed = 0;
    for (const auto &part : kParts) {
        std::vector<std::string> written;
        if (const auto status = (this->*part.install)(written); status != StoreStatus::Ok) {
            static_cast<void>(execute("ROLLBACK;"));
            return status;
        }
        if (const auto status = rememberSeeded(part.kind, written); status != StoreStatus::Ok) {
            static_cast<void>(execute("ROLLBACK;"));
            return status;
        }
        installed += static_cast<int>(written.size());
    }

    if (installed > 0) {
        core::Logger::instance().log(core::LogLevel::Info, "storage",
                                     "offered " + std::to_string(installed) +
                                         " shipped prompts and rules this database had not seen");
    }

    if (const auto status = fillEmptyShippedPrompts(); status != StoreStatus::Ok) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    }

    return execute("COMMIT;");
}

core::StoreStatus SqliteStore::installTaxonomy(std::vector<std::string> &written)
{

    std::set<std::string> seen;
    if (const auto status = alreadySeeded("taxonomy", seen); status != StoreStatus::Ok) {
        return status;
    }

    const Statement statement{database_, R"sql(
        INSERT INTO taxonomy_entries
            (id, kind, code, title, description, severity, colour, ordinal,
             built_in, retired_at, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, '', ?, 1, 0, ?, ?);
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    const auto now = secondsOf(std::chrono::system_clock::now());
    for (const auto &entry : core::starterTaxonomy(contentLanguage_)) {

        const auto id = core::makeUuid();
        const auto kind = std::string{core::toString(entry.kind)};
        const auto noted = kind + "/" + entry.code;
        if (seen.count(noted) != 0) {
            continue;
        }
        const auto severity = entry.severity.has_value()
                                  ? std::string{core::toString(*entry.severity)}
                                  : std::string{};

        sqlite3_reset(statement.handle());
        statement.bindText(1, id);
        statement.bindText(2, kind);
        statement.bindText(3, entry.code);
        statement.bindText(4, entry.title);
        statement.bindText(5, entry.description);
        statement.bindText(6, severity);
        statement.bindInt(7, entry.ordinal);
        statement.bindInt(8, now);
        statement.bindInt(9, now);
        if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
            return remember(code);
        }
        written.push_back(noted);
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::save(core::TaxonomyEntry &entry)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }
    if (!core::validTaxonomyCode(entry.code)) {
        lastError_ = "a taxonomy code must be lowercase letters, digits and hyphens";
        return StoreStatus::Failed;
    }

    if (entry.id.empty()) {
        entry.id = core::makeUuid();
    }
    const auto now = std::chrono::system_clock::now();
    if (entry.createdAt.time_since_epoch().count() == 0) {
        entry.createdAt = now;
    }
    entry.updatedAt = now;

    const auto kindText = std::string{core::toString(entry.kind)};
    const auto severityText =
        entry.severity.has_value() ? std::string{core::toString(*entry.severity)} : std::string{};

    const Statement statement{database_, R"sql(
        INSERT INTO taxonomy_entries
            (id, kind, code, title, description, severity, colour, ordinal,
             built_in, retired_at, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(id) DO UPDATE SET
            kind = excluded.kind, code = excluded.code,
            title = excluded.title, description = excluded.description,
            severity = excluded.severity, colour = excluded.colour, ordinal = excluded.ordinal,
            built_in = excluded.built_in, retired_at = excluded.retired_at,
            updated_at = excluded.updated_at;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, entry.id);
    statement.bindText(2, kindText);
    statement.bindText(3, entry.code);
    statement.bindText(4, entry.title);
    statement.bindText(5, entry.description);
    statement.bindText(6, severityText);
    statement.bindText(7, entry.colour);
    statement.bindInt(8, entry.ordinal);
    statement.bindInt(9, entry.builtIn ? 1 : 0);
    statement.bindInt(10, entry.retiredAt.has_value() ? secondsOf(*entry.retiredAt) : 0);
    statement.bindInt(11, secondsOf(entry.createdAt));
    statement.bindInt(12, secondsOf(entry.updatedAt));

    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::entries(core::TaxonomyKind kind, bool includeRetired,
                                       std::vector<core::TaxonomyEntry> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    auto sql = std::string{"SELECT "} + kTaxonomyColumns + " FROM taxonomy_entries WHERE kind = ?";
    if (!includeRetired) {
        sql += " AND retired_at = 0";
    }

    sql += " ORDER BY ordinal, code;";

    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    const auto kindText = std::string{core::toString(kind)};
    statement.bindText(1, kindText);

    for (;;) {
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            break;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        out.push_back(taxonomyFrom(statement.handle()));
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::entry(core::TaxonomyKind kind, const std::string &code,
                                     core::TaxonomyEntry &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto sql = std::string{"SELECT "} + kTaxonomyColumns +
                     " FROM taxonomy_entries WHERE kind = ? AND code = ?;";
    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    const auto kindText = std::string{core::toString(kind)};
    statement.bindText(1, kindText);
    statement.bindText(2, code);

    const auto stepped = sqlite3_step(statement.handle());
    if (stepped == SQLITE_DONE) {

        return StoreStatus::Missing;
    }
    if (stepped != SQLITE_ROW) {
        return remember(stepped);
    }
    out = taxonomyFrom(statement.handle());
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::retireEntry(const std::string &id, bool retired)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, R"sql(
        UPDATE taxonomy_entries SET retired_at = ?, updated_at = ? WHERE id = ?;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    const auto now = secondsOf(std::chrono::system_clock::now());
    statement.bindInt(1, retired ? now : 0);
    statement.bindInt(2, now);
    statement.bindText(3, id);

    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return sqlite3_changes(database_) == 0 ? StoreStatus::Missing : StoreStatus::Ok;
}

core::StoreStatus SqliteStore::restoreShippedTaxonomy()
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (const auto status = execute("BEGIN IMMEDIATE;"); status != StoreStatus::Ok) {
        return status;
    }

    if (const auto status = execute("DELETE FROM taxonomy_entries;"
                                    "DELETE FROM seeded WHERE kind = 'taxonomy';");
        status != StoreStatus::Ok) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    }

    std::vector<std::string> written;
    if (const auto status = installTaxonomy(written); status != StoreStatus::Ok) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    }
    if (const auto status = rememberSeeded("taxonomy", written); status != StoreStatus::Ok) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    }
    return execute("COMMIT;");
}

core::StoreStatus SqliteStore::taxonomyIsUsed(core::TaxonomyKind kind, const std::string &code,
                                              bool &used)
{
    used = false;

    std::vector<std::string> queries;
    switch (kind) {
    case core::TaxonomyKind::Category:
        queries.emplace_back("SELECT 1 FROM recordings WHERE category_code = ? LIMIT 1;");
        break;
    case core::TaxonomyKind::Tag:
        queries.emplace_back("SELECT 1 FROM recording_tags WHERE tag_code = ? LIMIT 1;");
        break;
    case core::TaxonomyKind::RedFlag:
        queries.emplace_back("SELECT 1 FROM red_flags WHERE flag_code = ? LIMIT 1;");
        break;
    case core::TaxonomyKind::Instruction:

        break;
    }

    for (const auto &sql : queries) {
        const Statement statement{database_, sql};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }
        statement.bindText(1, code);
        if (sqlite3_bind_parameter_count(statement.handle()) > 1) {
            statement.bindText(2, code);
        }
        const auto stepped = sqlite3_step(statement.handle());
        if (stepped == SQLITE_ROW) {
            used = true;
            return StoreStatus::Ok;
        }
        if (stepped != SQLITE_DONE) {
            return remember(stepped);
        }
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::removeEntry(const std::string &id, bool &retiredInstead)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }
    retiredInstead = false;

    core::TaxonomyEntry found;
    {
        const auto sql =
            std::string{"SELECT "} + kTaxonomyColumns + " FROM taxonomy_entries WHERE id = ?;";
        const Statement statement{database_, sql};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }
        statement.bindText(1, id);
        const auto stepped = sqlite3_step(statement.handle());
        if (stepped == SQLITE_DONE) {
            return StoreStatus::Missing;
        }
        if (stepped != SQLITE_ROW) {
            return remember(stepped);
        }
        found = taxonomyFrom(statement.handle());
    }

    bool used = false;
    if (const auto status = taxonomyIsUsed(found.kind, found.code, used);
        status != StoreStatus::Ok) {
        return status;
    }

    if (used) {
        retiredInstead = true;
        return retireEntry(id, true);
    }

    const Statement statement{database_, "DELETE FROM taxonomy_entries WHERE id = ?;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, id);
    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::usedCodes(core::TaxonomyKind kind, std::vector<std::string> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const char *sql = nullptr;
    switch (kind) {
    case core::TaxonomyKind::Category:
        sql = "SELECT DISTINCT category_code FROM recordings "
              "WHERE category_code <> '' AND deleted_at = 0 ORDER BY category_code;";
        break;
    case core::TaxonomyKind::Tag:
        sql = "SELECT DISTINCT tag_code FROM recording_tags ORDER BY tag_code;";
        break;
    case core::TaxonomyKind::RedFlag:
        sql = "SELECT DISTINCT flag_code FROM red_flags WHERE dismissed = 0 ORDER BY flag_code;";
        break;
    case core::TaxonomyKind::Instruction:

        out.clear();
        return StoreStatus::Ok;
    }

    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    for (;;) {
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            break;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        out.push_back(textAt(statement.handle(), 0));
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::tags(const std::string &recordingId,
                                    std::vector<core::RecordingTag> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, R"sql(
        SELECT tag_code, source, artifact_id FROM recording_tags
        WHERE recording_id = ? ORDER BY tag_code;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, recordingId);

    for (;;) {
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            break;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        core::RecordingTag tag;
        tag.code = textAt(statement.handle(), 0);
        tag.source = core::parseTagSource(textAt(statement.handle(), 1)).value_or(tag.source);
        tag.artifactId = textAt(statement.handle(), 2);
        out.push_back(std::move(tag));
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::setTag(const std::string &recordingId, const core::RecordingTag &tag)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, R"sql(
        INSERT INTO recording_tags (recording_id, tag_code, source, artifact_id, created_at)
        VALUES (?, ?, ?, ?, ?)
        ON CONFLICT(recording_id, tag_code) DO UPDATE SET
            source = CASE WHEN recording_tags.source = 'manual' THEN 'manual'
                          ELSE excluded.source END,
            artifact_id = CASE WHEN recording_tags.source = 'manual'
                               THEN recording_tags.artifact_id ELSE excluded.artifact_id END;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    const auto sourceText = std::string{core::toString(tag.source)};
    statement.bindText(1, recordingId);
    statement.bindText(2, tag.code);
    statement.bindText(3, sourceText);
    statement.bindText(4, tag.artifactId);
    statement.bindInt(5, secondsOf(std::chrono::system_clock::now()));

    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::clearTag(const std::string &recordingId, const std::string &code)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{
        database_, "DELETE FROM recording_tags WHERE recording_id = ? AND tag_code = ?;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, recordingId);
    statement.bindText(2, code);
    if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
        return remember(stepped);
    }
    return sqlite3_changes(database_) == 0 ? StoreStatus::Missing : StoreStatus::Ok;
}

core::StoreStatus SqliteStore::clearTagsFrom(const std::string &recordingId,
                                             const std::string &templateCode)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (!hasTable(database_, "artifacts")) {
        return StoreStatus::Ok;
    }

    const Statement statement{database_, R"sql(
        DELETE FROM recording_tags
         WHERE recording_id = ?
           AND source = 'llm'
           AND artifact_id <> ''
           AND artifact_id IN (SELECT id FROM artifacts
                                WHERE recording_id = ? AND template_code = ?);
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, recordingId);
    statement.bindText(2, recordingId);
    statement.bindText(3, templateCode);
    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::redFlags(const std::string &recordingId,
                                        std::vector<core::RedFlagHit> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, R"sql(
        SELECT id, recording_id, artifact_id, flag_code, severity, evidence, at_ms, dismissed,
               created_at
        FROM red_flags WHERE recording_id = ? ORDER BY at_ms, flag_code;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, recordingId);

    for (;;) {
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            break;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        core::RedFlagHit hit;
        hit.id = textAt(statement.handle(), 0);
        hit.recordingId = textAt(statement.handle(), 1);
        hit.artifactId = textAt(statement.handle(), 2);
        hit.code = textAt(statement.handle(), 3);
        hit.severity = core::parseSeverity(textAt(statement.handle(), 4)).value_or(hit.severity);
        hit.evidence = textAt(statement.handle(), 5);
        if (const auto at = sqlite3_column_int64(statement.handle(), 6); at >= 0) {
            hit.atMs = at;
        }
        hit.dismissed = sqlite3_column_int(statement.handle(), 7) != 0;
        hit.createdAt = timeAt(statement.handle(), 8);
        out.push_back(std::move(hit));
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::saveRedFlag(core::RedFlagHit &hit)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (hit.id.empty()) {
        hit.id = core::makeUuid();
    }
    if (hit.createdAt.time_since_epoch().count() == 0) {
        hit.createdAt = std::chrono::system_clock::now();
    }

    const Statement statement{database_, R"sql(
        INSERT INTO red_flags
            (id, recording_id, artifact_id, flag_code, severity, evidence, at_ms, dismissed,
             created_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(id) DO UPDATE SET
            artifact_id = excluded.artifact_id, flag_code = excluded.flag_code,
            severity = excluded.severity, evidence = excluded.evidence, at_ms = excluded.at_ms,
            dismissed = excluded.dismissed;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    const auto severityText = std::string{core::toString(hit.severity)};
    statement.bindText(1, hit.id);
    statement.bindText(2, hit.recordingId);
    statement.bindText(3, hit.artifactId);
    statement.bindText(4, hit.code);
    statement.bindText(5, severityText);
    statement.bindText(6, hit.evidence);
    statement.bindInt(7, hit.atMs.value_or(-1));
    statement.bindInt(8, hit.dismissed ? 1 : 0);
    statement.bindInt(9, secondsOf(hit.createdAt));

    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return countRedFlagsOn(hit.recordingId);
}

core::StoreStatus SqliteStore::dismissRedFlag(const std::string &id, bool dismissed)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    std::string recordingId;
    {
        const Statement statement{database_, "SELECT recording_id FROM red_flags WHERE id = ?;"};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }
        statement.bindText(1, id);
        const auto stepped = sqlite3_step(statement.handle());
        if (stepped == SQLITE_DONE) {
            return StoreStatus::Missing;
        }
        if (stepped != SQLITE_ROW) {
            return remember(stepped);
        }
        recordingId = textAt(statement.handle(), 0);
    }

    {
        const Statement statement{database_, "UPDATE red_flags SET dismissed = ? WHERE id = ?;"};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }
        statement.bindInt(1, dismissed ? 1 : 0);
        statement.bindText(2, id);
        if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
            return remember(code);
        }
    }
    return countRedFlagsOn(recordingId);
}

core::StoreStatus SqliteStore::clearRedFlagsFrom(const std::string &recordingId,
                                                 const std::string &templateCode)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (!hasTable(database_, "artifacts")) {
        return StoreStatus::Ok;
    }

    {

        const Statement statement{database_, R"sql(
            DELETE FROM red_flags
             WHERE recording_id = ?
               AND dismissed = 0
               AND artifact_id <> ''
               AND artifact_id IN (SELECT id FROM artifacts
                                    WHERE recording_id = ? AND template_code = ?);
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return remember(statement.prepareCode());
        }
        statement.bindText(1, recordingId);
        statement.bindText(2, recordingId);
        statement.bindText(3, templateCode);
        if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
            return remember(code);
        }
    }

    return countRedFlagsOn(recordingId);
}

core::StoreStatus SqliteStore::countRedFlagsOn(const std::string &recordingId)
{

    const Statement statement{database_, R"sql(
        UPDATE recordings SET red_flag_count =
            (SELECT COUNT(*) FROM red_flags WHERE recording_id = ? AND dismissed = 0)
        WHERE id = ?;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, recordingId);
    statement.bindText(2, recordingId);
    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::recordUsage(core::UsageEntry &entry)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (entry.id.empty()) {
        entry.id = core::makeUuid();
    }
    if (entry.at.time_since_epoch().count() == 0) {
        entry.at = std::chrono::system_clock::now();
    }

    const auto kindText = std::string{core::toString(entry.kind)};

    const Statement statement{database_, R"sql(
        INSERT INTO usage_ledger
            (id, at, provider_id, provider_type, model, kind, job_id, recording_id,
             prompt_tokens, answer_tokens, cost_micros, currency)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, entry.id);
    statement.bindInt(2, secondsOf(entry.at));
    statement.bindText(3, entry.providerId);
    statement.bindText(4, entry.providerType);
    statement.bindText(5, entry.model);
    statement.bindText(6, kindText);
    statement.bindText(7, entry.jobId);
    statement.bindText(8, entry.recordingId);
    statement.bindInt(9, entry.promptTokens);
    statement.bindInt(10, entry.answerTokens);
    statement.bindInt(11, entry.costMicros);
    statement.bindText(12, entry.currency);

    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return StoreStatus::Ok;
}

namespace {

constexpr const char *kUsageColumns =
    "COUNT(*), COALESCE(SUM(prompt_tokens), 0), COALESCE(SUM(answer_tokens), 0), "
    "COALESCE(SUM(cost_micros), 0), COUNT(DISTINCT CASE WHEN currency <> '' THEN currency END), "
    "COALESCE(MAX(currency), '')";

[[nodiscard]] core::UsageTotals totalsFrom(sqlite3_stmt *statement)
{
    core::UsageTotals totals;
    totals.calls = sqlite3_column_int(statement, 0);
    totals.promptTokens = sqlite3_column_int64(statement, 1);
    totals.answerTokens = sqlite3_column_int64(statement, 2);
    totals.costMicros = sqlite3_column_int64(statement, 3);
    if (sqlite3_column_int(statement, 4) == 1) {
        totals.currency = textAt(statement, 5);
    }
    return totals;
}

}

core::StoreStatus SqliteStore::usageSince(std::chrono::system_clock::time_point from,
                                          core::UsageTotals &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto sql = std::string{"SELECT "} + kUsageColumns + " FROM usage_ledger WHERE at >= ?;";
    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindInt(1, secondsOf(from));

    const auto code = sqlite3_step(statement.handle());
    if (code != SQLITE_ROW) {
        return remember(code);
    }
    out = totalsFrom(statement.handle());
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::usageFor(const std::string &recordingId, core::UsageTotals &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto sql =
        std::string{"SELECT "} + kUsageColumns + " FROM usage_ledger WHERE recording_id = ?;";
    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, recordingId);

    const auto code = sqlite3_step(statement.handle());
    if (code != SQLITE_ROW) {
        return remember(code);
    }
    out = totalsFrom(statement.handle());
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::forgetUsageBefore(std::chrono::system_clock::time_point when)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, "DELETE FROM usage_ledger WHERE at < ?;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindInt(1, secondsOf(when));
    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return StoreStatus::Ok;
}

namespace {

constexpr auto kTemplateColumns =
    "id, code, name, kind, shape, system, body, language_policy, language, params, provider_id, "
    "built_in, enabled, version, created_at, updated_at, instructions, realtime_mode, "
    "realtime_near, asr_provider_id, turn_policy, position";

[[nodiscard]] core::PromptTemplate templateFrom(sqlite3_stmt *statement)
{
    core::PromptTemplate prompt;
    prompt.id = textAt(statement, 0);
    prompt.code = textAt(statement, 1);
    prompt.name = textAt(statement, 2);
    prompt.kind = core::parseArtifactKind(textAt(statement, 3)).value_or(prompt.kind);
    prompt.shape = core::parseAnswerShape(textAt(statement, 4)).value_or(prompt.shape);
    prompt.system = textAt(statement, 5);
    prompt.body = textAt(statement, 6);
    prompt.languagePolicy =
        core::parseLanguagePolicy(textAt(statement, 7)).value_or(prompt.languagePolicy);
    prompt.language = textAt(statement, 8);
    prompt.params = core::decodeParams(textAt(statement, 9));
    prompt.providerId = textAt(statement, 10);
    prompt.builtIn = sqlite3_column_int(statement, 11) != 0;
    prompt.enabled = sqlite3_column_int(statement, 12) != 0;
    prompt.version = sqlite3_column_int(statement, 13);
    prompt.createdAt = timeAt(statement, 14);
    prompt.updatedAt = timeAt(statement, 15);
    prompt.extraInstructions = core::splitCodes(textAt(statement, 16));
    prompt.realtimeMode =
        core::parseRealtimeMode(textAt(statement, 17)).value_or(prompt.realtimeMode);
    prompt.realtimeNear = sqlite3_column_int(statement, 18) != 0;
    prompt.asrProviderId = textAt(statement, 19);
    prompt.turnPolicy = core::parseTurnPolicy(textAt(statement, 20)).value_or(prompt.turnPolicy);
    prompt.position = sqlite3_column_int(statement, 21);
    return prompt;
}

constexpr auto kArtifactColumns =
    "id, recording_id, transcript_id, template_id, template_code, template_name, "
    "template_version, provider_id, provider_type, model, kind, shape, status, content, "
    "structured, raw, language, parts, requested_at, completed_at, error, prompt_tokens, "
    "answer_tokens, cost_micros, currency, supersedes, superseded, question, "
    "turn_from, turn_to";

[[nodiscard]] core::Artifact artifactFrom(sqlite3_stmt *statement)
{
    core::Artifact artifact;
    artifact.id = textAt(statement, 0);
    artifact.recordingId = textAt(statement, 1);
    artifact.transcriptId = textAt(statement, 2);
    artifact.templateId = textAt(statement, 3);
    artifact.templateCode = textAt(statement, 4);
    artifact.templateName = textAt(statement, 5);
    artifact.templateVersion = sqlite3_column_int(statement, 6);
    artifact.providerId = textAt(statement, 7);
    artifact.providerType = textAt(statement, 8);
    artifact.model = textAt(statement, 9);
    artifact.kind = core::parseArtifactKind(textAt(statement, 10)).value_or(artifact.kind);
    artifact.shape = core::parseAnswerShape(textAt(statement, 11)).value_or(artifact.shape);
    artifact.status = core::parseArtifactStatus(textAt(statement, 12)).value_or(artifact.status);
    artifact.content = textAt(statement, 13);
    artifact.structured = textAt(statement, 14);
    artifact.raw = textAt(statement, 15);
    artifact.language = textAt(statement, 16);
    artifact.parts = sqlite3_column_int(statement, 17);
    artifact.requestedAt = timeAt(statement, 18);
    artifact.completedAt = timeAt(statement, 19);
    artifact.error = textAt(statement, 20);
    artifact.promptTokens = sqlite3_column_int(statement, 21);
    artifact.answerTokens = sqlite3_column_int(statement, 22);
    artifact.costMicros = sqlite3_column_int64(statement, 23);
    artifact.currency = textAt(statement, 24);
    artifact.supersedes = textAt(statement, 25);
    artifact.superseded = sqlite3_column_int(statement, 26) != 0;
    artifact.question = textAt(statement, 27);
    artifact.turnFrom = sqlite3_column_int(statement, 28);
    artifact.turnTo = sqlite3_column_int(statement, 29);
    return artifact;
}

}

core::StoreStatus SqliteStore::fillEmptyShippedPrompts()
{

    const Statement statement{database_, R"sql(
        UPDATE prompt_templates
           SET system = ?, body = ?, updated_at = ?
         WHERE code = ? AND built_in = 1 AND body = '';
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    const auto now = secondsOf(std::chrono::system_clock::now());
    int filled = 0;
    for (const auto &prompt : core::shippedTemplates(contentLanguage_)) {
        if (prompt.body.empty()) {
            continue;
        }

        sqlite3_reset(statement.handle());

        statement.bindText(1, prompt.system);
        statement.bindText(2, prompt.body);
        statement.bindInt(3, now);
        statement.bindText(4, prompt.code);
        if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
            return remember(code);
        }
        filled += sqlite3_changes(database_);
    }

    if (filled > 0) {
        core::Logger::instance().log(core::LogLevel::Info, "storage",
                                     "filled in " + std::to_string(filled) +
                                         " shipped prompt(s) that had been seeded with no words");
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::installTemplates(std::vector<std::string> &written)
{

    std::set<std::string> seen;
    if (const auto status = alreadySeeded("template", seen); status != StoreStatus::Ok) {
        return status;
    }

    const Statement statement{database_, R"sql(
        INSERT INTO prompt_templates
            (id, code, name, kind, shape, system, body, language_policy, language, params,
             provider_id, built_in, enabled, version, created_at, updated_at, instructions,
             realtime_mode, realtime_near, asr_provider_id, turn_policy, position)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, '', 1, ?, 1, ?, ?, ?, ?, ?, '', ?, ?);
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    const auto now = secondsOf(std::chrono::system_clock::now());
    for (const auto &prompt : core::shippedTemplates(contentLanguage_)) {
        if (seen.count(prompt.code) != 0) {
            continue;
        }

        const auto id = core::makeUuid();
        const auto kind = std::string{core::toString(prompt.kind)};
        const auto shape = std::string{core::toString(prompt.shape)};
        const auto policy = std::string{core::toString(prompt.languagePolicy)};
        const auto params = core::encodeParams(prompt.params);
        const auto instructions = core::joinCodes(prompt.extraInstructions);

        sqlite3_reset(statement.handle());
        statement.bindText(1, id);
        statement.bindText(2, prompt.code);
        statement.bindText(3, prompt.name);
        statement.bindText(4, kind);
        statement.bindText(5, shape);
        statement.bindText(6, prompt.system);
        statement.bindText(7, prompt.body);
        statement.bindText(8, policy);
        statement.bindText(9, prompt.language);
        statement.bindText(10, params);

        statement.bindInt(11, prompt.enabled ? 1 : 0);
        statement.bindInt(12, now);
        statement.bindInt(13, now);
        statement.bindText(14, instructions);

        const auto mode = std::string{core::toString(prompt.realtimeMode)};
        const auto turns = std::string{core::toString(prompt.turnPolicy)};
        statement.bindText(15, mode);
        statement.bindInt(16, prompt.realtimeNear ? 1 : 0);
        statement.bindText(17, turns);

        statement.bindInt(18, prompt.position);
        if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
            return remember(code);
        }
        written.push_back(prompt.code);
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::save(core::PromptTemplate &prompt)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (prompt.id.empty()) {
        prompt.id = core::makeUuid();
    }
    const auto now = std::chrono::system_clock::now();
    if (prompt.createdAt.time_since_epoch().count() == 0) {
        prompt.createdAt = now;
    }
    prompt.updatedAt = now;

    prompt.version = std::max(1, prompt.version) + 1;

    const auto kind = std::string{core::toString(prompt.kind)};
    const auto shape = std::string{core::toString(prompt.shape)};
    const auto policy = std::string{core::toString(prompt.languagePolicy)};
    const auto params = core::encodeParams(prompt.params);
    const auto instructions = core::joinCodes(prompt.extraInstructions);

    const Statement statement{database_, R"sql(
        INSERT INTO prompt_templates
            (id, code, name, kind, shape, system, body, language_policy, language, params,
             provider_id, built_in, enabled, version, created_at, updated_at, instructions,
             realtime_mode, realtime_near, asr_provider_id, turn_policy, position)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(id) DO UPDATE SET
            code = excluded.code, name = excluded.name, kind = excluded.kind,
            shape = excluded.shape, system = excluded.system, body = excluded.body,
            language_policy = excluded.language_policy, language = excluded.language,
            params = excluded.params, provider_id = excluded.provider_id,
            built_in = excluded.built_in, enabled = excluded.enabled,
            version = excluded.version, updated_at = excluded.updated_at,
            instructions = excluded.instructions, realtime_mode = excluded.realtime_mode,
            realtime_near = excluded.realtime_near,
            asr_provider_id = excluded.asr_provider_id, turn_policy = excluded.turn_policy,
            position = excluded.position;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, prompt.id);
    statement.bindText(2, prompt.code);
    statement.bindText(3, prompt.name);
    statement.bindText(4, kind);
    statement.bindText(5, shape);
    statement.bindText(6, prompt.system);
    statement.bindText(7, prompt.body);
    statement.bindText(8, policy);
    statement.bindText(9, prompt.language);
    statement.bindText(10, params);
    statement.bindText(11, prompt.providerId);
    statement.bindInt(12, prompt.builtIn ? 1 : 0);
    statement.bindInt(13, prompt.enabled ? 1 : 0);
    statement.bindInt(14, prompt.version);
    statement.bindInt(15, secondsOf(prompt.createdAt));
    statement.bindInt(16, secondsOf(prompt.updatedAt));
    statement.bindText(17, instructions);

    const auto mode = std::string{core::toString(prompt.realtimeMode)};
    const auto turns = std::string{core::toString(prompt.turnPolicy)};
    statement.bindText(18, mode);
    statement.bindInt(19, prompt.realtimeNear ? 1 : 0);
    statement.bindText(20, prompt.asrProviderId);
    statement.bindText(21, turns);
    statement.bindInt(22, prompt.position);

    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return StoreStatus::Ok;
}

core::StoreStatus
SqliteStore::orderTemplates(const std::vector<std::pair<std::string, int>> &places)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, "UPDATE prompt_templates SET position = ? WHERE id = ?;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    for (const auto &[id, place] : places) {
        sqlite3_reset(statement.handle());
        statement.bindInt(1, place);
        statement.bindText(2, id);
        if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
            return remember(code);
        }
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::templates(std::vector<core::PromptTemplate> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto sql = std::string{"SELECT "} + kTemplateColumns +
                     " FROM prompt_templates ORDER BY position, name, id;";
    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    for (;;) {
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            break;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        out.push_back(templateFrom(statement.handle()));
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::prompt(const std::string &id, core::PromptTemplate &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto sql =
        std::string{"SELECT "} + kTemplateColumns + " FROM prompt_templates WHERE id = ?;";
    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, id);

    const auto code = sqlite3_step(statement.handle());
    if (code == SQLITE_DONE) {
        return StoreStatus::Missing;
    }
    if (code != SQLITE_ROW) {
        return remember(code);
    }
    out = templateFrom(statement.handle());
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::restoreShippedTemplates()
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (const auto status = execute("BEGIN IMMEDIATE;"); status != StoreStatus::Ok) {
        return status;
    }

    if (const auto status = execute("DELETE FROM prompt_templates;"
                                    "DELETE FROM seeded WHERE kind = 'template';");
        status != StoreStatus::Ok) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    }

    std::vector<std::string> written;
    if (const auto status = installTemplates(written); status != StoreStatus::Ok) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    }
    if (const auto status = rememberSeeded("template", written); status != StoreStatus::Ok) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    }
    return execute("COMMIT;");
}

core::StoreStatus SqliteStore::removeTemplate(const std::string &id)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, "DELETE FROM prompt_templates WHERE id = ?;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, id);
    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return sqlite3_changes(database_) == 0 ? StoreStatus::Missing : StoreStatus::Ok;
}

core::StoreStatus SqliteStore::save(core::Artifact &artifact)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (artifact.id.empty()) {
        artifact.id = core::makeUuid();
    }
    if (artifact.requestedAt.time_since_epoch().count() == 0) {
        artifact.requestedAt = std::chrono::system_clock::now();
    }

    const auto kind = std::string{core::toString(artifact.kind)};
    const auto shape = std::string{core::toString(artifact.shape)};
    const auto status = std::string{core::toString(artifact.status)};

    if (const auto began = execute("BEGIN IMMEDIATE;"); began != StoreStatus::Ok) {
        return began;
    }
    const auto give = [this](StoreStatus failed) {
        static_cast<void>(execute("ROLLBACK;"));
        return failed;
    };

    {
        const Statement statement{database_, R"sql(
            INSERT INTO artifacts
                (id, recording_id, transcript_id, template_id, template_code, template_name,
                 template_version, provider_id, provider_type, model, kind, shape, status,
                 content, structured, raw, language, parts, requested_at, completed_at, error,
                 prompt_tokens, answer_tokens, cost_micros, currency, supersedes, superseded,
                 question, turn_from, turn_to)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,
                    ?, ?, ?, ?, ?)
            ON CONFLICT(id) DO UPDATE SET
                transcript_id = excluded.transcript_id, template_id = excluded.template_id,
                template_code = excluded.template_code, template_name = excluded.template_name,
                template_version = excluded.template_version,
                provider_id = excluded.provider_id, provider_type = excluded.provider_type,
                model = excluded.model, kind = excluded.kind, shape = excluded.shape,
                status = excluded.status, content = excluded.content,
                structured = excluded.structured, raw = excluded.raw,
                language = excluded.language, parts = excluded.parts,
                completed_at = excluded.completed_at, error = excluded.error,
                prompt_tokens = excluded.prompt_tokens, answer_tokens = excluded.answer_tokens,
                cost_micros = excluded.cost_micros, currency = excluded.currency,
                supersedes = excluded.supersedes, superseded = excluded.superseded,
                question = excluded.question, turn_from = excluded.turn_from,
                turn_to = excluded.turn_to;
        )sql"};
        if (statement.prepareCode() != SQLITE_OK) {
            return give(remember(statement.prepareCode()));
        }
        statement.bindText(1, artifact.id);
        statement.bindText(2, artifact.recordingId);
        statement.bindText(3, artifact.transcriptId);
        statement.bindText(4, artifact.templateId);
        statement.bindText(5, artifact.templateCode);
        statement.bindText(6, artifact.templateName);
        statement.bindInt(7, artifact.templateVersion);
        statement.bindText(8, artifact.providerId);
        statement.bindText(9, artifact.providerType);
        statement.bindText(10, artifact.model);
        statement.bindText(11, kind);
        statement.bindText(12, shape);
        statement.bindText(13, status);
        statement.bindText(14, artifact.content);
        statement.bindText(15, artifact.structured);
        statement.bindText(16, artifact.raw);
        statement.bindText(17, artifact.language);
        statement.bindInt(18, std::max(1, artifact.parts));
        statement.bindInt(19, secondsOf(artifact.requestedAt));
        statement.bindInt(20, secondsOf(artifact.completedAt));
        statement.bindText(21, artifact.error);
        statement.bindInt(22, artifact.promptTokens);
        statement.bindInt(23, artifact.answerTokens);
        statement.bindInt(24, artifact.costMicros);
        statement.bindText(25, artifact.currency);
        statement.bindText(26, artifact.supersedes);
        statement.bindInt(27, artifact.superseded ? 1 : 0);
        statement.bindText(28, artifact.question);
        statement.bindInt(29, artifact.turnFrom);
        statement.bindInt(30, artifact.turnTo);

        if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
            return give(remember(code));
        }
    }

    if (!artifact.supersedes.empty()) {
        const Statement statement{database_, "UPDATE artifacts SET superseded = 1 WHERE id = ?;"};
        if (statement.prepareCode() != SQLITE_OK) {
            return give(remember(statement.prepareCode()));
        }
        statement.bindText(1, artifact.supersedes);
        if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
            return give(remember(code));
        }
    }

    return execute("COMMIT;");
}

core::StoreStatus SqliteStore::artifacts(const core::ArtifactQuery &query,
                                         std::vector<core::Artifact> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    auto sql = std::string{"SELECT "} + kArtifactColumns + " FROM artifacts WHERE 1 = 1";
    if (!query.recordingId.empty()) {
        sql += " AND recording_id = ?";
    }
    if (query.kind.has_value()) {
        sql += " AND kind = ?";
    }
    if (query.currentOnly) {
        sql += " AND superseded = 0";
    }
    sql += " ORDER BY requested_at DESC, id;";

    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    int at = 1;
    if (!query.recordingId.empty()) {
        statement.bindText(at++, query.recordingId);
    }
    const auto kind =
        query.kind.has_value() ? std::string{core::toString(*query.kind)} : std::string{};
    if (query.kind.has_value()) {
        statement.bindText(at++, kind);
    }

    for (;;) {
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            break;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        out.push_back(artifactFrom(statement.handle()));
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::artifact(const std::string &id, core::Artifact &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto sql = std::string{"SELECT "} + kArtifactColumns + " FROM artifacts WHERE id = ?;";
    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, id);

    const auto code = sqlite3_step(statement.handle());
    if (code == SQLITE_DONE) {
        return StoreStatus::Missing;
    }
    if (code != SQLITE_ROW) {
        return remember(code);
    }
    out = artifactFrom(statement.handle());
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::removeArtifact(const std::string &id)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, "DELETE FROM artifacts WHERE id = ?;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, id);
    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return sqlite3_changes(database_) == 0 ? StoreStatus::Missing : StoreStatus::Ok;
}

core::StoreStatus SqliteStore::suggestCode(core::CodeSuggestion &suggestion)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (suggestion.id.empty()) {
        suggestion.id = core::makeUuid();
    }
    if (suggestion.lastAt.time_since_epoch().count() == 0) {
        suggestion.lastAt = std::chrono::system_clock::now();
    }
    const auto kind = std::string{core::toString(suggestion.kind)};

    const Statement statement{database_, R"sql(
        INSERT INTO code_suggestions (id, kind, code, recording_id, artifact_id, seen, last_at)
        VALUES (?, ?, ?, ?, ?, 1, ?)
        ON CONFLICT(kind, code) DO UPDATE SET
            seen = seen + 1, recording_id = excluded.recording_id,
            artifact_id = excluded.artifact_id, last_at = excluded.last_at;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, suggestion.id);
    statement.bindText(2, kind);
    statement.bindText(3, suggestion.code);
    statement.bindText(4, suggestion.recordingId);
    statement.bindText(5, suggestion.artifactId);
    statement.bindInt(6, secondsOf(suggestion.lastAt));

    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::suggestions(core::TaxonomyKind kind,
                                           std::vector<core::CodeSuggestion> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, R"sql(
        SELECT id, kind, code, recording_id, artifact_id, seen, last_at
        FROM code_suggestions WHERE kind = ? ORDER BY seen DESC, code;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    const auto kindText = std::string{core::toString(kind)};
    statement.bindText(1, kindText);

    for (;;) {
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            break;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        core::CodeSuggestion one;
        one.id = textAt(statement.handle(), 0);
        one.kind = core::parseTaxonomyKind(textAt(statement.handle(), 1)).value_or(one.kind);
        one.code = textAt(statement.handle(), 2);
        one.recordingId = textAt(statement.handle(), 3);
        one.artifactId = textAt(statement.handle(), 4);
        one.seen = sqlite3_column_int(statement.handle(), 5);
        one.lastAt = timeAt(statement.handle(), 6);
        out.push_back(std::move(one));
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::forgetSuggestion(const std::string &code, core::TaxonomyKind kind)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_,
                              "DELETE FROM code_suggestions WHERE kind = ? AND code = ?;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    const auto kindText = std::string{core::toString(kind)};
    statement.bindText(1, kindText);
    statement.bindText(2, code);
    if (const auto stepped = sqlite3_step(statement.handle()); stepped != SQLITE_DONE) {
        return remember(stepped);
    }
    return StoreStatus::Ok;
}

namespace {

constexpr auto kAutomationColumns = "id, code, name, enabled, ordinal, conditions, action, "
                                    "template_code, tag_code, built_in, created_at, updated_at";

[[nodiscard]] core::Automation automationFrom(sqlite3_stmt *statement)
{
    core::Automation one;
    one.id = textAt(statement, 0);
    one.code = textAt(statement, 1);
    one.name = textAt(statement, 2);
    one.enabled = sqlite3_column_int(statement, 3) != 0;
    one.ordinal = sqlite3_column_int(statement, 4);
    one.conditions = core::decodeConditions(textAt(statement, 5));
    one.action = core::parseAction(textAt(statement, 6)).value_or(one.action);
    one.templateCode = textAt(statement, 7);
    one.tagCode = textAt(statement, 8);
    one.builtIn = sqlite3_column_int(statement, 9) != 0;
    one.createdAt = timeAt(statement, 10);
    one.updatedAt = timeAt(statement, 11);
    return one;
}

}

core::StoreStatus SqliteStore::installAutomations(std::vector<std::string> &written)
{

    std::set<std::string> seen;
    if (const auto status = alreadySeeded("automation", seen); status != StoreStatus::Ok) {
        return status;
    }

    const Statement statement{database_, R"sql(
        INSERT INTO automations
            (id, code, name, enabled, ordinal, conditions, action,
             template_code, tag_code, built_in, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, '', 1, ?, ?);
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    const auto now = secondsOf(std::chrono::system_clock::now());
    for (const auto &one : core::shippedAutomations(contentLanguage_)) {
        if (seen.count(one.code) != 0) {
            continue;
        }

        const auto id = core::makeUuid();
        const auto conditions = core::encodeConditions(one.conditions);
        const auto action = std::string{core::toString(one.action)};

        sqlite3_reset(statement.handle());
        statement.bindText(1, id);
        statement.bindText(2, one.code);
        statement.bindText(3, one.name);
        statement.bindInt(4, one.enabled ? 1 : 0);
        statement.bindInt(5, one.ordinal);
        statement.bindText(6, conditions);
        statement.bindText(7, action);
        statement.bindText(8, one.templateCode);
        statement.bindInt(9, now);
        statement.bindInt(10, now);
        if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
            return remember(code);
        }
        written.push_back(one.code);
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::save(core::Automation &automation)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (automation.id.empty()) {
        automation.id = core::makeUuid();
    }
    const auto now = std::chrono::system_clock::now();
    if (automation.createdAt.time_since_epoch().count() == 0) {
        automation.createdAt = now;
    }
    automation.updatedAt = now;

    const auto conditions = core::encodeConditions(automation.conditions);
    const auto action = std::string{core::toString(automation.action)};

    const Statement statement{database_, R"sql(
        INSERT INTO automations
            (id, code, name, enabled, ordinal, conditions, action,
             template_code, tag_code, built_in, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(id) DO UPDATE SET
            code = excluded.code, name = excluded.name, enabled = excluded.enabled,
            ordinal = excluded.ordinal,
            conditions = excluded.conditions, action = excluded.action,
            template_code = excluded.template_code,
            tag_code = excluded.tag_code, built_in = excluded.built_in,
            updated_at = excluded.updated_at;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, automation.id);
    statement.bindText(2, automation.code);
    statement.bindText(3, automation.name);
    statement.bindInt(4, automation.enabled ? 1 : 0);
    statement.bindInt(5, automation.ordinal);
    statement.bindText(6, conditions);
    statement.bindText(7, action);
    statement.bindText(8, automation.templateCode);
    statement.bindText(9, automation.tagCode);
    statement.bindInt(10, automation.builtIn ? 1 : 0);
    statement.bindInt(11, secondsOf(automation.createdAt));
    statement.bindInt(12, secondsOf(automation.updatedAt));

    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::automations(std::vector<core::Automation> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto sql = std::string{"SELECT "} + kAutomationColumns +
                     " FROM automations ORDER BY ordinal, code, id;";
    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }

    for (;;) {
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            break;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        out.push_back(automationFrom(statement.handle()));
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::automation(const std::string &id, core::Automation &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const auto sql =
        std::string{"SELECT "} + kAutomationColumns + " FROM automations WHERE id = ?;";
    const Statement statement{database_, sql};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, id);

    const auto code = sqlite3_step(statement.handle());
    if (code == SQLITE_DONE) {
        return StoreStatus::Missing;
    }
    if (code != SQLITE_ROW) {
        return remember(code);
    }
    out = automationFrom(statement.handle());
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::restoreShippedAutomations()
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (const auto status = execute("BEGIN IMMEDIATE;"); status != StoreStatus::Ok) {
        return status;
    }

    if (const auto status = execute("DELETE FROM automation_runs;"
                                    "DELETE FROM automations;"
                                    "DELETE FROM seeded WHERE kind = 'automation';");
        status != StoreStatus::Ok) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    }

    std::vector<std::string> written;
    if (const auto status = installAutomations(written); status != StoreStatus::Ok) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    }
    if (const auto status = rememberSeeded("automation", written); status != StoreStatus::Ok) {
        static_cast<void>(execute("ROLLBACK;"));
        return status;
    }
    return execute("COMMIT;");
}

core::StoreStatus SqliteStore::removeAutomation(const std::string &id)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    if (const auto began = execute("BEGIN IMMEDIATE;"); began != StoreStatus::Ok) {
        return began;
    }
    const auto give = [this](StoreStatus failed) {
        static_cast<void>(execute("ROLLBACK;"));
        return failed;
    };

    for (const char *sql : {"DELETE FROM automation_runs WHERE automation_id = ?;",
                            "DELETE FROM automations WHERE id = ?;"}) {
        const Statement statement{database_, sql};
        if (statement.prepareCode() != SQLITE_OK) {
            return give(remember(statement.prepareCode()));
        }
        statement.bindText(1, id);
        if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
            return give(remember(code));
        }
    }

    return execute("COMMIT;");
}

core::StoreStatus SqliteStore::noteRun(const core::AutomationRun &run)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, R"sql(
        INSERT OR IGNORE INTO automation_runs (automation_id, recording_id, fired_at, job_id)
        VALUES (?, ?, ?, ?);
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, run.automationId);
    statement.bindText(2, run.recordingId);
    statement.bindInt(3, secondsOf(run.firedAt));
    statement.bindText(4, run.jobId);

    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }

    return sqlite3_changes(database_) == 0 ? StoreStatus::Missing : StoreStatus::Ok;
}

core::StoreStatus SqliteStore::hasRun(const std::string &automationId,
                                      const std::string &recordingId, bool &already)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }
    already = false;

    const Statement statement{database_, R"sql(
        SELECT 1 FROM automation_runs WHERE automation_id = ? AND recording_id = ?;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, automationId);
    statement.bindText(2, recordingId);

    const auto code = sqlite3_step(statement.handle());
    if (code == SQLITE_ROW) {
        already = true;
        return StoreStatus::Ok;
    }
    if (code != SQLITE_DONE) {
        return remember(code);
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::runsFor(const std::string &recordingId,
                                       std::vector<core::AutomationRun> &out)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, R"sql(
        SELECT automation_id, recording_id, fired_at, job_id
        FROM automation_runs WHERE recording_id = ? ORDER BY fired_at, automation_id;
    )sql"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, recordingId);

    for (;;) {
        const auto code = sqlite3_step(statement.handle());
        if (code == SQLITE_DONE) {
            break;
        }
        if (code != SQLITE_ROW) {
            return remember(code);
        }
        core::AutomationRun run;
        run.automationId = textAt(statement.handle(), 0);
        run.recordingId = textAt(statement.handle(), 1);
        run.firedAt = timeAt(statement.handle(), 2);
        run.jobId = textAt(statement.handle(), 3);
        out.push_back(std::move(run));
    }
    return StoreStatus::Ok;
}

core::StoreStatus SqliteStore::forgetRuns(const std::string &recordingId)
{
    const std::lock_guard<std::recursive_mutex> held{mutex_};
    if (database_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    const Statement statement{database_, "DELETE FROM automation_runs WHERE recording_id = ?;"};
    if (statement.prepareCode() != SQLITE_OK) {
        return remember(statement.prepareCode());
    }
    statement.bindText(1, recordingId);
    if (const auto code = sqlite3_step(statement.handle()); code != SQLITE_DONE) {
        return remember(code);
    }
    return StoreStatus::Ok;
}

}
