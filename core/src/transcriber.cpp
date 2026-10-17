// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/transcriber.hpp>

#include <softphone/core/logging.hpp>

#include <algorithm>
#include <filesystem>
#include <system_error>
#include <utility>

namespace softphone::core {
namespace {

using namespace std::chrono_literals;

constexpr std::string_view kArea{"asr"};

void forget(const PreparedAudio &prepared)
{
    if (!prepared.temporary || prepared.file.empty()) {
        return;
    }
    std::error_code ignored;
    std::filesystem::remove(prepared.file, ignored);
}

}

Transcriber::Transcriber(IAsrProvider &provider, AudioPreparer prepare, WallClock clock,
                         SpeechFinder findSpeech)
    : provider_{provider}, prepare_{std::move(prepare)},
      clock_{clock ? std::move(clock) : defaultWallClock()}, findSpeech_{std::move(findSpeech)}
{}

Transcriber::Outcome Transcriber::run(const Request &request, const AsrCancel &cancelled)
{
    Outcome outcome;
    auto &transcript = outcome.transcript;
    transcript.recordingId = request.recordingId;
    transcript.providerId = request.providerId;
    transcript.providerType = std::string{provider_.type()};
    transcript.model = request.endpoint.model;
    transcript.requestedLanguage = request.options.language;
    transcript.requestedAt = clock_();
    transcript.status = TranscriptStatus::Running;

    const auto give = [&outcome, &transcript, this](AsrStatus status, std::string error,
                                                    bool retry) -> Outcome & {
        outcome.status = status;
        outcome.error = std::move(error);
        outcome.retry = retry;
        transcript.status =
            status == AsrStatus::Cancelled ? TranscriptStatus::Cancelled : TranscriptStatus::Failed;
        transcript.error = outcome.error;
        transcript.completedAt = clock_();
        return outcome;
    };

    const auto can = provider_.capabilities(request.endpoint);

    auto subject = request.subject;
    if (!can.answersWithTimes && findSpeech_) {
        subject.speech = findSpeech_(subject.file, subject.duration);
        for (auto &stem : subject.stems) {
            if (stem.present && !stem.file.empty() && stem.duration > 0ms) {
                stem.speech = findSpeech_(stem.file, stem.duration);
            }
        }
    }

    const auto plan = planTranscription(can, subject, request.options);
    if (!plan.usable()) {
        const auto why = plan.refusal.empty() ? "there was nothing to send" : plan.refusal;
        return give(AsrStatus::Unsupported, why, false);
    }

    {
        const auto steps = std::to_string(plan.steps.size());
        const LogAttribute fields[] = {{"recording", request.recordingId},
                                       {"provider", transcript.providerType},
                                       {"requests", steps}};
        Logger::instance().log(LogLevel::Info, kArea, "transcribing", fields);
    }

    std::vector<AsrRun> runs;
    runs.reserve(plan.steps.size());
    auto untimed = false;

    for (const auto &step : plan.steps) {
        if (cancelled && cancelled()) {
            return give(AsrStatus::Cancelled, "cancelled", false);
        }

        const auto prepared = prepare_ ? prepare_(step.input) : PreparedAudio{};
        if (!prepared.error.empty() || prepared.file.empty()) {
            const auto why =
                prepared.error.empty() ? "the audio could not be prepared" : prepared.error;
            return give(AsrStatus::Failed, why, false);
        }

        auto input = step.input;
        input.file = prepared.file;

        auto asking = step.options;
        if (asking.language.empty() && !transcript.detectedLanguage.empty()) {
            asking.language = transcript.detectedLanguage;
        }

        auto answer = provider_.transcribe(request.endpoint, input, asking, cancelled);
        forget(prepared);

        for (const auto &[name, value] : answer.learnt) {
            const LogAttribute fields[] = {{"recording", request.recordingId},
                                           {"provider", request.providerId},
                                           {"model", request.endpoint.model},
                                           {"name", name},
                                           {"value", value}};
            Logger::instance().log(LogLevel::Info, kArea,
                                   "the recogniser would not answer in that shape, so it was "
                                   "asked for another",
                                   fields);
        }

        if (!answer.raw.empty()) {
            outcome.raw.push_back(Outcome::Kept{voiceText(input.voice, input.stemId),
                                                input.offset.count(), answer.raw});
        }
        transcript.costMicros += answer.costMicros;
        transcript.audio += answer.audio;

        if (!succeeded(answer.status)) {
            return give(answer.status, answer.error, worthRetrying(answer.status));
        }
        if (transcript.detectedLanguage.empty() && !answer.detectedLanguage.empty()) {
            transcript.detectedLanguage = answer.detectedLanguage;
        }
        if (!answer.model.empty()) {
            transcript.model = answer.model;
        }

        untimed = untimed || !answer.timed;

        runs.push_back(AsrRun{std::move(input), std::move(answer)});
    }

    if (untimed && !plan.cutAtSilences) {
        const LogAttribute fields[] = {{"recording", request.recordingId},
                                       {"provider", transcript.providerType},
                                       {"model", transcript.model}};
        Logger::instance().log(LogLevel::Warning, kArea,
                               "this recogniser answered with words and no times, so the "
                               "transcript has no turns and nothing to follow the sound with",
                               fields);
    }

    if (plan.cutAtSilences) {
        const auto lines = std::to_string(plan.steps.size());
        const LogAttribute fields[] = {{"recording", request.recordingId},
                                       {"provider", transcript.providerType},
                                       {"model", transcript.model},
                                       {"requests", lines}};
        Logger::instance().log(LogLevel::Info, kArea,
                               "this model answers without times, so the recording was cut "
                               "into phrases here and the lines are stamped where they were "
                               "sent from",
                               fields);
    }

    transcript.segments = mergeRuns(runs);
    transcript.wordCount = countWords(transcript.segments);
    transcript.status = TranscriptStatus::Done;
    transcript.completedAt = clock_();
    outcome.status = AsrStatus::Ok;
    return outcome;
}

}
