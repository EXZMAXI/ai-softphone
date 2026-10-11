// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/xai_asr.hpp>

#include <softphone/core/provider_knobs.hpp>

#include "streaming_json.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace softphone::http {
namespace {

using namespace std::chrono_literals;

const std::vector<std::string> kFormats{"audio/mpeg", "audio/wav", "audio/ogg",
                                        "audio/flac", "audio/mp4", "audio/aac"};

constexpr std::uint64_t kUploadCeiling = 500ULL * 1000 * 1000;

[[nodiscard]] const std::vector<core::ProviderKnob> &knobs()
{
    static const auto declared = core::knobsFor(core::ProviderKind::Asr, "xai");
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

[[nodiscard]] double numberIn(const nlohmann::json &object, const char *name)
{
    const auto found = object.find(name);
    return found != object.end() && found->is_number() ? found->get<double>() : 0.0;
}

[[nodiscard]] std::string textIn(const nlohmann::json &object, const char *name)
{
    const auto found = object.find(name);
    return found != object.end() && found->is_string() ? found->get<std::string>() : std::string{};
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
    if (const auto error = parsed.find("error"); error != parsed.end()) {
        if (error->is_string()) {
            return error->get<std::string>();
        }
        if (error->is_object()) {
            return textIn(*error, "message");
        }
    }
    return textIn(parsed, "detail");
}

[[nodiscard]] std::string speakerOf(const nlohmann::json &word)
{
    const auto found = word.find("speaker");
    if (found == word.end()) {
        return {};
    }
    if (found->is_number_integer()) {
        return "speaker " + std::to_string(found->get<std::int64_t>() + 1);
    }
    return found->is_string() ? found->get<std::string>() : std::string{};
}

[[nodiscard]] std::string twoLetters(std::string code)
{
    const auto cut = code.find_first_of("-_");
    if (cut != std::string::npos) {
        code.resize(cut);
    }
    std::transform(code.begin(), code.end(), code.begin(),
                   [](unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
    return code;
}

void turnsOf(const nlohmann::json &words, std::chrono::milliseconds gap,
             std::optional<unsigned> channel, std::vector<core::AsrSegment> &into)
{
    core::AsrSegment current;
    bool open = false;
    std::chrono::milliseconds last{0};
    double confidence = 0.0;
    std::size_t counted = 0;

    const auto close = [&into, &current, &open, &confidence, &counted]() {
        if (!open) {
            return;
        }
        if (counted > 0) {
            current.confidence = std::clamp(confidence / static_cast<double>(counted), 0.0, 1.0);
        }
        if (!current.text.empty()) {
            into.push_back(std::move(current));
        }
        current = core::AsrSegment{};
        open = false;
        confidence = 0.0;
        counted = 0;
    };

    for (const auto &entry : words) {
        if (!entry.is_object()) {
            continue;
        }
        auto text = textIn(entry, "text");
        if (text.empty()) {
            continue;
        }
        const auto start = secondsToMs(numberIn(entry, "start"));
        const auto end = secondsToMs(numberIn(entry, "end"));
        const auto who = speakerOf(entry);

        if (open && (who != current.speaker || (start - last) > gap)) {
            close();
        }
        if (!open) {
            current.start = start;
            current.speaker = who;
            current.channel = channel;
            open = true;
        }

        if (!current.text.empty()) {
            current.text += ' ';
        }
        current.text += text;
        current.end = end;
        last = end;

        core::AsrWord word;
        word.text = std::move(text);
        word.start = start;
        word.end = end;
        if (const auto sure = entry.find("confidence"); sure != entry.end() && sure->is_number()) {
            word.confidence = std::clamp(sure->get<double>(), 0.0, 1.0);
            confidence += word.confidence;
            ++counted;
        }
        current.words.push_back(std::move(word));
    }
    close();
}

void wholeOf(const nlohmann::json &object, std::optional<unsigned> channel,
             std::vector<core::AsrSegment> &into)
{
    auto text = textIn(object, "text");
    if (text.empty()) {
        return;
    }
    core::AsrSegment whole;
    whole.text = std::move(text);
    whole.channel = channel;
    into.push_back(std::move(whole));
}

}

core::AsrResult readXaiTranscription(std::string_view body, std::chrono::milliseconds gap)
{
    const auto parsed = nlohmann::json::parse(body, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return refuse(core::AsrStatus::Malformed,
                      "the answer was not the JSON object this shape describes");
    }

    core::AsrResult result;
    result.raw = std::string{body};
    result.status = core::AsrStatus::Ok;
    result.detectedLanguage = twoLetters(textIn(parsed, "language"));

    const auto channels = parsed.find("channels");
    if (channels != parsed.end() && channels->is_array() && !channels->empty()) {
        unsigned position = 0;
        for (const auto &side : *channels) {
            if (side.is_object()) {
                const auto index = side.find("index");
                const auto which = index != side.end() && index->is_number_unsigned()
                                       ? index->get<unsigned>()
                                       : position;
                const auto words = side.find("words");
                if (words != side.end() && words->is_array() && !words->empty()) {
                    turnsOf(*words, gap, which, result.segments);
                } else {
                    wholeOf(side, which, result.segments);
                }
                if (result.detectedLanguage.empty()) {
                    result.detectedLanguage = twoLetters(textIn(side, "language"));
                }
            }
            ++position;
        }

        std::stable_sort(result.segments.begin(), result.segments.end(),
                         [](const core::AsrSegment &one, const core::AsrSegment &other) {
                             return one.start < other.start;
                         });
    } else if (const auto words = parsed.find("words");
               words != parsed.end() && words->is_array() && !words->empty()) {
        turnsOf(*words, gap, std::nullopt, result.segments);
    } else {
        wholeOf(parsed, std::nullopt, result.segments);
    }

    if (const auto duration = numberIn(parsed, "duration"); duration > 0.0) {
        result.audio = secondsToMs(duration);
    } else {
        for (const auto &segment : result.segments) {
            result.audio = std::max(result.audio, segment.end);
        }
    }
    return result;
}

XaiAsrProvider::XaiAsrProvider(core::IHttpClient &client) : client_{client} {}

core::AsrCapabilities XaiAsrProvider::capabilities(const core::AsrEndpoint &where) const
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

core::AsrResult XaiAsrProvider::transcribe(const core::AsrEndpoint &where,
                                           const core::AsrInput &input,
                                           const core::AsrOptions &options,
                                           const core::AsrCancel &cancelled)
{
    if (where.url.empty()) {
        return refuse(core::AsrStatus::Unsupported, "this provider has no address");
    }

    core::HttpRequest request;
    request.verb = core::HttpVerb::Post;
    request.url = join(where.url, "stt");
    request.timeout = where.timeout;
    if (!where.token.empty()) {
        request.headers.emplace_back("Authorization", "Bearer " + where.token);
    }

    if (!options.language.empty()) {
        request.parts.push_back({"language", {}, {}, options.language, {}});
    }
    request.parts.push_back({"diarize", {}, {}, options.diarise ? "true" : "false", {}});
    if (input.channels > 1 && flagOf(where, "multichannel", true)) {
        request.parts.push_back({"multichannel", {}, {}, "true", {}});
    }

    if (const auto formatted = paramOf(where, "format");
        !formatted.empty() && !options.language.empty()) {
        request.parts.push_back({"format", {}, {}, formatted, {}});
    }
    for (const auto &[name, value] : where.params) {
        if (!ours(name)) {
            request.parts.push_back({name, {}, {}, value, {}});
        }
    }

    core::HttpPart audio;
    audio.name = "file";
    audio.fileName = input.file.filename().string();
    audio.contentType = input.mimeType;
    audio.file = input.file;
    request.parts.push_back(std::move(audio));

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
    auto result = readXaiTranscription(answer.body, gap);
    result.model = where.model;
    return result;
}

core::AsrProbe XaiAsrProvider::probe(const core::AsrEndpoint &where)
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
    }
    return probe;
}

}
