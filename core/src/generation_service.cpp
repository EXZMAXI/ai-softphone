// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/generation_service.hpp>

#include <softphone/core/answers.hpp>
#include <softphone/core/exchange_log.hpp>
#include <softphone/core/logging.hpp>
#include <softphone/core/templates.hpp>
#include <softphone/core/uuid.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <utility>

namespace softphone::core {
namespace {

constexpr std::string_view kArea = "generation";

[[nodiscard]] std::string paramsFor(const std::string &question)
{
    if (question.empty()) {
        return {};
    }
    nlohmann::json params;
    params["question"] = question;
    return params.dump();
}

[[nodiscard]] std::string questionIn(const std::string &params)
{
    if (params.empty()) {
        return {};
    }
    const auto parsed = nlohmann::json::parse(params, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return {};
    }
    return parsed.value("question", std::string{});
}

[[nodiscard]] JobOutcome refuse(std::string error, bool again = false)
{
    JobOutcome outcome;
    outcome.done = false;
    outcome.worthRetrying = again;
    outcome.error = std::move(error);

    const LogAttribute fields[] = {{"again", again ? "yes" : "no"}};
    Logger::instance().log(LogLevel::Warning, kArea, outcome.error, fields);
    return outcome;
}

}

std::string asTurns(const Transcript &transcript)
{

    const auto &spoken = transcript.detectedLanguage.empty() ? transcript.requestedLanguage
                                                             : transcript.detectedLanguage;

    std::string text;
    for (const auto &segment : transcript.segments) {
        const auto &said = segment.editedText.empty() ? segment.text : segment.editedText;
        if (said.empty()) {
            continue;
        }
        if (!text.empty()) {
            text += '\n';
        }

        text += "[" + std::to_string(segment.start.count()) + " ms] ";

        const auto parsed = parseVoice(segment.voice);
        const auto voice = parsed.has_value() ? parsed->first : Voice::Mixed;
        if (voice == Voice::Stem && !segment.speaker.empty()) {
            text += segment.speaker + ": ";
        } else {
            text += std::string{speakerWord(voice, spoken)} + ": ";
        }

        text += said;
    }
    return text;
}

std::string asTurns(const Transcript &transcript, Voice only)
{
    Transcript oneSide = transcript;
    oneSide.segments.clear();
    for (const auto &segment : transcript.segments) {
        const auto parsed = parseVoice(segment.voice);
        if (parsed.has_value() && parsed->first == only) {
            oneSide.segments.push_back(segment);
        }
    }
    return asTurns(oneSide);
}

GenerationService::GenerationService(IRecordingStore &recordings, ITranscriptStore &transcripts,
                                     ITemplateStore &templates, IArtifactStore &artifacts,
                                     IProviderStore &providers, ITaxonomyStore &taxonomy,
                                     IUsageStore &usage, JobRunner &runner, WallClock clock)
    : recordings_{recordings}, transcripts_{transcripts}, templates_{templates},
      artifacts_{artifacts}, providers_{providers}, taxonomy_{taxonomy}, usage_{usage},
      runner_{runner}, clock_{std::move(clock)}
{}

void GenerationService::arm(bool armed)
{
    if (armed_.exchange(armed) == armed) {
        return;
    }
    Logger::instance().log(LogLevel::Info, kArea,
                           armed ? "Processing is on"
                                 : "Processing is off; nothing will be queued "
                                   "and nothing claimed will run");
}

bool GenerationService::armed() const
{
    return armed_.load();
}

void GenerationService::setSettings(Settings settings)
{
    settings_ = std::move(settings);
}

void GenerationService::setInterfaceLanguage(std::string language)
{
    settings_.interfaceLanguage = std::move(language);
}

void GenerationService::setProviderMaker(ProviderMaker maker)
{
    maker_ = std::move(maker);
}

void GenerationService::setSecretLookup(SecretLookup lookup)
{
    secret_ = std::move(lookup);
}

void GenerationService::watch(Watcher watcher)
{
    watcher_ = std::move(watcher);
}

void GenerationService::attach()
{
    runner_.handle(JobKind::Generate, [this](const Job &job, const JobRunner::Cancelled &stop) {
        return run(job, stop);
    });
}

StoreStatus GenerationService::generate(const std::string &recordingId,
                                        const std::string &templateId,
                                        const std::string &providerId, int priority,
                                        const std::string &question,
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

    PromptTemplate prompt;
    if (const auto status = templates_.prompt(templateId, prompt); status != StoreStatus::Ok) {
        return status;
    }

    auto chosen = providerId;
    if (chosen.empty()) {
        chosen = prompt.providerId;
    }
    if (chosen.empty()) {
        ProviderConfig fallback;
        if (const auto status = providers_.defaultProvider(ProviderKind::Llm, fallback);
            status != StoreStatus::Ok) {
            return status;
        }
        chosen = fallback.id;
    }

    Job job;
    job.kind = JobKind::Generate;
    job.recordingId = recordingId;
    job.templateId = templateId;
    job.providerId = chosen;
    job.priority = priority;
    job.automationId = automationId;
    job.params = paramsFor(question);
    return runner_.post(job);
}

StoreStatus GenerationService::contextFor(const Recording &recording, const Transcript &transcript,
                                          PromptContext &out) const
{
    out.transcript = asTurns(transcript);
    out.nearSide = asTurns(transcript, Voice::Near);
    out.farSide = asTurns(transcript, Voice::Far);
    out.duration = std::chrono::duration_cast<std::chrono::milliseconds>(recording.duration);
    out.direction = recording.direction == CallDirection::Incoming ? "incoming" : "outgoing";
    out.party = recording.remoteName.empty() ? recording.remoteNumber : recording.remoteName;
    out.account = recording.accountId;
    out.language = transcript.detectedLanguage.empty() ? transcript.requestedLanguage
                                                       : transcript.detectedLanguage;

    static_cast<void>(taxonomy_.entries(TaxonomyKind::Category, false, out.categories));
    static_cast<void>(taxonomy_.entries(TaxonomyKind::Tag, false, out.tags));
    static_cast<void>(taxonomy_.entries(TaxonomyKind::RedFlag, false, out.redFlags));

    static_cast<void>(taxonomy_.entries(TaxonomyKind::Instruction, true, out.instructions));

    ArtifactQuery query;
    query.recordingId = recording.id;
    std::vector<Artifact> made;
    if (const auto status = artifacts_.artifacts(query, made); status != StoreStatus::Ok) {
        return status;
    }
    for (const auto &artifact : made) {
        if (artifact.status != ArtifactStatus::Done || artifact.content.empty()) {
            continue;
        }
        const auto kind = std::string{toString(artifact.kind)};

        out.artifacts.emplace(kind, artifact.content);
    }
    return StoreStatus::Ok;
}

JobOutcome GenerationService::run(const Job &job, const JobRunner::Cancelled &cancelled)
{

    if (!armed()) {
        return refuse("the processing module is off");
    }

    if (!maker_) {
        return refuse("this build cannot reach a language model");
    }

    Recording recording;
    if (const auto status = recordings_.recording(job.recordingId, recording);
        status != StoreStatus::Ok) {
        return refuse("the conversation is no longer there");
    }

    Transcript transcript;
    if (const auto status = transcripts_.primaryTranscript(job.recordingId, transcript);
        status != StoreStatus::Ok) {
        return refuse("the conversation has not been transcribed yet");
    }
    if (transcript.segments.empty()) {
        return refuse("the transcript is empty");
    }

    PromptTemplate prompt;
    if (const auto status = templates_.prompt(job.templateId, prompt); status != StoreStatus::Ok) {
        return refuse("the prompt this was queued with is no longer there");
    }

    ProviderConfig configured;
    if (const auto status = providers_.provider(job.providerId, configured);
        status != StoreStatus::Ok) {
        return refuse("the model this was queued with is no longer configured");
    }
    if (!configured.enabled) {
        return refuse("the model this was queued with is switched off");
    }

    const auto provider = maker_(configured);
    if (!provider) {
        return refuse("this build does not know that kind of model");
    }

    {
        const LogAttribute fields[] = {
            {"recording", job.recordingId},
            {"prompt", prompt.code.empty() ? prompt.name : prompt.code},
            {"kind", toString(prompt.kind)},
            {"shape", toString(prompt.shape)},
            {"model", configured.name.empty() ? configured.id : configured.name}};
        Logger::instance().log(LogLevel::Info, kArea, "asking a model", fields);
    }

    PromptContext context;
    if (const auto status = contextFor(recording, transcript, context); status != StoreStatus::Ok) {
        return refuse("what the conversation already holds could not be read", true);
    }
    context.question = questionIn(job.params);

    LlmEndpoint where;
    where.url = configured.endpoint;
    where.model = configured.model;
    where.region = configured.region;
    where.params = configured.params;
    if (secret_) {
        where.token = secret_(configured.id);
    }
    for (const auto &[name, value] : prompt.params) {

        auto found = std::find_if(where.params.begin(), where.params.end(),
                                  [&name](const auto &pair) { return pair.first == name; });
        if (found == where.params.end()) {
            where.params.emplace_back(name, value);
        } else {
            found->second = value;
        }
    }

    const auto what = provider->capabilities(where);

    FoldBudget budget;

    budget.answerTokens = what.maxOutputTokens > 0
                              ? std::max(1, std::min(settings_.answerTokens, what.maxOutputTokens))
                              : std::max(1, settings_.answerTokens);

    const auto plan = planGeneration(prompt, context, what, settings_.interfaceLanguage, budget);
    if (!plan.usable()) {
        return refuse(plan.refusal);
    }

    Artifact artifact;
    artifact.recordingId = recording.id;
    artifact.transcriptId = transcript.id;
    artifact.templateId = prompt.id;
    artifact.templateCode = prompt.code;
    artifact.templateName = prompt.name;
    artifact.templateVersion = prompt.version;
    artifact.providerId = configured.id;
    artifact.providerType = configured.type;
    artifact.model = configured.model;
    artifact.kind = prompt.kind;
    artifact.shape = prompt.shape;
    artifact.status = ArtifactStatus::Running;

    artifact.language = answerLanguage(prompt, context, settings_.interfaceLanguage);
    artifact.parts = static_cast<int>(plan.steps.size());
    artifact.requestedAt = clock_();
    artifact.currency = configured.currency;

    artifact.question = context.question;

    if (const auto status = artifacts_.save(artifact); status != StoreStatus::Ok) {
        return refuse("the answer could not be written down", true);
    }
    announce(artifact);

    LlmOptions options;
    options.wantsJson = isStructured(prompt.shape);

    const auto spend = [this, &artifact, &configured, &job](const LlmAnswer &answer) {
        artifact.promptTokens += answer.usage.promptTokens;
        artifact.answerTokens += answer.usage.answerTokens;
        const auto cost =
            costMicros(answer.usage, configured.inPricePerMillion, configured.outPricePerMillion);
        artifact.costMicros += cost;

        UsageEntry entry;
        entry.at = clock_();
        entry.providerId = configured.id;
        entry.providerType = configured.type;
        entry.model = answer.model.empty() ? configured.model : answer.model;
        entry.kind = JobKind::Generate;
        entry.jobId = job.id;
        entry.recordingId = artifact.recordingId;
        entry.promptTokens = answer.usage.promptTokens;
        entry.answerTokens = answer.usage.answerTokens;
        entry.costMicros = cost;
        entry.currency = configured.currency;
        static_cast<void>(usage_.recordUsage(entry));
    };

    const auto fail = [this, &artifact](LlmStatus status, const std::string &detail) {
        artifact.status = ArtifactStatus::Failed;
        artifact.completedAt = clock_();
        artifact.error = detail.empty() ? std::string{toString(status)} : detail;
        static_cast<void>(artifacts_.save(artifact));
        announce(artifact);
        return refuse(artifact.error, worthRetrying(status));
    };

    const auto remember = [&artifact](const LlmAnswer &answer, int part, int of) {
        exchange::keep(artifact.id, part, of, answer.request, answer.raw);
        for (const auto &[name, value] : answer.learnt) {
            const LogAttribute fields[] = {{"provider", artifact.providerId},
                                           {"model", artifact.model},
                                           {"name", name},
                                           {"value", value}};
            Logger::instance().log(LogLevel::Info, kArea,
                                   "the model refused a parameter, so it was sent without it",
                                   fields);
        }
    };

    std::vector<std::string> answers;
    answers.reserve(plan.steps.size());

    for (const auto &step : plan.steps) {
        if (cancelled && cancelled()) {

            return refuse("cancelled", true);
        }

        options.maxOutputTokens = answerRoomFor(step.prompt, what, budget);
        const auto answer = provider->ask(where, step.prompt.messages, options, cancelled);
        spend(answer);
        remember(answer, step.part, step.of);
        if (answer.status != LlmStatus::Ok) {
            return fail(answer.status, answer.detail);
        }
        answers.push_back(answer.text);
        artifact.raw = answer.raw;
        if (!answer.model.empty()) {
            artifact.model = answer.model;
        }
    }

    std::string text = answers.front();
    if (plan.folded) {
        const auto folded = foldAnswers(prompt, answers, context, settings_.interfaceLanguage);
        if (!folded.usable()) {
            return fail(LlmStatus::Failed, "the parts could not be brought together");
        }

        options.maxOutputTokens = answerRoomFor(folded, what, budget);

        const auto answer = provider->ask(where, folded.messages, options, cancelled);
        spend(answer);

        remember(answer, static_cast<int>(plan.steps.size()) + 1,
                 static_cast<int>(plan.steps.size()) + 1);

        if (answer.status == LlmStatus::TooLarge) {
            return fail(answer.status,
                        "the parts' answers are longer together than one answer may be: raise "
                        "the model's answer limit, or ask the prompt for less");
        }
        if (answer.status != LlmStatus::Ok) {
            return fail(answer.status, answer.detail);
        }
        text = answer.text;
        artifact.raw = answer.raw;
    }

    const auto answer = readAnswer(prompt.shape, text);
    if (!answer.ok()) {
        return fail(LlmStatus::Malformed, answer.problem);
    }
    artifact.structured = answer.json;
    artifact.content = renderAnswer(answer);

    artifact.status = ArtifactStatus::Done;
    artifact.completedAt = clock_();
    if (const auto status = artifacts_.save(artifact); status != StoreStatus::Ok) {
        return refuse("the answer could not be written down", true);
    }

    applyDigest(artifact, answer, recording, context);
    announce(artifact);

    {
        const auto chars = std::to_string(artifact.content.size());
        const auto cost = std::to_string(artifact.costMicros);
        const auto in = std::to_string(artifact.promptTokens);
        const auto out = std::to_string(artifact.answerTokens);
        const LogAttribute fields[] = {{"recording", artifact.recordingId},
                                       {"prompt", prompt.code.empty() ? prompt.name : prompt.code},
                                       {"kind", toString(artifact.kind)},
                                       {"chars", chars},
                                       {"tokens_in", in},
                                       {"tokens_out", out},
                                       {"cost_micros", cost}};
        Logger::instance().log(LogLevel::Info, kArea, "a model answered", fields);
    }

    JobOutcome outcome;
    outcome.done = true;
    outcome.costMicros = artifact.costMicros;
    return outcome;
}

void GenerationService::applyDigest(const Artifact &artifact, const Answer &answer,
                                    Recording &recording, const PromptContext &context)
{

    const auto remember = [this, &artifact](TaxonomyKind kind,
                                            const std::vector<std::string> &codes) {
        for (const auto &code : codes) {
            CodeSuggestion suggestion;
            suggestion.kind = kind;
            suggestion.code = code;
            suggestion.recordingId = artifact.recordingId;
            suggestion.artifactId = artifact.id;
            suggestion.lastAt = clock_();
            static_cast<void>(artifacts_.suggestCode(suggestion));
        }
    };

    switch (artifact.kind) {
    case ArtifactKind::SuperSummary: {

        recording.superSummary = artifact.content;
        static_cast<void>(recordings_.save(recording));
        break;
    }

    case ArtifactKind::Categorise: {
        remember(TaxonomyKind::Category, unknownCodes(answer.labels, context.categories));

        const AnswerLabel *best = nullptr;
        for (const auto &label : answer.labels) {
            const auto known = std::find_if(
                context.categories.begin(), context.categories.end(),
                [&label](const TaxonomyEntry &entry) { return entry.code == label.code; });
            if (known == context.categories.end()) {
                continue;
            }
            if (best == nullptr ||
                label.confidence.value_or(0.0) > best->confidence.value_or(0.0)) {
                best = &label;
            }
        }
        if (best == nullptr) {
            break;
        }

        recording.categoryCode = best->code;
        static_cast<void>(recordings_.save(recording));
        break;
    }

    case ArtifactKind::Tags: {
        remember(TaxonomyKind::Tag, unknownCodes(answer.labels, context.tags));

        static_cast<void>(taxonomy_.clearTagsFrom(artifact.recordingId, artifact.templateCode));
        for (const auto &label : answer.labels) {
            const auto known = std::any_of(
                context.tags.begin(), context.tags.end(),
                [&label](const TaxonomyEntry &entry) { return entry.code == label.code; });
            if (!known) {
                continue;
            }
            RecordingTag tag;
            tag.code = label.code;
            tag.source = TagSource::Llm;
            tag.artifactId = artifact.id;
            static_cast<void>(taxonomy_.setTag(artifact.recordingId, tag));
        }
        break;
    }

    case ArtifactKind::RedFlags: {
        remember(TaxonomyKind::RedFlag, unknownCodes(answer.flags, context.redFlags));

        static_cast<void>(taxonomy_.clearRedFlagsFrom(artifact.recordingId, artifact.templateCode));
        for (const auto &flag : answer.flags) {
            const auto known = std::find_if(
                context.redFlags.begin(), context.redFlags.end(),
                [&flag](const TaxonomyEntry &entry) { return entry.code == flag.code; });
            if (known == context.redFlags.end()) {
                continue;
            }

            RedFlagHit hit;
            hit.recordingId = artifact.recordingId;
            hit.artifactId = artifact.id;
            hit.code = flag.code;

            hit.severity = flag.severity;
            if (known->severity.has_value() && flag.severity == Severity::Medium) {
                hit.severity = *known->severity;
            }
            hit.evidence = flag.evidence;
            hit.atMs = flag.atMs;
            hit.createdAt = clock_();
            static_cast<void>(taxonomy_.saveRedFlag(hit));
        }
        break;
    }

    case ArtifactKind::Score: {

        if (!answer.score.has_value()) {
            break;
        }
        recording.score = *answer.score;
        static_cast<void>(recordings_.save(recording));
        break;
    }

    case ArtifactKind::Summary:
    case ArtifactKind::ActionItems:
    case ArtifactKind::Topics:
    case ArtifactKind::Entities:
    case ArtifactKind::Translation:
    case ArtifactKind::Qa:
    case ArtifactKind::Custom:

        break;

    case ArtifactKind::Realtime:

        break;
    }
}

void GenerationService::announce(const Artifact &artifact) const
{
    if (watcher_) {
        watcher_(artifact);
    }
    const LogAttribute fields[] = {{"recording", artifact.recordingId},
                                   {"kind", toString(artifact.kind)},
                                   {"status", toString(artifact.status)}};
    Logger::instance().log(LogLevel::Debug, kArea, "an artefact changed state", fields);
}

}
