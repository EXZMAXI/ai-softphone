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
#include <functional>
#include <string>
#include <string_view>

namespace softphone::plugins::storage {

class IStorageAccess {
public:
    IStorageAccess() = default;
    IStorageAccess(const IStorageAccess &) = delete;
    IStorageAccess &operator=(const IStorageAccess &) = delete;
    IStorageAccess(IStorageAccess &&) = delete;
    IStorageAccess &operator=(IStorageAccess &&) = delete;
    virtual ~IStorageAccess() = default;

    virtual void post(std::function<void(core::IContactStore &, core::ICallLog &)> work) = 0;

    virtual void withRecordings(std::function<void(core::IRecordingStore &)> work) = 0;

    [[nodiscard]] virtual core::IJobStore *jobs() noexcept = 0;
    [[nodiscard]] virtual core::IProviderStore *providers() noexcept = 0;
    [[nodiscard]] virtual core::ITranscriptStore *transcripts() noexcept = 0;
    [[nodiscard]] virtual core::ITaxonomyStore *taxonomy() noexcept = 0;
    [[nodiscard]] virtual core::ITemplateStore *templates() noexcept = 0;
    [[nodiscard]] virtual core::IAutomationStore *automations() noexcept = 0;
    [[nodiscard]] virtual core::IArtifactStore *artifacts() noexcept = 0;
    [[nodiscard]] virtual core::IUsageStore *usage() noexcept = 0;
    [[nodiscard]] virtual core::IRecordingStore *library() noexcept = 0;

    [[nodiscard]] virtual core::IContactStore *contacts() noexcept = 0;

    [[nodiscard]] virtual core::ICallLog &callLog() noexcept = 0;

    [[nodiscard]] virtual core::ICallLog *history() noexcept = 0;

    [[nodiscard]] virtual core::StoreStatus status() const = 0;

    virtual void startAgain(std::function<void(core::StoreStatus)> done) = 0;

    virtual void setContentLanguage(std::string language) = 0;

    virtual void declareCollections(std::string owner, int version, std::string ddl) = 0;

    virtual void seedShippedDictionaries() = 0;

    virtual void seedShippedPrompts() = 0;

    [[nodiscard]] virtual const std::filesystem::path &file() const noexcept = 0;
};

inline constexpr std::string_view kStorageAccess = "storage.access";

}
