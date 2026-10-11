// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/elevenlabs_asr.hpp>

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

constexpr std::uint64_t kUploadCeiling = 1024ULL * 1024 * 1024;

[[nodiscard]] const std::vector<core::ProviderKnob> &knobs()
{
    static const auto declared = core::knobsFor(core::ProviderKind::Asr, "elevenlabs");
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
    const auto detail = parsed.find("detail");
    if (detail == parsed.end()) {
        return {};
    }
    if (detail->is_string()) {
        return detail->get<std::string>();
    }
    if (detail->is_object()) {
        const auto message = detail->find("message");
        if (message != detail->end() && message->is_string()) {
            return message->get<std::string>();
        }
    }
    return {};
}

[[nodiscard]] bool transcribes(std::string_view identifier)
{
    return identifier.find("scribe") != std::string_view::npos;
}

[[nodiscard]] std::string speakerFrom(const std::string &raw)
{
    constexpr std::string_view kPrefix{"speaker_"};
    if (raw.size() > kPrefix.size() && raw.compare(0, kPrefix.size(), kPrefix) == 0) {
        const auto number = raw.substr(kPrefix.size());
        try {
            return "speaker " + std::to_string(std::stoi(number) + 1);
        } catch (const std::exception &) {
            return raw;
        }
    }
    return raw;
}

}

std::string asTwoLetterCode(std::string_view code)
{
    std::string text;
    text.reserve(code.size());
    for (const auto letter : code) {
        text.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(letter))));
    }
    if (text.size() <= 2) {
        return text;
    }

    return iso639Alpha2(text);
}

core::AsrResult readElevenLabsTranscription(std::string_view body, std::chrono::milliseconds gap)
{
    const auto parsed = nlohmann::json::parse(body, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return refuse(core::AsrStatus::Malformed,
                      "the answer was not the JSON object this shape describes");
    }

    core::AsrResult result;
    result.raw = std::string{body};
    result.status = core::AsrStatus::Ok;
    result.detectedLanguage =
        asTwoLetterCode(streaming::readOr(parsed, "language_code", std::string{}));

    const auto words = parsed.find("words");
    if (words == parsed.end() || !words->is_array() || words->empty()) {

        const auto text = streaming::readOr(parsed, "text", std::string{});
        if (!text.empty()) {
            core::AsrSegment whole;
            whole.text = text;
            result.segments.push_back(std::move(whole));
        }
        return result;
    }

    core::AsrSegment current;
    bool open = false;
    std::string speaker;
    std::chrono::milliseconds last{0};

    const auto close = [&result, &current, &open]() {
        if (!open) {
            return;
        }

        const auto first = current.text.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            const auto stop = current.text.find_last_not_of(" \t\r\n");
            current.text = current.text.substr(first, stop - first + 1);
            result.segments.push_back(std::move(current));
        }
        current = core::AsrSegment{};
        open = false;
    };

    double confidence = 0.0;
    std::size_t counted = 0;

    for (const auto &entry : *words) {
        if (!entry.is_object()) {
            continue;
        }
        const auto kind = streaming::readOr(entry, "type", std::string{"word"});
        const auto text = streaming::readOr(entry, "text", std::string{});
        const auto start = secondsToMs(streaming::readOr(entry, "start", 0.0));
        const auto end = secondsToMs(streaming::readOr(entry, "end", 0.0));
        const auto who = speakerFrom(streaming::readOr(entry, "speaker_id", std::string{}));

        if (kind == "spacing") {
            if (open) {
                current.text += text;
            }
            continue;
        }
        if (kind != "word") {

            continue;
        }

        const auto turned = open && (who != speaker || (start - last) > gap);
        if (turned) {
            if (counted > 0) {
                current.confidence =
                    std::clamp(confidence / static_cast<double>(counted), 0.0, 1.0);
            }
            close();
            confidence = 0.0;
            counted = 0;
        }

        if (!open) {
            current = core::AsrSegment{};
            current.start = start;
            current.speaker = who;
            speaker = who;
            open = true;
        }

        current.text += text;
        current.end = end;
        last = end;

        core::AsrWord word;
        word.text = text;
        word.start = start;
        word.end = end;

        if (const auto logprob = entry.find("logprob");
            logprob != entry.end() && logprob->is_number()) {
            word.confidence = std::clamp(std::exp(logprob->get<double>()), 0.0, 1.0);
            confidence += word.confidence;
            ++counted;
        }
        current.words.push_back(std::move(word));
    }

    if (counted > 0) {
        current.confidence = std::clamp(confidence / static_cast<double>(counted), 0.0, 1.0);
    }
    close();

    if (!result.segments.empty()) {
        result.audio = result.segments.back().end;
    }
    return result;
}

ElevenLabsAsrProvider::ElevenLabsAsrProvider(core::IHttpClient &client) : client_{client} {}

core::AsrCapabilities ElevenLabsAsrProvider::capabilities(const core::AsrEndpoint &where) const
{
    core::AsrCapabilities can;

    can.multichannel = false;
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

core::AsrResult ElevenLabsAsrProvider::transcribe(const core::AsrEndpoint &where,
                                                  const core::AsrInput &input,
                                                  const core::AsrOptions &options,
                                                  const core::AsrCancel &cancelled)
{
    if (where.url.empty()) {
        return refuse(core::AsrStatus::Unsupported, "this provider has no address");
    }

    core::HttpRequest request;
    request.verb = core::HttpVerb::Post;
    request.url = join(where.url, "speech-to-text");
    request.timeout = where.timeout;
    if (!where.token.empty()) {

        request.headers.emplace_back("xi-api-key", where.token);
    }

    core::HttpPart audio;
    audio.name = "file";
    audio.fileName = input.file.filename().string();
    audio.contentType = input.mimeType;
    audio.file = input.file;
    request.parts.push_back(std::move(audio));

    if (!where.model.empty()) {
        request.parts.push_back({"model_id", {}, {}, where.model, {}});
    }
    request.parts.push_back({"diarize", {}, {}, options.diarise ? "true" : "false", {}});
    request.parts.push_back({"timestamps_granularity", {}, {}, "word", {}});
    if (!options.language.empty()) {
        request.parts.push_back({"language_code", {}, {}, options.language, {}});
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
    auto result = readElevenLabsTranscription(answer.body, gap);
    if (result.model.empty()) {
        result.model = where.model.empty() ? std::string{"scribe_v1"} : where.model;
    }
    return result;
}

core::AsrProbe ElevenLabsAsrProvider::probe(const core::AsrEndpoint &where)
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
        request.headers.emplace_back("xi-api-key", where.token);
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

    const auto parsed = nlohmann::json::parse(answer.body, nullptr, false);
    if (!parsed.is_discarded() && parsed.is_array()) {
        for (const auto &entry : parsed) {
            if (!entry.is_object()) {
                continue;
            }
            if (!entry.contains("model_id") || !entry["model_id"].is_string()) {
                continue;
            }
            auto identifier = entry["model_id"].get<std::string>();
            if (transcribes(identifier)) {
                probe.models.push_back(std::move(identifier));
            }
        }
    }
    return probe;
}

}
