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

#include <softphone/core/answers.hpp>
#include <softphone/core/artifacts.hpp>
#include <softphone/core/generation_plan.hpp>
#include <softphone/core/jobs.hpp>
#include <softphone/core/llm.hpp>
#include <softphone/core/providers.hpp>
#include <softphone/core/recordings.hpp>
#include <softphone/core/taxonomy.hpp>
#include <softphone/core/templates.hpp>
#include <softphone/core/transcripts.hpp>
#include <softphone/core/usage.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <string>

namespace softphone::core {

class GenerationService {
public:

    using ProviderMaker = std::function<std::shared_ptr<ILlmProvider>(const ProviderConfig &)>;

    using SecretLookup = std::function<std::string(const std::string &providerId)>;

    using Watcher = std::function<void(const Artifact &)>;

    struct Settings {

        std::string interfaceLanguage;

        int answerTokens{1024};
    };

    GenerationService(IRecordingStore &recordings, ITranscriptStore &transcripts,
                      ITemplateStore &templates, IArtifactStore &artifacts,
                      IProviderStore &providers, ITaxonomyStore &taxonomy, IUsageStore &usage,
                      JobRunner &runner, WallClock clock = defaultWallClock());

    GenerationService(const GenerationService &) = delete;
    GenerationService &operator=(const GenerationService &) = delete;
    GenerationService(GenerationService &&) = delete;
    GenerationService &operator=(GenerationService &&) = delete;
    ~GenerationService() = default;

    void setSettings(Settings settings);

    void setInterfaceLanguage(std::string language);

    [[nodiscard]] const std::string &interfaceLanguage() const noexcept
    {
        return settings_.interfaceLanguage;
    }

    void setProviderMaker(ProviderMaker maker);
    void setSecretLookup(SecretLookup lookup);
    void watch(Watcher watcher);

    void arm(bool armed);

    void attach();

    [[nodiscard]] StoreStatus generate(const std::string &recordingId,
                                       const std::string &templateId,
                                       const std::string &providerId = {}, int priority = 0,
                                       const std::string &question = {},
                                       const std::string &automationId = {});

    [[nodiscard]] JobOutcome run(const Job &job, const JobRunner::Cancelled &cancelled);

private:
    [[nodiscard]] bool armed() const;

    [[nodiscard]] StoreStatus contextFor(const Recording &recording, const Transcript &transcript,
                                         PromptContext &out) const;

    void applyDigest(const Artifact &artifact, const Answer &answer, Recording &recording,
                     const PromptContext &context);

    void announce(const Artifact &artifact) const;

    IRecordingStore &recordings_;
    ITranscriptStore &transcripts_;
    ITemplateStore &templates_;
    IArtifactStore &artifacts_;
    IProviderStore &providers_;
    ITaxonomyStore &taxonomy_;
    IUsageStore &usage_;
    JobRunner &runner_;
    WallClock clock_;

    std::atomic<bool> armed_{false};

    Settings settings_;
    ProviderMaker maker_;
    SecretLookup secret_;
    Watcher watcher_;
};

[[nodiscard]] std::string asTurns(const Transcript &transcript);

[[nodiscard]] std::string asTurns(const Transcript &transcript, Voice only);

}
