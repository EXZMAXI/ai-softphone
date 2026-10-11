// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/transcription_service.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/transcriber.hpp>
#include <softphone/core/uuid.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <system_error>
#include <utility>

namespace softphone::core {
namespace {

using namespace std::chrono_literals;

constexpr std::string_view kArea{"asr"};

[[nodiscard]] std::string asMinutesAndSeconds(std::chrono::seconds span)
{
    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(span);
    return std::to_string(minutes.count()) + "m" + std::to_string((span - minutes).count()) + "s";
}

[[nodiscard]] std::string paramsFor(const std::string &language)
{
    if (language.empty()) {
        return {};
    }
    nlohmann::json params;
    params["language"] = language;
    return params.dump();
}

[[nodiscard]] std::string languageIn(const std::string &params)
{
    if (params.empty()) {
        return {};
    }
    const auto parsed = nlohmann::json::parse(params, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return {};
    }
    const auto found = parsed.find("language");
    if (found == parsed.end() || !found->is_string()) {
        return {};
    }
    return found->get<std::string>();
}

}

TranscriptionService::TranscriptionService(IRecordingStore &recordings,
                                           ITranscriptStore &transcripts, IProviderStore &providers,
                                           JobRunner &runner, std::filesystem::path root,
                                           WallClock clock)
    : recordings_{recordings}, transcripts_{transcripts}, providers_{providers}, runner_{runner},
      root_{std::move(root)}, clock_{clock ? std::move(clock) : defaultWallClock()}
{}

void TranscriptionService::arm(bool armed)
{
    if (armed_.exchange(armed) == armed) {
        return;
    }
    Logger::instance().log(LogLevel::Info, kArea,
                           armed ? "Transcription is on"
                                 : "Transcription is off; nothing will be queued "
                                   "and nothing claimed will run");
}

bool TranscriptionService::armed() const
{
    return armed_.load();
}

void TranscriptionService::setSettings(Settings settings)
{
    settings_ = std::move(settings);
}

void TranscriptionService::setProviderMaker(ProviderMaker maker)
{
    maker_ = std::move(maker);
}

void TranscriptionService::setSecretLookup(SecretLookup lookup)
{
    secret_ = std::move(lookup);
}

void TranscriptionService::setPreparer(AudioPreparer prepare)
{
    prepare_ = std::move(prepare);
}

void TranscriptionService::watch(Watcher watcher)
{
    watcher_ = std::move(watcher);
}

void TranscriptionService::attach()
{
    runner_.handle(JobKind::Transcribe, [this](const Job &job, const JobRunner::Cancelled &stop) {
        return run(job, stop);
    });
}

StoreStatus TranscriptionService::transcribe(const std::string &recordingId,
                                             const std::string &providerId,
                                             const std::string &language, int priority,
                                             const std::string &automationId)
{

    if (!armed()) {
        return StoreStatus::Missing;
    }

    Recording recording;
    if (const auto status = recordings_.recording(recordingId, recording);
        status != StoreStatus::Ok) {
        return status;
    }
    if (!recording.hasAudio()) {

        return StoreStatus::Missing;
    }
    if (recording.unfinished) {

        return StoreStatus::Missing;
    }

    auto chosen = providerId;
    if (chosen.empty()) {
        ProviderConfig fallback;
        if (const auto status = providers_.defaultProvider(ProviderKind::Asr, fallback);
            status != StoreStatus::Ok) {
            return status;
        }
        chosen = fallback.id;
    }

    Job job;
    job.kind = JobKind::Transcribe;
    job.recordingId = recordingId;
    job.providerId = chosen;
    job.priority = priority;
    job.automationId = automationId;

    job.params = paramsFor(language);
    return runner_.post(job);
}

void TranscriptionService::observe(const Recording &recording)
{
    if (!settings_.automatic || recording.id.empty() || !recording.hasAudio()) {
        return;
    }

    ProviderConfig chosen;
    if (const auto status = providers_.defaultProvider(ProviderKind::Asr, chosen);
        status != StoreStatus::Ok) {
        return;
    }

    if (const auto status = transcribe(recording.id, chosen.id); !succeeded(status)) {
        const LogAttribute fields[] = {{"recording", recording.id}, {"status", toString(status)}};
        Logger::instance().log(LogLevel::Warning, kArea,
                               "a recording could not be queued for transcription", fields);
    }
}

AsrSubject TranscriptionService::subjectFor(const Recording &recording) const
{
    AsrSubject subject;
    subject.file = root_ / recording.filePath;
    subject.mimeType = recording.format == "wav" ? "audio/wav" : "audio/mpeg";
    subject.channels = recording.channels;
    subject.nearFar = recording.channelLayout == "near-far" && recording.channels >= 2;
    subject.duration = recording.duration;
    subject.bytes = recording.bytes;
    subject.parties = recording.partyCount;

    for (const auto &stem : recording.stems) {
        AsrSubject::Stem one;
        one.id = stem.id;
        one.file = stem.filePath.empty() ? std::filesystem::path{} : root_ / stem.filePath;
        one.offset = stem.offset;
        one.duration = stem.duration;
        one.bytes = stem.bytes;
        one.present = stem.droppedAt.time_since_epoch().count() == 0 && !stem.filePath.empty();
        subject.stems.push_back(std::move(one));
    }
    return subject;
}

std::string
TranscriptionService::keepRaw(const Recording &recording, const Transcript &transcript,
                              const std::vector<Transcriber::Outcome::Kept> &answers) const
{
    if (answers.empty()) {
        return {};
    }

    if (transcript.id.empty()) {
        Logger::instance().log(LogLevel::Warning, kArea,
                               "a provider's answer could not be kept: the transcript has no "
                               "name yet, so there is nothing to file it under");
        return {};
    }

    const auto where = std::filesystem::path{recording.filePath}.parent_path();
    const auto relative =
        (where.empty() ? std::filesystem::path{"transcripts"} : where) / (transcript.id + ".json");

    const auto full = root_ / relative;
    std::error_code failed;
    std::filesystem::create_directories(full.parent_path(), failed);

    auto array = nlohmann::ordered_json::array();
    for (const auto &answer : answers) {
        nlohmann::ordered_json entry;
        entry["voice"] = answer.voice;
        entry["offset_ms"] = answer.offsetMs;

        auto parsed = nlohmann::json::parse(answer.body, nullptr, false);
        if (parsed.is_discarded()) {

            entry["answer"] = answer.body;
        } else {
            entry["answer"] = std::move(parsed);
        }
        array.push_back(std::move(entry));
    }

    std::ofstream out{full, std::ios::binary};
    if (!out) {
        const LogAttribute fields[] = {{"transcript", transcript.id}};
        Logger::instance().log(LogLevel::Warning, kArea,
                               "the provider's own answer could not be kept", fields);
        return {};
    }
    const auto text = array.dump();
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    return out ? relative.generic_string() : std::string{};
}

void TranscriptionService::announce(const Transcript &transcript) const
{
    if (watcher_) {
        watcher_(transcript);
    }
}

JobOutcome TranscriptionService::run(const Job &job, const JobRunner::Cancelled &cancelled)
{
    JobOutcome outcome;

    if (!armed()) {
        outcome.error = "the transcription module is off";
        return outcome;
    }

    Recording recording;
    if (const auto status = recordings_.recording(job.recordingId, recording);
        status != StoreStatus::Ok) {
        outcome.error = "the recording is no longer there";
        return outcome;
    }
    if (!recording.hasAudio()) {
        outcome.error = "the recording no longer has any audio";
        return outcome;
    }

    ProviderConfig configured;
    if (const auto status = providers_.provider(job.providerId, configured);
        status != StoreStatus::Ok) {
        outcome.error = "the provider this job was for is no longer configured";
        return outcome;
    }
    if (!configured.enabled) {
        outcome.error = "the provider this job was for is switched off";
        return outcome;
    }

    if (!maker_) {
        outcome.error = "this build has no way to reach a provider";
        return outcome;
    }
    const auto provider = maker_(configured);
    if (!provider) {
        outcome.error = "nothing here knows how to speak to a " + configured.type + " provider";
        return outcome;
    }

    AsrEndpoint endpoint;
    endpoint.url = configured.endpoint;
    endpoint.model = configured.model;
    endpoint.region = configured.region;
    endpoint.params = configured.params;

    endpoint.timeout = asrTimeoutFor(recording.duration, configured.readTimeout);
    if (configured.hasSecret && secret_) {
        endpoint.token = secret_(configured.id);
    }

    const auto spoken = [&job, &configured] {
        const auto asked = languageIn(job.params);
        return asked.empty() ? configured.language : asked;
    }();

    {
        const auto wanted = spoken;
        const LogAttribute fields[] = {
            {"recording", recording.id},
            {"provider", configured.name.empty() ? configured.id : configured.name},
            {"type", configured.type},
            {"language", wanted}};
        Logger::instance().log(LogLevel::Info, kArea, "transcribing", fields);
    }

    Transcriber transcriber{*provider, prepare_, clock_};

    Transcriber::Request request;
    request.recordingId = recording.id;
    request.providerId = configured.id;
    request.subject = subjectFor(recording);
    request.endpoint = endpoint;
    request.options.language = spoken;

    const auto began = std::chrono::steady_clock::now();
    auto answer = transcriber.run(request, cancelled ? AsrCancel{cancelled} : AsrCancel{});
    const auto took =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - began);
    auto &transcript = answer.transcript;

    if (succeeded(answer.status)) {

        if (transcript.id.empty()) {
            transcript.id = makeUuid();
        }
        transcript.rawPath = keepRaw(recording, transcript, answer.raw);
    }

    if (const auto status = transcripts_.save(transcript); !succeeded(status)) {

        outcome.error = "the transcript could not be written: " + std::string{toString(status)};
        outcome.worthRetrying = status == StoreStatus::Busy || status == StoreStatus::Unwritable;
        return outcome;
    }

    announce(transcript);

    if (!succeeded(answer.status)) {
        outcome.error = answer.error;
        outcome.worthRetrying = answer.retry;

        const auto elapsed = asMinutesAndSeconds(took);
        const auto allowed = asMinutesAndSeconds(endpoint.timeout);
        const auto audio = asMinutesAndSeconds(
            std::chrono::duration_cast<std::chrono::seconds>(recording.duration));
        const LogAttribute fields[] = {{"recording", recording.id},
                                       {"status", toString(answer.status)},
                                       {"error", outcome.error},
                                       {"audio", audio},
                                       {"elapsed", elapsed},
                                       {"limit", allowed},
                                       {"again", answer.retry ? "yes" : "no"}};
        Logger::instance().log(LogLevel::Warning, kArea, "a transcription failed", fields);
        return outcome;
    }

    {
        const auto lines = std::to_string(transcript.segments.size());
        const auto words = std::to_string(transcript.wordCount);

        const auto audioMs = std::to_string(transcript.audio.count());
        auto furthest = std::chrono::milliseconds::zero();
        for (const auto &line : transcript.segments) {
            furthest = std::max(furthest, line.end);
        }
        const auto lastMs = std::to_string(furthest.count());
        const LogAttribute fields[] = {{"recording", recording.id},
                                       {"transcript", transcript.id},
                                       {"provider", transcript.providerType},
                                       {"language", transcript.detectedLanguage},
                                       {"lines", lines},
                                       {"words", words},
                                       {"audio_ms", audioMs},
                                       {"last_line_ends_ms", lastMs}};
        Logger::instance().log(LogLevel::Info, kArea, "transcribed", fields);
    }

    outcome.done = true;
    outcome.costMicros = transcript.costMicros;
    return outcome;
}

}
