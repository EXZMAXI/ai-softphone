// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/storage/storage_host.hpp>

#include <softphone/core/uuid.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/storage/sqlite_store.hpp>

#include <string>
#include <utility>

namespace softphone::plugins::storage {
namespace {

namespace backend = ::softphone::storage;

}

class StorageHost::QueuedCallLog final : public core::ICallLog {
public:
    explicit QueuedCallLog(StorageHost &host) : host_{host} {}

    core::StoreStatus record(core::CallRecord &entry) override
    {

        if (entry.id.empty()) {
            entry.id = core::makeUuid();
        }

        host_.post([entry](core::IContactStore &, core::ICallLog &log) mutable {
            static_cast<void>(log.record(entry));
        });
        return core::StoreStatus::Ok;
    }

    core::StoreStatus calls(const core::CallLogQuery &, std::vector<core::CallRecord> &) override
    {
        return core::StoreStatus::Unavailable;
    }

    core::StoreStatus removeCall(const std::string &id) override
    {
        host_.post([id](core::IContactStore &, core::ICallLog &log) {
            static_cast<void>(log.removeCall(id));
        });
        return core::StoreStatus::Ok;
    }

    core::StoreStatus clearCalls() override
    {
        host_.post([](core::IContactStore &, core::ICallLog &log) {
            static_cast<void>(log.clearCalls());
        });
        return core::StoreStatus::Ok;
    }

    core::StoreStatus removeCallsBefore(std::chrono::system_clock::time_point before,
                                        int &removed) override
    {

        removed = 0;
        host_.post([before](core::IContactStore &, core::ICallLog &log) {
            auto gone = 0;
            static_cast<void>(log.removeCallsBefore(before, gone));
        });
        return core::StoreStatus::Ok;
    }

private:
    StorageHost &host_;
};

StorageHost::StorageHost()
    : store_{std::make_unique<backend::SqliteStore>()},
      queued_{std::make_unique<QueuedCallLog>(*this)}
{}

core::ICallLog &StorageHost::callLog() noexcept
{
    return *queued_;
}

StorageHost::~StorageHost()
{
    stop();
}

void StorageHost::setContentLanguage(std::string language)
{
    store_->setContentLanguage(std::move(language));
}

void StorageHost::start(const std::filesystem::path &file,
                        std::function<void(core::StoreStatus)> opened)
{
    if (thread_.joinable()) {
        return;
    }

    file_ = file;
    loop_.post([this, file, opened = std::move(opened)]() {
        const auto status = store_->open(file);
        status_.store(status, std::memory_order_relaxed);

        if (status != core::StoreStatus::Ok) {

            core::Logger::instance().log(
                core::LogLevel::Error, "storage",
                "database unavailable: " + std::string{core::toString(status)} + " (" +
                    store_->lastError() + ")");
        }

        if (opened) {
            opened(status);
        }
    });

    thread_ = std::thread{[this]() { loop_.run(); }};
}

core::IJobStore *StorageHost::jobs() noexcept
{
    return store_.get();
}

core::IProviderStore *StorageHost::providers() noexcept
{
    return store_.get();
}

core::ITaxonomyStore *StorageHost::taxonomy() noexcept
{
    return store_.get();
}

core::ITemplateStore *StorageHost::templates() noexcept
{
    return store_.get();
}

core::IAutomationStore *StorageHost::automations() noexcept
{
    return store_.get();
}

core::IArtifactStore *StorageHost::artifacts() noexcept
{
    return store_.get();
}

core::IUsageStore *StorageHost::usage() noexcept
{
    return store_.get();
}

core::ITranscriptStore *StorageHost::transcripts() noexcept
{
    return store_.get();
}

core::IRecordingStore *StorageHost::library() noexcept
{
    return store_.get();
}

core::IContactStore *StorageHost::contacts() noexcept
{
    return store_.get();
}

core::ICallLog *StorageHost::history() noexcept
{
    return store_.get();
}

void StorageHost::stop()
{
    if (!thread_.joinable()) {
        return;
    }

    loop_.post([this]() {
        store_->close();
        loop_.stop();
    });
    thread_.join();
}

void StorageHost::post(std::function<void(core::IContactStore &, core::ICallLog &)> work)
{
    if (!work) {
        return;
    }
    loop_.post([this, work = std::move(work)]() { work(*store_, *store_); });
}

void StorageHost::withRecordings(std::function<void(core::IRecordingStore &)> work)
{
    if (!work) {
        return;
    }
    loop_.post([this, work = std::move(work)]() { work(*store_); });
}

void StorageHost::startAgain(std::function<void(core::StoreStatus)> done)
{
    if (!thread_.joinable()) {
        return;
    }

    loop_.post([this, done = std::move(done)]() {
        const auto status = store_->recreate();
        status_.store(status, std::memory_order_relaxed);

        if (status == core::StoreStatus::Ok) {
            for (const auto &declaration : declared_) {
                static_cast<void>(store_->declareCollections(declaration.owner, declaration.version,
                                                             declaration.ddl));
            }
            if (seedDictionaries_) {
                static_cast<void>(store_->installShippedTaxonomy());
            }
            if (seedPrompts_) {
                static_cast<void>(store_->installShippedPrompts());
            }
        }

        if (done) {
            done(status);
        }
    });
}

core::StoreStatus StorageHost::status() const
{
    return status_.load(std::memory_order_relaxed);
}

void StorageHost::declareCollections(std::string owner, int version, std::string ddl)
{
    if (owner.empty() || ddl.empty()) {
        return;
    }

    declared_.push_back(Declaration{owner, version, ddl});

    loop_.post([this, owner = std::move(owner), version, ddl = std::move(ddl)]() {
        const auto status = store_->declareCollections(owner, version, ddl);
        if (status != core::StoreStatus::Ok) {

            core::Logger::instance().log(core::LogLevel::Error, "storage",
                                         "could not make the tables for " + owner + ": " +
                                             std::string{core::toString(status)} + " (" +
                                             store_->lastError() + ")");
        }
    });
}

void StorageHost::seedShippedDictionaries()
{
    seedDictionaries_ = true;

    loop_.post([this]() {
        if (const auto status = store_->installShippedTaxonomy(); status != core::StoreStatus::Ok) {
            core::Logger::instance().log(
                core::LogLevel::Error, "storage",
                "could not offer the shipped dictionaries: " + std::string{core::toString(status)} +
                    " (" + store_->lastError() + ")");
        }
    });
}

void StorageHost::seedShippedPrompts()
{
    seedPrompts_ = true;

    loop_.post([this]() {
        if (const auto status = store_->installShippedPrompts(); status != core::StoreStatus::Ok) {
            core::Logger::instance().log(core::LogLevel::Error, "storage",
                                         "could not offer the shipped prompts and rules: " +
                                             std::string{core::toString(status)} + " (" +
                                             store_->lastError() + ")");
        }
    });
}

const std::filesystem::path &StorageHost::file() const noexcept
{
    return file_;
}

}
