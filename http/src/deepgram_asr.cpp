// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "streaming_json.hpp"

#include <softphone/http/deepgram_asr.hpp>

#include <softphone/core/provider_knobs.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
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

constexpr std::uint64_t kUploadCeiling = 2ULL * 1024 * 1024 * 1024;

[[nodiscard]] const std::vector<core::ProviderKnob> &knobs()
{
    static const auto declared = core::knobsFor(core::ProviderKind::Asr, "deepgram");
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

[[nodiscard]] std::string escaped(std::string_view text)
{
    static constexpr char kDigits[] = "0123456789ABCDEF";
    std::string out;
    for (const auto raw : text) {
        const auto letter = static_cast<unsigned char>(raw);
        const auto plain = (letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z') ||
                           (letter >= '0' && letter <= '9') || letter == '-' || letter == '_' ||
                           letter == '.' || letter == '~';
        if (plain) {
            out.push_back(static_cast<char>(letter));
        } else {
            out.push_back('%');
            out.push_back(kDigits[letter >> 4]);
            out.push_back(kDigits[letter & 0x0F]);
        }
    }
    return out;
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
    for (const auto *name : {"err_msg", "message", "reason"}) {
        const auto found = parsed.find(name);
        if (found != parsed.end() && found->is_string()) {
            return found->get<std::string>();
        }
    }
    return {};
}

[[nodiscard]] core::AsrWord wordFrom(const nlohmann::json &entry)
{
    core::AsrWord word;

    word.text = streaming::readOr(entry, "punctuated_word",
                                  streaming::readOr(entry, "word", std::string{}));
    word.start = secondsToMs(streaming::readOr(entry, "start", 0.0));
    word.end = secondsToMs(streaming::readOr(entry, "end", 0.0));
    word.confidence = std::clamp(streaming::readOr(entry, "confidence", 0.0), 0.0, 1.0);
    return word;
}

void readWords(const nlohmann::json &from, core::AsrSegment &into)
{
    const auto words = from.find("words");
    if (words == from.end() || !words->is_array()) {
        return;
    }
    for (const auto &word : *words) {
        if (word.is_object()) {
            into.words.push_back(wordFrom(word));
        }
    }
}

[[nodiscard]] std::string speakerFrom(const nlohmann::json &entry)
{
    const auto speaker = entry.find("speaker");
    if (speaker == entry.end()) {
        return {};
    }
    if (speaker->is_number_integer()) {
        return "speaker " + std::to_string(speaker->get<int>() + 1);
    }
    if (speaker->is_string()) {
        return speaker->get<std::string>();
    }
    return {};
}

}

core::AsrResult readDeepgramTranscription(std::string_view body)
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
        result.audio = secondsToMs(streaming::readOr(*metadata, "duration", 0.0));
        const auto models = metadata->find("model_info");
        if (models != metadata->end() && models->is_object() && !models->empty()) {

            const auto first = *models->begin();
            if (first.is_object()) {
                result.model = streaming::readOr(first, "name", std::string{});
            }
        }
    }

    const auto results = parsed.find("results");
    if (results == parsed.end() || !results->is_object()) {
        return refuse(core::AsrStatus::Malformed, "the answer carried no results");
    }

    const auto utterances = results->find("utterances");
    if (utterances != results->end() && utterances->is_array() && !utterances->empty()) {
        for (const auto &entry : *utterances) {
            if (!entry.is_object()) {
                continue;
            }
            core::AsrSegment segment;
            segment.start = secondsToMs(streaming::readOr(entry, "start", 0.0));
            segment.end = secondsToMs(streaming::readOr(entry, "end", 0.0));
            segment.text = streaming::readOr(entry, "transcript", std::string{});
            segment.confidence = std::clamp(streaming::readOr(entry, "confidence", 0.0), 0.0, 1.0);
            segment.speaker = speakerFrom(entry);
            if (const auto channel = entry.find("channel");
                channel != entry.end() && channel->is_number_integer()) {
                segment.channel = static_cast<unsigned>(channel->get<int>());
            }
            readWords(entry, segment);
            result.segments.push_back(std::move(segment));
        }
    } else {

        const auto channels = results->find("channels");
        if (channels == results->end() || !channels->is_array()) {
            return refuse(core::AsrStatus::Malformed, "the answer carried no channels");
        }
        unsigned index = 0;
        for (const auto &channel : *channels) {
            const auto alternatives = channel.find("alternatives");
            if (alternatives != channel.end() && alternatives->is_array() &&
                !alternatives->empty()) {
                const auto &best = alternatives->front();
                core::AsrSegment segment;
                segment.start = 0ms;
                segment.end = result.audio;
                segment.text = streaming::readOr(best, "transcript", std::string{});
                segment.confidence =
                    std::clamp(streaming::readOr(best, "confidence", 0.0), 0.0, 1.0);
                segment.channel = index;
                readWords(best, segment);
                if (!segment.words.empty()) {
                    segment.start = segment.words.front().start;
                    segment.end = segment.words.back().end;
                }
                result.segments.push_back(std::move(segment));
            }
            ++index;
        }
    }

    const auto channels = results->find("channels");
    if (channels != results->end() && channels->is_array()) {
        for (const auto &channel : *channels) {
            if (channel.is_object() && channel.contains("detected_language") &&
                channel["detected_language"].is_string()) {
                result.detectedLanguage = channel["detected_language"].get<std::string>();
                break;
            }
        }
    }

    return result;
}

DeepgramAsrProvider::DeepgramAsrProvider(core::IHttpClient &client) : client_{client} {}

core::AsrCapabilities DeepgramAsrProvider::capabilities(const core::AsrEndpoint &where) const
{
    core::AsrCapabilities can;

    can.multichannel = flagOf(where, "multichannel", true);
    can.diarisation = flagOf(where, "diarisation", true);
    can.wordTimestamps = true;
    can.languageHint = true;
    can.detectsLanguage = true;
    can.formats = kFormats;
    can.maxBytes = numberOf(where, "max_bytes", kUploadCeiling);
    can.maxDuration =
        std::chrono::seconds{static_cast<std::int64_t>(numberOf(where, "max_duration", 0))};
    can.maxConcurrent =
        static_cast<unsigned>(std::max<std::uint64_t>(numberOf(where, "max_concurrent", 2), 1));
    return can;
}

core::AsrResult DeepgramAsrProvider::transcribe(const core::AsrEndpoint &where,
                                                const core::AsrInput &input,
                                                const core::AsrOptions &options,
                                                const core::AsrCancel &cancelled)
{
    if (where.url.empty()) {
        return refuse(core::AsrStatus::Unsupported, "this provider has no address");
    }

    std::string query;
    const auto add = [&query](std::string_view name, std::string_view value) {
        query += query.empty() ? '?' : '&';
        query += escaped(name);
        query += '=';
        query += escaped(value);
    };

    if (!where.model.empty()) {
        add("model", where.model);
    }
    if (!options.language.empty()) {
        add("language", options.language);
    } else {
        add("detect_language", "true");
    }

    add("utterances", "true");
    add("smart_format", "true");
    if (input.channels > 1) {
        add("multichannel", "true");
    }
    if (options.diarise) {
        add("diarize", "true");
    }
    for (const auto &[name, value] : where.params) {
        if (!ours(name)) {
            add(name, value);
        }
    }

    core::HttpRequest request;
    request.verb = core::HttpVerb::Post;
    request.url = join(where.url, "listen") + query;
    request.timeout = where.timeout;
    request.contentType = input.mimeType;
    request.bodyFile = input.file;
    if (!where.token.empty()) {
        request.headers.emplace_back("Authorization", "Token " + where.token);
    }

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

    auto result = readDeepgramTranscription(answer.body);
    if (result.model.empty()) {
        result.model = where.model;
    }
    return result;
}

core::AsrProbe DeepgramAsrProvider::probe(const core::AsrEndpoint &where)
{
    core::AsrProbe probe;
    if (where.url.empty()) {
        probe.status = core::AsrStatus::Unsupported;
        probe.detail = "this provider has no address";
        return probe;
    }

    core::HttpRequest request;
    request.verb = core::HttpVerb::Get;
    request.url = join(where.url, "projects");
    request.timeout = std::chrono::seconds{15};
    if (!where.token.empty()) {
        request.headers.emplace_back("Authorization", "Token " + where.token);
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
