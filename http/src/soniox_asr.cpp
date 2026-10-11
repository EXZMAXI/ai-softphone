// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/soniox_asr.hpp>

#include <softphone/core/provider_knobs.hpp>

#include "json_params.hpp"
#include "streaming_json.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace softphone::http {
namespace {

using namespace std::chrono_literals;

constexpr auto kFirstWait = 2s;
constexpr auto kLongestWait = 20s;

constexpr std::array<std::string_view, 8> kOurs{"max_bytes",
                                                "max_duration",
                                                "max_concurrent",
                                                "diarisation",
                                                "file_id",
                                                "language_hints",
                                                "enable_speaker_diarization",
                                                "model"};

[[nodiscard]] bool ours(std::string_view name)
{
    return std::ranges::find(kOurs, name) != kOurs.end();
}

[[nodiscard]] std::string paramOf(const core::AsrEndpoint &where, std::string_view name)
{
    const auto found =
        std::ranges::find_if(where.params, [name](const auto &pair) { return pair.first == name; });
    return found == where.params.end() ? std::string{} : found->second;
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
    for (const auto *name : {"error_message", "error_type", "message", "detail"}) {
        if (const auto found = parsed.find(name);
            found != parsed.end() && found->is_string() && !found->get<std::string>().empty()) {
            return found->get<std::string>();
        }
    }
    return {};
}

[[nodiscard]] std::chrono::milliseconds msAt(const nlohmann::json &from, std::string_view name)
{
    const auto found = from.find(name);
    if (found == from.end() || !found->is_number()) {
        return 0ms;
    }
    return std::chrono::milliseconds{found->get<std::int64_t>()};
}

}

core::AsrResult readSonioxTranscription(std::string_view body)
{
    const auto parsed = nlohmann::json::parse(body, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return refuse(core::AsrStatus::Malformed, "the answer was not an object");
    }

    core::AsrResult result;
    result.raw = std::string{body};

    const auto tokens = parsed.find("tokens");
    if (tokens == parsed.end() || !tokens->is_array() || tokens->empty()) {

        const auto text = streaming::readOr(parsed, "text", std::string{});
        if (text.empty()) {
            return refuse(core::AsrStatus::Malformed, "the answer carried no transcript");
        }
        core::AsrSegment whole;
        whole.text = text;
        result.segments.push_back(std::move(whole));
        return result;
    }

    core::AsrSegment current;
    bool open = false;

    for (const auto &one : *tokens) {
        if (!one.is_object()) {
            continue;
        }
        if (streaming::readOr(one, "translation_status", std::string{}) == "translation") {
            continue;
        }
        const auto text = streaming::readOr(one, "text", std::string{});
        if (text.empty()) {
            continue;
        }

        core::AsrWord word;
        word.text = text;
        word.start = msAt(one, "start_ms");
        word.end = msAt(one, "end_ms");
        word.confidence = streaming::readOr(one, "confidence", 0.0);

        const auto speaker = streaming::readOr(one, "speaker", std::string{});
        const auto apart = open && !current.words.empty() &&
                           word.start > current.words.back().end + kSonioxTurnGap;

        if (open && (speaker != current.speaker || apart)) {
            result.segments.push_back(std::move(current));
            current = core::AsrSegment{};
            open = false;
        }

        if (!open) {
            current.speaker = speaker;
            current.start = word.start;
            open = true;
        }
        if (result.detectedLanguage.empty()) {
            result.detectedLanguage = streaming::readOr(one, "language", std::string{});
        }

        current.text += text;
        current.end = word.end;
        current.words.push_back(std::move(word));
    }

    if (open) {
        result.segments.push_back(std::move(current));
    }
    if (result.segments.empty()) {
        return refuse(core::AsrStatus::Malformed, "the answer carried no transcript");
    }

    for (auto &segment : result.segments) {
        if (segment.words.empty()) {
            continue;
        }
        double inside = 0.0;
        for (const auto &word : segment.words) {
            inside += word.confidence;
        }
        segment.confidence = inside / static_cast<double>(segment.words.size());
    }

    result.audio = result.segments.back().end;
    return result;
}

SonioxAsrProvider::SonioxAsrProvider(core::IHttpClient &client) : client_{client} {}

core::AsrCapabilities SonioxAsrProvider::capabilities(const core::AsrEndpoint &where) const
{
    core::AsrCapabilities can;
    can.multichannel = flagOf(where, "multichannel", false);
    can.diarisation = flagOf(where, "diarisation", true);
    can.wordTimestamps = true;
    can.languageHint = true;
    can.detectsLanguage = true;
    can.formats = {"audio/mpeg", "audio/wav",  "audio/x-wav", "audio/mp4",
                   "audio/aac",  "audio/flac", "audio/ogg",   "audio/webm"};
    can.maxBytes = numberOf(where, "max_bytes", 0);
    can.maxDuration =
        std::chrono::seconds{static_cast<std::int64_t>(numberOf(where, "max_duration", 0))};
    can.maxConcurrent =
        static_cast<unsigned>(std::max<std::uint64_t>(numberOf(where, "max_concurrent", 4), 1));
    return can;
}

core::AsrResult SonioxAsrProvider::transcribe(const core::AsrEndpoint &where,
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

    const auto refusedBy = [](const core::HttpResponse &answer, core::AsrStatus status) {
        auto said = complaintIn(answer.body);
        if (said.empty()) {
            said = answer.answered() ? "the provider answered " + std::to_string(answer.status)
                                     : answer.error;
        }
        auto result = refuse(status, std::move(said));
        result.raw = answer.body;
        return result;
    };

    std::string fileId;
    {
        core::HttpRequest request;
        request.verb = core::HttpVerb::Post;
        request.url = join(where.url, "v1/files");
        request.timeout = where.timeout;
        authorise(request);

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
            return refusedBy(answer, status);
        }

        const auto parsed = nlohmann::json::parse(answer.body, nullptr, false);
        if (parsed.is_discarded() || !parsed.is_object() || !parsed.contains("id") ||
            !parsed["id"].is_string()) {
            return refuse(core::AsrStatus::Malformed, "the upload answered no identifier");
        }
        fileId = parsed["id"].get<std::string>();
    }

    std::string jobId;
    {
        nlohmann::json asking;
        asking["file_id"] = fileId;

        if (!where.model.empty()) {
            asking["model"] = where.model;
        }
        if (!options.language.empty()) {
            asking["language_hints"] = nlohmann::json::array({options.language});
        } else {
            asking["enable_language_identification"] = true;
        }
        if (options.diarise) {
            asking["enable_speaker_diarization"] = true;
        }
        for (const auto &[name, value] : where.params) {
            if (!ours(name)) {
                asking[name] = jsonValueOf(value);
            }
        }

        core::HttpRequest request;
        request.verb = core::HttpVerb::Post;
        request.url = join(where.url, "v1/transcriptions");
        request.body = asking.dump();
        request.timeout = std::chrono::seconds{60};
        authorise(request);

        const auto answer = client_.send(request, cancelled);
        if (cancelled && cancelled()) {
            return refuse(core::AsrStatus::Cancelled, "cancelled");
        }
        if (const auto status = statusOf(answer); status != core::AsrStatus::Ok) {
            return refusedBy(answer, status);
        }

        const auto parsed = nlohmann::json::parse(answer.body, nullptr, false);
        if (parsed.is_discarded() || !parsed.is_object() || !parsed.contains("id") ||
            !parsed["id"].is_string()) {
            return refuse(core::AsrStatus::Malformed, "the job was accepted without an identifier");
        }
        jobId = parsed["id"].get<std::string>();
    }

    auto waited = 0ms;
    auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(kFirstWait);
    const auto limit = std::chrono::duration_cast<std::chrono::milliseconds>(where.timeout);

    for (;;) {
        if (cancelled && cancelled()) {
            return refuse(core::AsrStatus::Cancelled, "cancelled");
        }

        core::HttpRequest request;
        request.verb = core::HttpVerb::Get;
        request.url = join(where.url, "v1/transcriptions/" + jobId);
        request.timeout = std::chrono::seconds{30};
        authorise(request);

        const auto answer = client_.send(request, cancelled);
        if (const auto status = statusOf(answer); status != core::AsrStatus::Ok) {
            return refusedBy(answer, status);
        }

        const auto parsed = nlohmann::json::parse(answer.body, nullptr, false);
        const auto state =
            parsed.is_object() ? streaming::readOr(parsed, "status", std::string{}) : std::string{};

        if (state == "completed") {
            break;
        }
        if (state == "error") {
            return refusedBy(answer, core::AsrStatus::Failed);
        }
        if (waited >= limit) {
            return refuse(core::AsrStatus::Timeout, "the provider did not finish in time");
        }

        std::this_thread::sleep_for(std::min(wait, limit - waited));
        waited += wait;
        wait =
            std::min(wait * 2, std::chrono::duration_cast<std::chrono::milliseconds>(kLongestWait));
    }

    {
        core::HttpRequest request;
        request.verb = core::HttpVerb::Get;
        request.url = join(where.url, "v1/transcriptions/" + jobId + "/transcript");
        request.timeout = std::chrono::seconds{60};
        authorise(request);

        const auto answer = client_.send(request, cancelled);
        if (cancelled && cancelled()) {
            return refuse(core::AsrStatus::Cancelled, "cancelled");
        }
        if (const auto status = statusOf(answer); status != core::AsrStatus::Ok) {
            return refusedBy(answer, status);
        }

        auto result = readSonioxTranscription(answer.body);
        if (result.model.empty()) {
            result.model = where.model;
        }
        return result;
    }
}

core::AsrProbe SonioxAsrProvider::probe(const core::AsrEndpoint &where)
{
    core::AsrProbe probe;
    if (where.url.empty()) {
        probe.status = core::AsrStatus::Unsupported;
        probe.detail = "this provider has no address";
        return probe;
    }

    core::HttpRequest request;
    request.verb = core::HttpVerb::Get;
    request.url = join(where.url, "v1/models");
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

    const auto parsed = nlohmann::json::parse(answer.body, nullptr, false);
    if (parsed.is_object()) {
        if (const auto found = parsed.find("models"); found != parsed.end() && found->is_array()) {
            for (const auto &one : *found) {
                if (one.is_object()) {
                    if (const auto id = streaming::readOr(one, "id", std::string{}); !id.empty()) {
                        probe.models.push_back(id);
                    }
                } else if (one.is_string()) {
                    probe.models.push_back(one.get<std::string>());
                }
            }
        }
    }
    return probe;
}

}
