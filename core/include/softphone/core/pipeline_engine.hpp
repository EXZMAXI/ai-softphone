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
#include <softphone/core/providers.hpp>
#include <softphone/core/recordings.hpp>
#include <softphone/core/taxonomy.hpp>
#include <softphone/core/templates.hpp>
#include <softphone/core/transcripts.hpp>
#include <softphone/core/usage.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace softphone::core {

class PipelineEngine {
public:
    struct Settings {

        bool automatic{false};

        std::int64_t monthlyCeilingMicros{0};

        std::int64_t monthlyCeilingTokens{0};
    };

    using Transcribe =
        std::function<StoreStatus(const std::string &recordingId, const std::string &providerId,
                                  int priority, const std::string &automationId)>;
    using Generate = std::function<StoreStatus(
        const std::string &recordingId, const std::string &templateId,
        const std::string &providerId, int priority, const std::string &automationId)>;

    using Busy = std::function<bool(const std::string &recordingId)>;

    using Watcher = std::function<void()>;

    PipelineEngine(IRecordingStore &recordings, ITranscriptStore &transcripts,
                   ITaxonomyStore &taxonomy, IAutomationStore &automations,
                   ITemplateStore &templates, IUsageStore &usage,
                   WallClock clock = defaultWallClock());

    PipelineEngine(const PipelineEngine &) = delete;
    PipelineEngine &operator=(const PipelineEngine &) = delete;
    PipelineEngine(PipelineEngine &&) = delete;
    PipelineEngine &operator=(PipelineEngine &&) = delete;
    ~PipelineEngine() = default;

    void setSettings(Settings settings);
    void setActions(Transcribe transcribe, Generate generate, Busy busy = {});
    void watch(Watcher watcher);

    [[nodiscard]] Settings settings() const
    {
        return settings_;
    }

    void advance(const std::string &recordingId);

    [[nodiscard]] StoreStatus factsFor(const std::string &recordingId, PipelineFacts &out) const;

    struct Spend {
        UsageTotals month;

        bool paused{false};

        bool byMoney{false};
        bool byTokens{false};
    };

    [[nodiscard]] Spend spend() const;

    [[nodiscard]] StoreStatus rerun(const std::string &recordingId);

private:
    [[nodiscard]] std::string templateIdFor(const std::string &code) const;

    IRecordingStore &recordings_;
    ITranscriptStore &transcripts_;
    ITaxonomyStore &taxonomy_;
    IAutomationStore &automations_;
    ITemplateStore &templates_;
    IUsageStore &usage_;
    WallClock clock_;

    Settings settings_;
    Transcribe transcribe_;
    Generate generate_;
    Busy busy_;
    Watcher watcher_;
};

[[nodiscard]] std::chrono::system_clock::time_point
startOfMonth(std::chrono::system_clock::time_point when);

}
