// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/speechmatics_asr.hpp>

#include <softphone/core/provider_knobs.hpp>

#include "json_params.hpp"
#include "streaming_json.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace softphone::http {
namespace {

using namespace std::chrono_literals;

const std::vector<std::string> kFormats{"audio/mpeg", "audio/wav",  "audio/mp4",
                                        "audio/ogg",  "audio/flac", "audio/webm"};

constexpr std::uint64_t kUploadCeiling = 4ULL * 1024 * 1024 * 1024;

constexpr auto kFirstWait = 2s;
constexpr auto kLongestWait = 15s;

[[nodiscard]] const std::vector<core::ProviderKnob> &knobs()
{
    static const auto declared = core::knobsFor(core::ProviderKind::Asr, "speechmatics");
    return declared;
}

[[nodiscard]] bool ours(std::string_view name)
{
    return core::isKnob(knobs(), name);
}

[[nodiscard]] std::string paramOf(const core::AsrEndpoint &where, std::string_view name)
{
    return core::knobValue(where.params, knobs(), name);
}

[[nodiscard]] bool flagOf(const core::AsrEndpoint &where, std::string_view name, bool fallback)
{
    const auto text = paramOf(where, name);
    if (text.empty()) {
        return fallback;
    }
    return text == "true" || text == "1" || text == "yes";
}

[[nodiscard]] std::uint64_t numberOf(const core::AsrEndpoint &where, std::string_view name,
                                     std::uint64_t fallback)
{
    const auto text = paramOf(where, name);
    if (text.empty()) {
        return fallback;
    }
    try {
        return static_cast<std::uint64_t>(std::stoull(text));
    } catch (const std::exception &) {
        return fallback;
    }
}

[[nodiscard]] std::string join(std::string_view base, std::string_view leaf)
{
    std::string address{base};
    while (!address.empty() && address.back() == '/') {
        address.pop_back();
    }
    return address + '/' + std::string{leaf};
}

[[nodiscard]] std::chrono::milliseconds secondsToMs(double seconds)
{
    if (!std::isfinite(seconds) || seconds < 0.0) {
        return 0ms;
    }
    return std::chrono::milliseconds{static_cast<std::int64_t>(std::llround(seconds * 1000.0))};
}

[[nodiscard]] core::AsrResult refuse(core::AsrStatus status, std::string error)
{
    core::AsrResult result;
    result.status = status;
    result.error = std::move(error);
    return result;
}

[[nodiscard]] core::AsrStatus statusOf(const core::HttpResponse &answer)
{
    if (!answer.answered()) {
        const auto isDeadline = answer.error.find("imeout") != std::string::npos ||
                                answer.error.find("Read") != std::string::npos ||
                                answer.error.find("Write") != std::string::npos;
        return isDeadline ? core::AsrStatus::Timeout : core::AsrStatus::Unavailable;
    }
    if (answer.ok()) {
        return core::AsrStatus::Ok;
    }
    switch (answer.status) {
    case 401:
    case 403:
        return core::AsrStatus::Unauthorised;
    case 400:
    case 404:
    case 422:
        return core::AsrStatus::Unsupported;
    case 413:
        return core::AsrStatus::TooLarge;
    case 408:
        return core::AsrStatus::Timeout;
    case 429:

        return streaming::outOfCredit(answer.body) ? core::AsrStatus::Unauthorised
                                                   : core::AsrStatus::Unavailable;
    default:
        break;
    }
    return answer.status >= 500 ? core::AsrStatus::Unavailable : core::AsrStatus::Failed;
}

[[nodiscard]] std::string complaintIn(const std::string &body)
{
    const auto parsed = nlohmann::json::parse(body, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return {};
    }
    std::string said;
    for (const auto *name : {"error", "detail", "message"}) {
        const auto found = parsed.find(name);
        if (found != parsed.end() && found->is_string()) {
            if (!said.empty()) {
                said += ": ";
            }
            said += found->get<std::string>();
        }
    }
    return said;
}

[[nodiscard]] std::optional<unsigned> channelFrom(const std::string &name)
{
    constexpr std::string_view kPrefix{"channel_"};
    if (name.size() <= kPrefix.size() || name.compare(0, kPrefix.size(), kPrefix) != 0) {
        return std::nullopt;
    }
    try {
        const auto number = std::stoi(name.substr(kPrefix.size()));
        return number > 0 ? std::optional<unsigned>{static_cast<unsigned>(number - 1)}
                          : std::nullopt;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

}

core::AsrResult readSpeechmaticsTranscription(std::string_view body, std::chrono::milliseconds gap)
{
    const auto parsed = nlohmann::json::parse(body, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return refuse(core::AsrStatus::Malformed,
                      "the answer was not the JSON object this shape describes");
    }

    core::AsrResult result;
    result.raw = std::string{body};
    result.status = core::AsrStatus::Ok;

    const auto metadata = parsed.find("metadata");
    if (metadata != parsed.end() && metadata->is_object()) {
        const auto config = metadata->find("transcription_config");
        if (config != metadata->end() && config->is_object()) {
            result.detectedLanguage = streaming::readOr(*config, "language", std::string{});
            result.model = streaming::readOr(*config, "operating_point", std::string{});
        }
    }

    const auto results = parsed.find("results");
    if (results == parsed.end() || !results->is_array()) {
        return refuse(core::AsrStatus::Malformed, "the answer carried no results");
    }

    core::AsrSegment current;
    bool open = false;
    std::string speaker;
    std::optional<unsigned> channel;
    std::chrono::milliseconds last{0};
    double confidence = 0.0;
    std::size_t counted = 0;

    const auto close = [&result, &current, &open, &confidence, &counted]() {
        if (!open) {
            return;
        }
        if (counted > 0) {
            current.confidence = std::clamp(confidence / static_cast<double>(counted), 0.0, 1.0);
        }
        if (!current.text.empty()) {
            result.segments.push_back(std::move(current));
        }
        current = core::AsrSegment{};
        open = false;
        confidence = 0.0;
        counted = 0;
    };

    for (const auto &entry : *results) {
        if (!entry.is_object()) {
            continue;
        }
        const auto kind = streaming::readOr(entry, "type", std::string{});
        const auto alternatives = entry.find("alternatives");
        if (alternatives == entry.end() || !alternatives->is_array() || alternatives->empty()) {
            continue;
        }
        const auto &best = alternatives->front();
        const auto content = streaming::readOr(best, "content", std::string{});
        if (content.empty()) {
            continue;
        }

        const auto start = secondsToMs(streaming::readOr(entry, "start_time", 0.0));
        const auto end = secondsToMs(streaming::readOr(entry, "end_time", 0.0));
        const auto who = streaming::readOr(best, "speaker", std::string{});
        const auto where = channelFrom(streaming::readOr(entry, "channel", std::string{}));

        if (kind == "punctuation") {

            if (open) {
                current.text += content;
                current.end = std::max(current.end, end);
                if (!current.words.empty()) {
                    current.words.back().text += content;
                    current.words.back().end = std::max(current.words.back().end, end);
                }
            }
            continue;
        }
        if (kind != "word") {
            continue;
        }

        const auto turned = open && (who != speaker || where != channel || (start - last) > gap);
        if (turned) {
            close();
        }

        if (!open) {
            current = core::AsrSegment{};
            current.start = start;
            current.speaker = who;
            current.channel = where;
            speaker = who;
            channel = where;
            open = true;
        } else {
            current.text += ' ';
        }

        current.text += content;
        current.end = end;
        last = end;

        core::AsrWord word;
        word.text = content;
        word.start = start;
        word.end = end;
        word.confidence = std::clamp(streaming::readOr(best, "confidence", 0.0), 0.0, 1.0);
        confidence += word.confidence;
        ++counted;
        current.words.push_back(std::move(word));
    }

    close();

    if (!result.segments.empty()) {
        result.audio = result.segments.back().end;
    }
    return result;
}

SpeechmaticsAsrProvider::SpeechmaticsAsrProvider(core::IHttpClient &client, Sleeper sleep)
    : client_{client}, sleep_{sleep ? std::move(sleep) : Sleeper{[](std::chrono::milliseconds how) {
          std::this_thread::sleep_for(how);
      }}}
{}

core::AsrCapabilities SpeechmaticsAsrProvider::capabilities(const core::AsrEndpoint &where) const
{
    core::AsrCapabilities can;

    can.multichannel = false;
    can.diarisation = flagOf(where, "diarisation", true);
    can.wordTimestamps = true;
    can.languageHint = true;
    can.detectsLanguage = false;
    can.formats = kFormats;
    can.maxBytes = numberOf(where, "max_bytes", kUploadCeiling);
    can.maxDuration =
        std::chrono::seconds{static_cast<std::int64_t>(numberOf(where, "max_duration", 0))};
    can.maxConcurrent =
        static_cast<unsigned>(std::max<std::uint64_t>(numberOf(where, "max_concurrent", 2), 1));
    return can;
}

core::AsrResult SpeechmaticsAsrProvider::transcribe(const core::AsrEndpoint &where,
                                                    const core::AsrInput &input,
                                                    const core::AsrOptions &options,
                                                    const core::AsrCancel &cancelled)
{
    if (where.url.empty()) {
        return refuse(core::AsrStatus::Unsupported, "this provider has no address");
    }

    const auto authorise = [&where](core::HttpRequest &request) {
        if (!where.token.empty()) {
            request.headers.emplace_back("Authorization", "Bearer " + where.token);
        }
    };

    std::string jobId;
    {
        nlohmann::json config;
        config["type"] = "transcription";
        auto &transcription = config["transcription_config"];

        transcription["language"] =
            options.language.empty() ? std::string{"auto"} : options.language;

        if (const auto point = paramOf(where, "operating_point"); !point.empty()) {
            transcription["operating_point"] = point;
        } else if (!where.model.empty()) {
            transcription["model"] = where.model;
        }
        if (options.diarise) {
            transcription["diarization"] = "speaker";
        }

        for (const auto &[name, value] : where.params) {
            if (!ours(name)) {
                transcription[name] = jsonValueOf(value);
            }
        }

        core::HttpRequest request;
        request.verb = core::HttpVerb::Post;
        request.url = join(where.url, "jobs");
        request.timeout = where.timeout;
        authorise(request);

        core::HttpPart audio;
        audio.name = "data_file";
        audio.fileName = input.file.filename().string();
        audio.contentType = input.mimeType;
        audio.file = input.file;
        request.parts.push_back(std::move(audio));
        request.parts.push_back({"config", {}, {}, config.dump(), {}});

        const auto answer = client_.send(request, cancelled);
        if (cancelled && cancelled()) {
            return refuse(core::AsrStatus::Cancelled, "cancelled");
        }
        if (const auto status = statusOf(answer); status != core::AsrStatus::Ok) {
            auto said = complaintIn(answer.body);
            if (said.empty()) {
                said = answer.answered() ? "the provider answered " + std::to_string(answer.status)
                                         : answer.error;
            }
            auto result = refuse(status, std::move(said));
            result.raw = answer.body;
            return result;
        }

        const auto parsed = nlohmann::json::parse(answer.body, nullptr, false);
        if (parsed.is_discarded() || !parsed.is_object() || !parsed.contains("id") ||
            !parsed["id"].is_string()) {
            return refuse(core::AsrStatus::Malformed, "the job was accepted without an identifier");
        }
        jobId = parsed["id"].get<std::string>();
    }

    {
        auto waited = 0ms;
        auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(kFirstWait);
        const auto limit = std::chrono::duration_cast<std::chrono::milliseconds>(where.timeout);

        for (;;) {
            if (cancelled && cancelled()) {
                return refuse(core::AsrStatus::Cancelled, "cancelled");
            }

            core::HttpRequest request;
            request.verb = core::HttpVerb::Get;
            request.url = join(where.url, "jobs/" + jobId);
            request.timeout = std::chrono::seconds{30};
            authorise(request);

            const auto answer = client_.send(request, cancelled);
            if (const auto status = statusOf(answer); status != core::AsrStatus::Ok) {
                auto said = complaintIn(answer.body);
                if (said.empty()) {
                    said = answer.answered()
                               ? "the provider answered " + std::to_string(answer.status)
                               : answer.error;
                }
                return refuse(status, std::move(said));
            }

            const auto parsed = nlohmann::json::parse(answer.body, nullptr, false);
            const auto job = parsed.is_object() ? parsed.find("job") : parsed.end();
            const auto state = job != parsed.end() && job->is_object()
                                   ? streaming::readOr(*job, "status", std::string{})
                                   : std::string{};

            if (state == "done") {
                break;
            }
            if (state == "rejected" || state == "expired" || state == "deleted") {
                auto said = streaming::readOr(*job, "errors", nlohmann::json::array()).dump();
                if (said == "[]" || said.empty()) {
                    said = "the provider " + state + " the job";
                }

                return refuse(core::AsrStatus::Unsupported, std::move(said));
            }
            if (state.empty()) {
                return refuse(core::AsrStatus::Malformed, "the job said nothing about itself");
            }

            if (waited >= limit) {
                return refuse(core::AsrStatus::Timeout, "the job was still running when the "
                                                        "deadline passed");
            }
            sleep_(wait);
            waited += wait;
            wait = std::min(wait * 2,
                            std::chrono::duration_cast<std::chrono::milliseconds>(kLongestWait));
        }
    }

    core::HttpRequest request;
    request.verb = core::HttpVerb::Get;
    request.url = join(where.url, "jobs/" + jobId + "/transcript") + "?format=json-v2";
    request.timeout = where.timeout;
    authorise(request);

    const auto answer = client_.send(request, cancelled);
    if (cancelled && cancelled()) {
        return refuse(core::AsrStatus::Cancelled, "cancelled");
    }
    if (const auto status = statusOf(answer); status != core::AsrStatus::Ok) {
        auto said = complaintIn(answer.body);
        if (said.empty()) {
            said = answer.answered() ? "the provider answered " + std::to_string(answer.status)
                                     : answer.error;
        }
        auto result = refuse(status, std::move(said));
        result.raw = answer.body;
        return result;
    }

    const auto gap =
        std::chrono::milliseconds{static_cast<std::int64_t>(numberOf(where, "turn_gap_ms", 1500))};
    auto result = readSpeechmaticsTranscription(answer.body, gap);
    if (result.detectedLanguage.empty()) {
        result.detectedLanguage = options.language;
    }
    if (result.model.empty()) {
        result.model = where.model;
    }
    return result;
}

core::AsrProbe SpeechmaticsAsrProvider::probe(const core::AsrEndpoint &where)
{
    core::AsrProbe probe;
    if (where.url.empty()) {
        probe.status = core::AsrStatus::Unsupported;
        probe.detail = "this provider has no address";
        return probe;
    }

    core::HttpRequest request;
    request.verb = core::HttpVerb::Get;
    request.url = join(where.url, "jobs") + "?limit=1";
    request.timeout = std::chrono::seconds{15};
    if (!where.token.empty()) {
        request.headers.emplace_back("Authorization", "Bearer " + where.token);
    }

    const auto answer = client_.send(request, {});
    probe.status = statusOf(answer);
    probe.reachable = probe.status == core::AsrStatus::Ok;
    if (!probe.reachable) {
        probe.detail = complaintIn(answer.body);
        if (probe.detail.empty()) {
            probe.detail = answer.answered()
                               ? "the provider answered " + std::to_string(answer.status)
                               : answer.error;
        }
    }
    return probe;
}

}
