// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/openai_asr.hpp>

#include <softphone/core/provider_knobs.hpp>

#include "language_codes.hpp"
#include "streaming_json.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace softphone::http {
namespace {

using namespace std::chrono_literals;

const std::vector<std::string> kFormats{"audio/mpeg", "audio/wav",  "audio/mp4",
                                        "audio/ogg",  "audio/flac", "audio/webm"};

constexpr std::uint64_t kOpenAiUploadCeiling = 25 * 1024 * 1024;

[[nodiscard]] const std::vector<core::ProviderKnob> &knobs()
{
    static const auto declared = core::knobsFor(core::ProviderKind::Asr, "openai");
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

[[nodiscard]] bool transcribes(std::string_view identifier)
{
    return identifier.find("whisper") != std::string_view::npos ||
           identifier.find("transcribe") != std::string_view::npos;
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

[[nodiscard]] double confidenceFrom(double averageLogProbability)
{
    if (!std::isfinite(averageLogProbability)) {
        return 0.0;
    }
    return std::clamp(std::exp(averageLogProbability), 0.0, 1.0);
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
    case 404:
    case 400:
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
    const auto detail = parsed.find("detail");
    if (detail != parsed.end() && detail->is_string()) {
        return detail->get<std::string>();
    }
    const auto error = parsed.find("error");
    if (error == parsed.end()) {
        return {};
    }
    if (error->is_string()) {
        return error->get<std::string>();
    }
    if (error->is_object()) {
        const auto message = error->find("message");
        if (message != error->end() && message->is_string()) {
            return message->get<std::string>();
        }
    }
    return {};
}

[[nodiscard]] core::AsrWord wordFrom(const nlohmann::json &entry)
{
    core::AsrWord word;
    word.text = streaming::readOr(entry, "word", streaming::readOr(entry, "text", std::string{}));
    word.start = secondsToMs(streaming::readOr(entry, "start", 0.0));
    word.end = secondsToMs(streaming::readOr(entry, "end", 0.0));
    word.confidence = std::clamp(streaming::readOr(entry, "probability", 0.0), 0.0, 1.0);
    return word;
}

}

std::string asLanguageCode(std::string_view spoken)
{
    std::string text;
    text.reserve(spoken.size());
    for (const auto letter : spoken) {
        text.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(letter))));
    }

    if (text.size() == 2 || text.size() == 3) {
        const auto lettersOnly = std::all_of(text.begin(), text.end(), [](char letter) {
            return std::isalpha(static_cast<unsigned char>(letter)) != 0;
        });
        if (lettersOnly) {
            return text;
        }
    }

    return iso639Alpha2(text);
}

core::AsrResult readOpenAiTranscription(std::string_view body)
{
    const auto parsed = nlohmann::json::parse(body, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return refuse(core::AsrStatus::Malformed,
                      "the answer was not the JSON object this shape describes");
    }

    core::AsrResult result;
    result.raw = std::string{body};
    result.status = core::AsrStatus::Ok;
    result.detectedLanguage = asLanguageCode(streaming::readOr(parsed, "language", std::string{}));
    result.audio = secondsToMs(streaming::readOr(parsed, "duration", 0.0));
    result.model = streaming::readOr(parsed, "model", std::string{});

    const auto segments = parsed.find("segments");
    if (segments != parsed.end() && segments->is_array()) {
        for (const auto &entry : *segments) {
            if (!entry.is_object()) {
                continue;
            }
            core::AsrSegment segment;
            segment.start = secondsToMs(streaming::readOr(entry, "start", 0.0));
            segment.end = secondsToMs(streaming::readOr(entry, "end", 0.0));
            segment.text = streaming::readOr(entry, "text", std::string{});
            segment.speaker = streaming::readOr(entry, "speaker", std::string{});
            segment.confidence = confidenceFrom(streaming::readOr(entry, "avg_logprob", 1.0));

            const auto words = entry.find("words");
            if (words != entry.end() && words->is_array()) {
                for (const auto &word : *words) {
                    if (word.is_object()) {
                        segment.words.push_back(wordFrom(word));
                    }
                }
            }
            result.segments.push_back(std::move(segment));
        }
    }

    const auto loose = parsed.find("words");
    if (loose != parsed.end() && loose->is_array() && !loose->empty()) {
        const auto nobodyHasWords =
            std::all_of(result.segments.begin(), result.segments.end(),
                        [](const core::AsrSegment &segment) { return segment.words.empty(); });
        if (nobodyHasWords) {
            for (const auto &entry : *loose) {
                if (!entry.is_object()) {
                    continue;
                }
                auto word = wordFrom(entry);
                auto owner =
                    std::find_if(result.segments.begin(), result.segments.end(),
                                 [&word](const core::AsrSegment &segment) {
                                     return word.start >= segment.start && word.start < segment.end;
                                 });
                if (owner == result.segments.end() && !result.segments.empty()) {
                    owner = std::prev(result.segments.end());
                }
                if (owner != result.segments.end()) {
                    owner->words.push_back(std::move(word));
                }
            }
        }
    }

    if (result.segments.empty()) {

        const auto text = streaming::readOr(parsed, "text", std::string{});
        if (!text.empty()) {
            core::AsrSegment whole;
            whole.start = 0ms;
            whole.end = result.audio;
            whole.text = text;
            result.segments.push_back(std::move(whole));
        }
    }

    return result;
}

OpenAiAsrProvider::OpenAiAsrProvider(core::IHttpClient &client) : client_{client} {}

core::AsrCapabilities OpenAiAsrProvider::capabilities(const core::AsrEndpoint &where) const
{
    core::AsrCapabilities can;
    can.multichannel = flagOf(where, "multichannel", false);
    can.diarisation = flagOf(where, "diarisation", false);
    can.wordTimestamps = flagOf(where, "word_timestamps", true);
    can.languageHint = true;
    can.detectsLanguage = true;
    can.formats = kFormats;
    can.maxBytes = numberOf(where, "max_bytes", kOpenAiUploadCeiling);
    can.maxDuration =
        std::chrono::seconds{static_cast<std::int64_t>(numberOf(where, "max_duration", 0))};
    can.maxConcurrent =
        static_cast<unsigned>(std::max<std::uint64_t>(numberOf(where, "max_concurrent", 1), 1));
    return can;
}

core::AsrResult OpenAiAsrProvider::transcribe(const core::AsrEndpoint &where,
                                              const core::AsrInput &input,
                                              const core::AsrOptions &options,
                                              const core::AsrCancel &cancelled)
{
    if (where.url.empty()) {
        return refuse(core::AsrStatus::Unsupported, "this provider has no address");
    }

    core::HttpRequest request;
    request.verb = core::HttpVerb::Post;
    request.url = join(where.url, "audio/transcriptions");
    request.timeout = where.timeout;
    if (!where.token.empty()) {
        request.headers.emplace_back("Authorization", "Bearer " + where.token);
    }

    core::HttpPart audio;
    audio.name = "file";
    audio.fileName = input.file.filename().string();
    audio.contentType = input.mimeType;
    audio.file = input.file;
    request.parts.push_back(std::move(audio));

    if (!where.model.empty()) {
        request.parts.push_back({"model", {}, {}, where.model, {}});
    }
    request.parts.push_back({"response_format", {}, {}, "verbose_json", {}});
    if (!options.language.empty()) {
        request.parts.push_back({"language", {}, {}, options.language, {}});
    }
    if (options.wordTimestamps) {

        request.parts.push_back({"timestamp_granularities[]", {}, {}, "segment", {}});
        request.parts.push_back({"timestamp_granularities[]", {}, {}, "word", {}});
    }
    for (const auto &[name, value] : where.params) {
        if (!ours(name)) {
            request.parts.push_back({name, {}, {}, value, {}});
        }
    }

    const auto answer = client_.send(request, cancelled);

    if (cancelled && cancelled()) {
        return refuse(core::AsrStatus::Cancelled, "cancelled");
    }

    const auto status = statusOf(answer);
    if (status != core::AsrStatus::Ok) {
        auto said = complaintIn(answer.body);
        if (said.empty()) {
            said = answer.answered() ? "the provider answered " + std::to_string(answer.status)
                                     : answer.error;
        }
        auto result = refuse(status, std::move(said));
        result.raw = answer.body;
        return result;
    }

    auto result = readOpenAiTranscription(answer.body);
    if (result.model.empty()) {
        result.model = where.model;
    }
    return result;
}

core::AsrProbe OpenAiAsrProvider::probe(const core::AsrEndpoint &where)
{
    core::AsrProbe probe;
    if (where.url.empty()) {
        probe.status = core::AsrStatus::Unsupported;
        probe.detail = "this provider has no address";
        return probe;
    }

    core::HttpRequest request;
    request.verb = core::HttpVerb::Get;
    request.url = join(where.url, "models");
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
        return probe;
    }

    std::vector<std::string> all;
    const auto parsed = nlohmann::json::parse(answer.body, nullptr, false);
    if (!parsed.is_discarded() && parsed.is_object()) {
        const auto data = parsed.find("data");
        if (data != parsed.end() && data->is_array()) {
            for (const auto &entry : *data) {
                if (entry.is_object() && entry.contains("id") && entry["id"].is_string()) {
                    all.push_back(entry["id"].get<std::string>());
                }
            }
        }
    }

    for (const auto &identifier : all) {
        if (transcribes(identifier)) {
            probe.models.push_back(identifier);
        }
    }
    if (probe.models.empty()) {
        probe.models = std::move(all);
    }
    return probe;
}

}
