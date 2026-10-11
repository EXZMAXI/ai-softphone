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

#include <softphone/core/asr.hpp>
#include <softphone/core/asr_plan.hpp>
#include <softphone/core/jobs.hpp>
#include <softphone/core/providers.hpp>
#include <softphone/core/recordings.hpp>
#include <softphone/core/transcriber.hpp>
#include <softphone/core/transcripts.hpp>

#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace softphone::core {

class TranscriptionService {
public:

    using ProviderMaker = std::function<std::shared_ptr<IAsrProvider>(const ProviderConfig &)>;

    using SecretLookup = std::function<std::string(const std::string &providerId)>;

    using Watcher = std::function<void(const Transcript &)>;

    struct Settings {

        bool automatic{false};

        std::string language;
    };

    TranscriptionService(IRecordingStore &recordings, ITranscriptStore &transcripts,
                         IProviderStore &providers, JobRunner &runner, std::filesystem::path root,
                         WallClock clock = defaultWallClock());

    TranscriptionService(const TranscriptionService &) = delete;
    TranscriptionService &operator=(const TranscriptionService &) = delete;
    TranscriptionService(TranscriptionService &&) = delete;
    TranscriptionService &operator=(TranscriptionService &&) = delete;
    ~TranscriptionService() = default;

    void setSettings(Settings settings);
    void setProviderMaker(ProviderMaker maker);
    void setSecretLookup(SecretLookup lookup);
    void setPreparer(AudioPreparer prepare);
    void watch(Watcher watcher);

    void arm(bool armed);

    void attach();

    [[nodiscard]] StoreStatus transcribe(const std::string &recordingId,
                                         const std::string &providerId = {},
                                         const std::string &language = {}, int priority = 0,
                                         const std::string &automationId = {});

    void observe(const Recording &recording);

    [[nodiscard]] JobOutcome run(const Job &job, const JobRunner::Cancelled &cancelled);

private:
    [[nodiscard]] bool armed() const;

    [[nodiscard]] AsrSubject subjectFor(const Recording &recording) const;
    [[nodiscard]] std::string keepRaw(const Recording &recording, const Transcript &transcript,
                                      const std::vector<Transcriber::Outcome::Kept> &answers) const;
    void announce(const Transcript &transcript) const;

    IRecordingStore &recordings_;
    ITranscriptStore &transcripts_;
    IProviderStore &providers_;
    JobRunner &runner_;
    std::filesystem::path root_;
    WallClock clock_;

    std::atomic<bool> armed_{false};

    Settings settings_;
    ProviderMaker maker_;
    SecretLookup secret_;
    AudioPreparer prepare_;
    Watcher watcher_;
};

}
