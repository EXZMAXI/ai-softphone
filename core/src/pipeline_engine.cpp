// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/pipeline_engine.hpp>

#include <softphone/core/logging.hpp>

#include <algorithm>
#include <ctime>
#include <utility>

namespace softphone::core {
namespace {

constexpr std::string_view kArea = "pipeline";

}

std::chrono::system_clock::time_point startOfMonth(std::chrono::system_clock::time_point when)
{
    const auto days = std::chrono::floor<std::chrono::days>(when);
    const std::chrono::year_month_day date{days};
    const std::chrono::year_month_day first{date.year(), date.month(), std::chrono::day{1}};
    return std::chrono::sys_days{first};
}

PipelineEngine::PipelineEngine(IRecordingStore &recordings, ITranscriptStore &transcripts,
                               ITaxonomyStore &taxonomy, IAutomationStore &automations,
                               ITemplateStore &templates, IUsageStore &usage, WallClock clock)
    : recordings_{recordings}, transcripts_{transcripts}, taxonomy_{taxonomy},
      automations_{automations}, templates_{templates}, usage_{usage}, clock_{std::move(clock)}
{}

void PipelineEngine::setSettings(Settings settings)
{
    settings_ = std::move(settings);
    if (watcher_) {
        watcher_();
    }
}

void PipelineEngine::setActions(Transcribe transcribe, Generate generate, Busy busy)
{
    transcribe_ = std::move(transcribe);
    generate_ = std::move(generate);
    busy_ = std::move(busy);
}

void PipelineEngine::watch(Watcher watcher)
{
    watcher_ = std::move(watcher);
}

StoreStatus PipelineEngine::factsFor(const std::string &recordingId, PipelineFacts &out) const
{
    Recording recording;
    if (const auto status = recordings_.recording(recordingId, recording);
        status != StoreStatus::Ok) {
        return status;
    }

    out.recordingId = recording.id;
    out.direction = recording.direction;
    out.accountId = recording.accountId;
    out.duration = std::chrono::duration_cast<std::chrono::seconds>(recording.duration);
    out.number = recording.remoteNumber;
    out.source = recording.source;
    out.categoryCode = recording.categoryCode;
    out.language = recording.contentLanguage;

    std::vector<RecordingTag> tags;
    if (taxonomy_.tags(recordingId, tags) == StoreStatus::Ok) {
        for (const auto &tag : tags) {
            out.tags.push_back(tag.code);
        }
    }

    std::vector<RedFlagHit> flags;
    if (taxonomy_.redFlags(recordingId, flags) == StoreStatus::Ok) {
        for (const auto &flag : flags) {

            if (!flag.dismissed) {
                out.redFlags.push_back(flag.code);
            }
        }
    }

    if (out.language.empty()) {
        Transcript primary;
        if (transcripts_.primaryTranscript(recordingId, primary) == StoreStatus::Ok) {
            out.language = primary.detectedLanguage.empty() ? primary.requestedLanguage
                                                            : primary.detectedLanguage;
        }
    }

    const auto seconds = std::chrono::system_clock::to_time_t(recording.startedAt);
    std::tm broken{};
#ifdef _WIN32
    localtime_s(&broken, &seconds);
#else
    localtime_r(&seconds, &broken);
#endif
    out.hour = broken.tm_hour;

    return StoreStatus::Ok;
}

PipelineEngine::Spend PipelineEngine::spend() const
{
    Spend out;
    static_cast<void>(usage_.usageSince(startOfMonth(clock_()), out.month));

    out.byMoney = settings_.monthlyCeilingMicros > 0 &&
                  out.month.costMicros >= settings_.monthlyCeilingMicros;
    out.byTokens =
        settings_.monthlyCeilingTokens > 0 &&
        (out.month.promptTokens + out.month.answerTokens) >= settings_.monthlyCeilingTokens;
    out.paused = out.byMoney || out.byTokens;
    return out;
}

std::string PipelineEngine::templateIdFor(const std::string &code) const
{
    if (code.empty()) {
        return {};
    }
    std::vector<PromptTemplate> all;
    if (templates_.templates(all) != StoreStatus::Ok) {
        return {};
    }
    const auto found = std::find_if(all.begin(), all.end(), [&code](const PromptTemplate &one) {
        return one.code == code && one.enabled;
    });

    return found == all.end() ? std::string{} : found->id;
}

void PipelineEngine::advance(const std::string &recordingId)
{

    const auto note = [&recordingId](LogLevel level, const char *what) {
        const LogAttribute fields[] = {{"recording", recordingId}};
        Logger::instance().log(level, kArea, what, fields);
    };

    if (recordingId.empty()) {
        return;
    }
    if (!settings_.automatic) {
        note(LogLevel::Debug, "nothing runs by itself; the switch is off");
        return;
    }

    if (const auto spent = spend(); spent.paused) {
        const auto spentSoFar = std::to_string(spent.month.costMicros);
        const LogAttribute fields[] = {{"recording", recordingId}, {"cost_micros", spentSoFar}};
        Logger::instance().log(LogLevel::Info, kArea,
                               "the month's ceiling has been reached; nothing was queued", fields);
        return;
    }

    if (busy_ && busy_(recordingId)) {
        note(LogLevel::Debug, "this conversation already has work of its own; the chain waits");
        return;
    }

    std::vector<Automation> rules;
    if (automations_.automations(rules) != StoreStatus::Ok) {
        note(LogLevel::Warning, "the rules could not be read");
        return;
    }

    std::vector<AutomationRun> already;
    if (automations_.runsFor(recordingId, already) != StoreStatus::Ok) {
        note(LogLevel::Warning, "what has already run could not be read");
        return;
    }
    const auto hasFired = [&already](const std::string &automationId) {
        return std::any_of(already.begin(), already.end(), [&automationId](const AutomationRun &r) {
            return r.automationId == automationId;
        });
    };

    PipelineFacts facts;
    if (factsFor(recordingId, facts) != StoreStatus::Ok) {
        note(LogLevel::Warning, "the conversation could not be read; no rule was considered");
        return;
    }

    for (const auto &rule : rules) {

        const std::string &named = rule.code.empty() ? rule.id : rule.code;

        if (!rule.enabled) {
            continue;
        }

        if (hasFired(rule.id)) {

            if (rule.action == Action::Stop) {
                const LogAttribute fields[] = {{"recording", recordingId}, {"rule", named}};
                Logger::instance().log(LogLevel::Debug, kArea,
                                       "a rule stopped this chain earlier and it stays stopped",
                                       fields);
                return;
            }
            continue;
        }

        if (!matches(rule.conditions, facts)) {
            const LogAttribute fields[] = {{"recording", recordingId}, {"rule", named}};
            Logger::instance().log(LogLevel::Debug, kArea,
                                   "a rule's conditions do not hold yet; passed over", fields);
            continue;
        }

        if (rule.action == Action::Generate) {
            Transcript primary;
            const auto status = transcripts_.primaryTranscript(recordingId, primary);
            if (status != StoreStatus::Ok || primary.status != TranscriptStatus::Done) {
                const LogAttribute fields[] = {
                    {"recording", recordingId}, {"rule", named}, {"transcript", toString(status)}};
                Logger::instance().log(LogLevel::Debug, kArea,
                                       "there is nothing to read yet; the chain waits", fields);
                return;
            }
            if (primary.wordCount == 0) {
                const LogAttribute fields[] = {{"recording", recordingId}, {"rule", named}};
                Logger::instance().log(LogLevel::Info, kArea,
                                       "the transcript is empty; the chain stops here", fields);
                return;
            }
        }

        if (rule.action == Action::Stop) {
            const LogAttribute fields[] = {{"recording", recordingId}, {"rule", named}};
            Logger::instance().log(LogLevel::Info, kArea, "a rule stopped the chain here", fields);

            AutomationRun stop;
            stop.automationId = rule.id;
            stop.recordingId = recordingId;
            stop.firedAt = clock_();
            static_cast<void>(automations_.noteRun(stop));
            return;
        }

        AutomationRun run;
        run.automationId = rule.id;
        run.recordingId = recordingId;
        run.firedAt = clock_();
        if (const auto status = automations_.noteRun(run); status != StoreStatus::Ok) {
            continue;
        }

        {
            const LogAttribute fields[] = {{"recording", recordingId},
                                           {"rule", named},
                                           {"action", toString(rule.action)},
                                           {"prompt", rule.templateCode},
                                           {"tag", rule.tagCode}};
            Logger::instance().log(LogLevel::Info, kArea, "a rule fired", fields);
        }

        auto queued = StoreStatus::Ok;
        switch (rule.action) {
        case Action::Transcribe:
            queued =
                transcribe_ ? transcribe_(recordingId, {}, 0, rule.id) : StoreStatus::Unavailable;
            break;

        case Action::Generate: {
            const auto templateId = templateIdFor(rule.templateCode);
            if (templateId.empty()) {
                queued = StoreStatus::Missing;
                break;
            }
            queued = generate_ ? generate_(recordingId, templateId, {}, 0, rule.id)
                               : StoreStatus::Unavailable;
            break;
        }

        case Action::Tag: {
            RecordingTag tag;
            tag.code = rule.tagCode;
            tag.source = TagSource::Llm;
            queued = tag.code.empty() ? StoreStatus::Missing : taxonomy_.setTag(recordingId, tag);
            break;
        }

        case Action::Stop:
            break;
        }

        if (!succeeded(queued)) {
            const LogAttribute fields[] = {{"rule", rule.code.empty() ? rule.id : rule.code},
                                           {"recording", recordingId},
                                           {"status", toString(queued)}};
            Logger::instance().log(LogLevel::Warning, kArea, "a rule fired and nothing happened",
                                   fields);
        }

        if (rule.action == Action::Tag) {
            continue;
        }
        return;
    }

    note(LogLevel::Debug, "every rule has run or been passed over; the chain is done");
}

StoreStatus PipelineEngine::rerun(const std::string &recordingId)
{
    return automations_.forgetRuns(recordingId);
}

}
