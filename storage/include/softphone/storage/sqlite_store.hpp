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

#include <softphone/core/artifacts.hpp>
#include <softphone/core/automations.hpp>
#include <softphone/core/call_log.hpp>
#include <softphone/core/contacts.hpp>
#include <softphone/core/jobs.hpp>
#include <softphone/core/providers.hpp>
#include <softphone/core/recordings.hpp>
#include <softphone/core/store.hpp>
#include <softphone/core/taxonomy.hpp>
#include <softphone/core/templates.hpp>
#include <softphone/core/transcripts.hpp>
#include <softphone/core/usage.hpp>

#include <filesystem>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <vector>

struct sqlite3;

namespace softphone::storage {

inline constexpr int kSchemaVersion = 34;

inline constexpr int kOldestUpgradableSchema = 3;

[[nodiscard]] std::filesystem::path defaultDatabaseFile();

[[nodiscard]] std::filesystem::path databaseFileIn(const std::filesystem::path &dataRoot);

class SqliteStore final : public core::IContactStore,
                          public core::ICallLog,
                          public core::IRecordingStore,
                          public core::IProviderStore,
                          public core::IJobStore,
                          public core::ITranscriptStore,
                          public core::ITaxonomyStore,
                          public core::IUsageStore,
                          public core::ITemplateStore,
                          public core::IArtifactStore,
                          public core::IAutomationStore {
public:
    SqliteStore();
    SqliteStore(const SqliteStore &) = delete;
    SqliteStore &operator=(const SqliteStore &) = delete;
    SqliteStore(SqliteStore &&) = delete;
    SqliteStore &operator=(SqliteStore &&) = delete;
    ~SqliteStore() override;

    [[nodiscard]] core::StoreStatus open(const std::filesystem::path &file);

    [[nodiscard]] core::StoreStatus declareCollections(const std::string &owner, int version,
                                                       const std::string &ddl);

    [[nodiscard]] core::StoreStatus installShippedTaxonomy();

    [[nodiscard]] core::StoreStatus installShippedPrompts();

    [[nodiscard]] core::StoreStatus recreate();

    [[nodiscard]] const std::filesystem::path &file() const noexcept
    {
        return file_;
    }

    void close();

    [[nodiscard]] bool isOpen() const;

    [[nodiscard]] int schemaVersion() const;

    [[nodiscard]] std::string lastError() const;

    void setContentLanguage(std::string language);

    [[nodiscard]] core::StoreStatus contacts(std::vector<core::Contact> &out) override;
    [[nodiscard]] core::StoreStatus save(core::Contact &contact) override;
    [[nodiscard]] core::StoreStatus removeContact(const std::string &id) override;

    [[nodiscard]] core::StoreStatus record(core::CallRecord &record) override;
    [[nodiscard]] core::StoreStatus calls(const core::CallLogQuery &query,
                                          std::vector<core::CallRecord> &out) override;
    [[nodiscard]] core::StoreStatus removeCall(const std::string &id) override;
    [[nodiscard]] core::StoreStatus clearCalls() override;
    [[nodiscard]] core::StoreStatus removeCallsBefore(std::chrono::system_clock::time_point before,
                                                      int &removed) override;

    [[nodiscard]] core::StoreStatus save(core::Recording &recording) override;
    [[nodiscard]] core::StoreStatus recordings(const core::RecordingQuery &query,
                                               std::vector<core::Recording> &out) override;
    [[nodiscard]] core::StoreStatus recording(const std::string &id, core::Recording &out) override;
    [[nodiscard]] core::StoreStatus removeRecording(const std::string &id,
                                                    std::vector<std::string> &files) override;
    [[nodiscard]] core::StoreStatus dropAudio(const std::string &id,
                                              std::vector<std::string> &files) override;
    [[nodiscard]] core::StoreStatus dropStems(const std::string &recordingId,
                                              std::vector<std::string> &files) override;
    [[nodiscard]] core::StoreStatus noteAccess(const std::string &recordingId,
                                               core::RecordingAccess what,
                                               const std::string &detail) override;
    [[nodiscard]] core::StoreStatus accessLog(const std::string &recordingId,
                                              std::vector<core::AccessEntry> &out) override;

    [[nodiscard]] core::StoreStatus save(core::ProviderConfig &provider) override;
    [[nodiscard]] core::StoreStatus providers(core::ProviderKind kind,
                                              std::vector<core::ProviderConfig> &out) override;
    [[nodiscard]] core::StoreStatus provider(const std::string &id,
                                             core::ProviderConfig &out) override;
    [[nodiscard]] core::StoreStatus defaultProvider(core::ProviderKind kind,
                                                    core::ProviderConfig &out) override;
    [[nodiscard]] core::StoreStatus removeProvider(const std::string &id) override;

    [[nodiscard]] core::StoreStatus save(core::Job &job) override;
    [[nodiscard]] core::StoreStatus job(const std::string &id, core::Job &out) override;
    [[nodiscard]] core::StoreStatus jobs(const core::JobQuery &query,
                                         std::vector<core::Job> &out) override;
    [[nodiscard]] core::StoreStatus claim(std::chrono::system_clock::time_point now,
                                          core::Job &out) override;
    [[nodiscard]] core::StoreStatus reviveRunning(std::size_t &revived) override;
    [[nodiscard]] core::StoreStatus cancelJob(const std::string &id) override;
    [[nodiscard]] core::StoreStatus forgetDone(std::chrono::system_clock::time_point before,
                                               std::size_t &removed) override;
    [[nodiscard]] core::StoreStatus forgetJob(const std::string &id) override;

    [[nodiscard]] core::StoreStatus save(core::Transcript &transcript) override;
    [[nodiscard]] core::StoreStatus transcripts(const std::string &recordingId,
                                                std::vector<core::Transcript> &out) override;
    [[nodiscard]] core::StoreStatus transcript(const std::string &id,
                                               core::Transcript &out) override;
    [[nodiscard]] core::StoreStatus primaryTranscript(const std::string &recordingId,
                                                      core::Transcript &out) override;
    [[nodiscard]] core::StoreStatus makePrimary(const std::string &id) override;
    [[nodiscard]] core::StoreStatus removeTranscript(const std::string &id,
                                                     std::vector<std::string> &files) override;
    [[nodiscard]] core::StoreStatus search(const core::TranscriptSearch &query,
                                           std::vector<core::TranscriptHit> &out) override;
    [[nodiscard]] core::StoreStatus
    transcriptLanguages(std::vector<std::string> &languages) override;

    [[nodiscard]] core::StoreStatus save(core::TaxonomyEntry &entry) override;
    [[nodiscard]] core::StoreStatus entries(core::TaxonomyKind kind, bool includeRetired,
                                            std::vector<core::TaxonomyEntry> &out) override;
    [[nodiscard]] core::StoreStatus entry(core::TaxonomyKind kind, const std::string &code,
                                          core::TaxonomyEntry &out) override;
    [[nodiscard]] core::StoreStatus retireEntry(const std::string &id, bool retired) override;

    [[nodiscard]] core::StoreStatus restoreShippedTaxonomy() override;
    [[nodiscard]] core::StoreStatus removeEntry(const std::string &id,
                                                bool &retiredInstead) override;
    [[nodiscard]] core::StoreStatus usedCodes(core::TaxonomyKind kind,
                                              std::vector<std::string> &out) override;

    [[nodiscard]] core::StoreStatus tags(const std::string &recordingId,
                                         std::vector<core::RecordingTag> &out) override;
    [[nodiscard]] core::StoreStatus setTag(const std::string &recordingId,
                                           const core::RecordingTag &tag) override;
    [[nodiscard]] core::StoreStatus clearTag(const std::string &recordingId,
                                             const std::string &code) override;
    [[nodiscard]] core::StoreStatus clearTagsFrom(const std::string &recordingId,
                                                  const std::string &templateCode) override;

    [[nodiscard]] core::StoreStatus redFlags(const std::string &recordingId,
                                             std::vector<core::RedFlagHit> &out) override;
    [[nodiscard]] core::StoreStatus saveRedFlag(core::RedFlagHit &hit) override;
    [[nodiscard]] core::StoreStatus dismissRedFlag(const std::string &id, bool dismissed) override;
    [[nodiscard]] core::StoreStatus clearRedFlagsFrom(const std::string &recordingId,
                                                      const std::string &templateCode) override;

    [[nodiscard]] core::StoreStatus recordUsage(core::UsageEntry &entry) override;
    [[nodiscard]] core::StoreStatus usageSince(std::chrono::system_clock::time_point from,
                                               core::UsageTotals &out) override;
    [[nodiscard]] core::StoreStatus usageFor(const std::string &recordingId,
                                             core::UsageTotals &out) override;
    [[nodiscard]] core::StoreStatus
    forgetUsageBefore(std::chrono::system_clock::time_point when) override;

    [[nodiscard]] core::StoreStatus save(core::PromptTemplate &prompt) override;
    [[nodiscard]] core::StoreStatus templates(std::vector<core::PromptTemplate> &out) override;
    [[nodiscard]] core::StoreStatus
    orderTemplates(const std::vector<std::pair<std::string, int>> &places) override;
    [[nodiscard]] core::StoreStatus prompt(const std::string &id,
                                           core::PromptTemplate &out) override;
    [[nodiscard]] core::StoreStatus removeTemplate(const std::string &id) override;
    [[nodiscard]] core::StoreStatus restoreShippedTemplates() override;

    [[nodiscard]] core::StoreStatus save(core::Artifact &artifact) override;
    [[nodiscard]] core::StoreStatus artifacts(const core::ArtifactQuery &query,
                                              std::vector<core::Artifact> &out) override;
    [[nodiscard]] core::StoreStatus artifact(const std::string &id, core::Artifact &out) override;
    [[nodiscard]] core::StoreStatus removeArtifact(const std::string &id) override;
    [[nodiscard]] core::StoreStatus suggestCode(core::CodeSuggestion &suggestion) override;
    [[nodiscard]] core::StoreStatus suggestions(core::TaxonomyKind kind,
                                                std::vector<core::CodeSuggestion> &out) override;
    [[nodiscard]] core::StoreStatus forgetSuggestion(const std::string &code,
                                                     core::TaxonomyKind kind) override;

    [[nodiscard]] core::StoreStatus save(core::Automation &automation) override;
    [[nodiscard]] core::StoreStatus automations(std::vector<core::Automation> &out) override;
    [[nodiscard]] core::StoreStatus automation(const std::string &id,
                                               core::Automation &out) override;
    [[nodiscard]] core::StoreStatus removeAutomation(const std::string &id) override;
    [[nodiscard]] core::StoreStatus restoreShippedAutomations() override;
    [[nodiscard]] core::StoreStatus noteRun(const core::AutomationRun &run) override;
    [[nodiscard]] core::StoreStatus hasRun(const std::string &automationId,
                                           const std::string &recordingId, bool &already) override;
    [[nodiscard]] core::StoreStatus runsFor(const std::string &recordingId,
                                            std::vector<core::AutomationRun> &out) override;
    [[nodiscard]] core::StoreStatus forgetRuns(const std::string &recordingId) override;

private:
    [[nodiscard]] core::StoreStatus migrate();

    [[nodiscard]] core::StoreStatus writeLegs(const core::Recording &recording);
    [[nodiscard]] core::StoreStatus writeStems(core::Recording &recording);
    [[nodiscard]] core::StoreStatus readChildren(core::Recording &recording);

    [[nodiscard]] core::StoreStatus readTags(std::vector<core::Recording> &page);
    [[nodiscard]] core::StoreStatus countRecordingsOn(const std::string &callId);

    [[nodiscard]] core::StoreStatus countTranscriptsOn(const std::string &recordingId);

    [[nodiscard]] core::StoreStatus writeSegments(const core::Transcript &transcript);
    [[nodiscard]] core::StoreStatus readSegments(core::Transcript &transcript);

    [[nodiscard]] core::StoreStatus installTaxonomy(std::vector<std::string> &written);
    [[nodiscard]] core::StoreStatus installTemplates(std::vector<std::string> &written);

    [[nodiscard]] core::StoreStatus fillEmptyShippedPrompts();
    [[nodiscard]] core::StoreStatus installAutomations(std::vector<std::string> &written);

    [[nodiscard]] core::StoreStatus alreadySeeded(std::string_view kind,
                                                  std::set<std::string> &codes);

    [[nodiscard]] core::StoreStatus rememberSeeded(std::string_view kind,
                                                   const std::vector<std::string> &codes);

    [[nodiscard]] core::StoreStatus taxonomyIsUsed(core::TaxonomyKind kind, const std::string &code,
                                                   bool &used);

    [[nodiscard]] core::StoreStatus countRedFlagsOn(const std::string &recordingId);

    [[nodiscard]] core::StoreStatus execute(const char *sql);
    [[nodiscard]] core::StoreStatus remember(int resultCode);

    mutable std::recursive_mutex mutex_;

    sqlite3 *database_{nullptr};

    std::filesystem::path file_;
    std::string lastError_;
    std::string contentLanguage_{"en"};
};

}
