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

#include <softphone/plugins/storage/storage_access.hpp>

#include <softphone/core/event_loop.hpp>

#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace softphone::storage {
class SqliteStore;
}

namespace softphone::plugins::storage {

class StorageHost final : public IStorageAccess {
public:
    StorageHost();
    StorageHost(const StorageHost &) = delete;
    StorageHost &operator=(const StorageHost &) = delete;
    StorageHost(StorageHost &&) = delete;
    StorageHost &operator=(StorageHost &&) = delete;
    ~StorageHost() override;

    void start(const std::filesystem::path &file,
               std::function<void(core::StoreStatus)> opened = {});

    void setContentLanguage(std::string language) override;

    void stop();

    [[nodiscard]] bool running() const noexcept
    {
        return thread_.joinable();
    }

    void post(std::function<void(core::IContactStore &, core::ICallLog &)> work) override;

    void withRecordings(std::function<void(core::IRecordingStore &)> work) override;

    [[nodiscard]] core::IJobStore *jobs() noexcept override;
    [[nodiscard]] core::IProviderStore *providers() noexcept override;
    [[nodiscard]] core::ITranscriptStore *transcripts() noexcept override;
    [[nodiscard]] core::ITaxonomyStore *taxonomy() noexcept override;
    [[nodiscard]] core::ITemplateStore *templates() noexcept override;
    [[nodiscard]] core::IAutomationStore *automations() noexcept override;
    [[nodiscard]] core::IArtifactStore *artifacts() noexcept override;
    [[nodiscard]] core::IUsageStore *usage() noexcept override;
    [[nodiscard]] core::IRecordingStore *library() noexcept override;
    [[nodiscard]] core::IContactStore *contacts() noexcept override;
    [[nodiscard]] core::ICallLog *history() noexcept override;

    [[nodiscard]] core::StoreStatus status() const override;

    void declareCollections(std::string owner, int version, std::string ddl) override;
    void seedShippedDictionaries() override;
    void seedShippedPrompts() override;

    [[nodiscard]] const std::filesystem::path &file() const noexcept override;

    void startAgain(std::function<void(core::StoreStatus)> done) override;

    [[nodiscard]] core::ICallLog &callLog() noexcept override;

private:
    class QueuedCallLog;

    std::filesystem::path file_;

    struct Declaration {
        std::string owner;
        int version{0};
        std::string ddl;
    };

    std::vector<Declaration> declared_;
    bool seedDictionaries_{false};
    bool seedPrompts_{false};

    core::EventLoop loop_;

    std::unique_ptr<::softphone::storage::SqliteStore> store_;
    std::unique_ptr<QueuedCallLog> queued_;
    std::thread thread_;

    std::atomic<core::StoreStatus> status_{core::StoreStatus::Unavailable};
};

}
