// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/yandex_asr.hpp>

#include <softphone/core/provider_knobs.hpp>

#include "json_params.hpp"
#include "streaming_json.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace softphone::http {
namespace {

using namespace std::chrono_literals;

constexpr auto kFirstWait = 1s;
constexpr auto kLongestWait = 10s;

constexpr std::uint64_t kUploadCeiling = 45ULL * 1000 * 1000;

constexpr std::uint64_t kLongestRecording = 4ULL * 60 * 60;

const std::vector<std::string> kFormats{"audio/mpeg", "audio/wav", "audio/ogg"};

[[nodiscard]] const std::vector<core::ProviderKnob> &knobs()
{
    static const auto declared = core::knobsFor(core::ProviderKind::Asr, "yandex");
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
    if (const auto message = parsed.find("message");
        message != parsed.end() && message->is_string()) {
        return message->get<std::string>();
    }
    if (const auto error = parsed.find("error"); error != parsed.end() && error->is_object()) {
        const auto message = error->find("message");
        if (message != error->end() && message->is_string()) {
            return message->get<std::string>();
        }
    }
    return {};
}

[[nodiscard]] std::string saidBy(const core::HttpResponse &answer)
{
    auto said = complaintIn(answer.body);
    if (said.empty()) {
        said = answer.answered() ? "the provider answered " + std::to_string(answer.status)
                                 : answer.error;
    }
    return said;
}

void authorise(core::HttpRequest &request, const core::AsrEndpoint &where)
{
    if (!where.token.empty()) {
        const bool iam = where.token.rfind("t1.", 0) == 0;
        request.headers.emplace_back("Authorization", (iam ? "Bearer " : "Api-Key ") + where.token);
    }
    if (const auto folder = paramOf(where, "folder_id"); !folder.empty()) {
        request.headers.emplace_back("x-folder-id", folder);
    }
    request.headers.emplace_back("x-data-logging-enabled", "false");
}

[[nodiscard]] std::int64_t integerIn(const nlohmann::json &object, const char *name)
{
    const auto found = object.find(name);
    if (found == object.end()) {
        return 0;
    }
    if (found->is_number_integer()) {
        return found->get<std::int64_t>();
    }
    if (found->is_number()) {
        return static_cast<std::int64_t>(found->get<double>());
    }
    if (found->is_string()) {
        try {
            return std::stoll(found->get<std::string>());
        } catch (const std::exception &) {
            return 0;
        }
    }
    return 0;
}

[[nodiscard]] std::string textIn(const nlohmann::json &object, const char *name)
{
    const auto found = object.find(name);
    return found != object.end() && found->is_string() ? found->get<std::string>() : std::string{};
}

[[nodiscard]] const nlohmann::json *firstAlternative(const nlohmann::json &update)
{
    const auto alternatives = update.find("alternatives");
    if (alternatives == update.end() || !alternatives->is_array() || alternatives->empty() ||
        !alternatives->front().is_object()) {
        return nullptr;
    }
    return &alternatives->front();
}

[[nodiscard]] std::string twoLettersOf(std::string code)
{
    const auto cut = code.find_first_of("-_");
    if (cut != std::string::npos) {
        code.resize(cut);
    }
    std::transform(code.begin(), code.end(), code.begin(),
                   [](unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
    return code == "auto" ? std::string{} : code;
}

[[nodiscard]] std::vector<nlohmann::json> objectsIn(std::string_view body)
{
    std::vector<nlohmann::json> found;
    const auto whole = nlohmann::json::parse(body, nullptr, false);
    if (!whole.is_discarded()) {
        if (whole.is_array()) {
            for (const auto &entry : whole) {
                found.push_back(entry);
            }
        } else {
            found.push_back(whole);
        }
        return found;
    }

    std::istringstream lines{std::string{body}};
    for (std::string line; std::getline(lines, line);) {
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }
        auto parsed = nlohmann::json::parse(line, nullptr, false);
        if (!parsed.is_discarded()) {
            found.push_back(std::move(parsed));
        }
    }
    return found;
}

[[nodiscard]] std::string containerFor(std::string_view mimeType)
{
    if (mimeType.find("wav") != std::string_view::npos) {
        return "WAV";
    }
    if (mimeType.find("ogg") != std::string_view::npos ||
        mimeType.find("opus") != std::string_view::npos) {
        return "OGG_OPUS";
    }
    return "MP3";
}

}

std::string base64Of(std::string_view bytes)
{
    static constexpr std::string_view kAlphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const auto byteAt = [&bytes](std::size_t at) -> std::uint32_t {
        return static_cast<unsigned char>(bytes[at]);
    };
    const auto letter = [](std::uint32_t group, unsigned shift) {
        return kAlphabet[(group >> shift) & 0x3FU];
    };

    std::string encoded;
    encoded.reserve(((bytes.size() + 2) / 3) * 4);
    std::size_t at = 0;
    for (; at + 3 <= bytes.size(); at += 3) {
        const std::uint32_t group = (byteAt(at) << 16U) | (byteAt(at + 1) << 8U) | byteAt(at + 2);
        encoded.push_back(letter(group, 18U));
        encoded.push_back(letter(group, 12U));
        encoded.push_back(letter(group, 6U));
        encoded.push_back(letter(group, 0U));
    }
    if (const auto left = bytes.size() - at; left == 1) {
        const std::uint32_t group = byteAt(at) << 16U;
        encoded.push_back(letter(group, 18U));
        encoded.push_back(letter(group, 12U));
        encoded += "==";
    } else if (left == 2) {
        const std::uint32_t group = (byteAt(at) << 16U) | (byteAt(at + 1) << 8U);
        encoded.push_back(letter(group, 18U));
        encoded.push_back(letter(group, 12U));
        encoded.push_back(letter(group, 6U));
        encoded.push_back('=');
    }
    return encoded;
}

std::string yandexLanguageFor(std::string_view twoLetters)
{

    static constexpr std::array<std::pair<std::string_view, std::string_view>, 15> kKnown{{
        {"ru", "ru-RU"},
        {"en", "en-US"},
        {"kk", "kk-KZ"},
        {"uz", "uz-UZ"},
        {"tr", "tr-TR"},
        {"de", "de-DE"},
        {"es", "es-ES"},
        {"fi", "fi-FI"},
        {"fr", "fr-FR"},
        {"he", "he-IL"},
        {"it", "it-IT"},
        {"nl", "nl-NL"},
        {"pl", "pl-PL"},
        {"pt", "pt-PT"},
        {"sv", "sv-SE"},
    }};
    for (const auto &[two, spelled] : kKnown) {
        if (two == twoLetters) {
            return std::string{spelled};
        }
    }
    return {};
}

core::AsrResult readYandexTranscription(std::string_view body)
{
    core::AsrResult result;
    result.raw = std::string{body};
    result.status = core::AsrStatus::Ok;

    std::map<std::int64_t, core::AsrSegment> finals;
    std::map<std::int64_t, std::string> refined;
    std::int64_t next = 0;

    for (const auto &entry : objectsIn(body)) {
        if (!entry.is_object()) {
            continue;
        }
        const auto wrapped = entry.find("result");
        const auto &update = wrapped != entry.end() && wrapped->is_object() ? *wrapped : entry;

        if (const auto final = update.find("final"); final != update.end() && final->is_object()) {
            const auto *alternative = firstAlternative(*final);
            if (alternative == nullptr) {
                continue;
            }
            std::int64_t index = next++;
            if (const auto cursors = update.find("audioCursors"); cursors != update.end() &&
                                                                  cursors->is_object() &&
                                                                  cursors->contains("finalIndex")) {
                index = integerIn(*cursors, "finalIndex");
            }

            core::AsrSegment segment;
            segment.text = textIn(*alternative, "text");
            segment.start = std::chrono::milliseconds{integerIn(*alternative, "startTimeMs")};
            segment.end = std::chrono::milliseconds{integerIn(*alternative, "endTimeMs")};
            if (const auto sure = alternative->find("confidence");
                sure != alternative->end() && sure->is_number()) {
                segment.confidence = std::clamp(sure->get<double>(), 0.0, 1.0);
            }
            if (const auto words = alternative->find("words");
                words != alternative->end() && words->is_array()) {
                for (const auto &spoken : *words) {
                    if (!spoken.is_object()) {
                        continue;
                    }
                    core::AsrWord word;
                    word.text = textIn(spoken, "text");
                    word.start = std::chrono::milliseconds{integerIn(spoken, "startTimeMs")};
                    word.end = std::chrono::milliseconds{integerIn(spoken, "endTimeMs")};
                    if (!word.text.empty()) {
                        segment.words.push_back(std::move(word));
                    }
                }
            }
            if (result.detectedLanguage.empty()) {
                if (const auto languages = alternative->find("languages");
                    languages != alternative->end() && languages->is_array() &&
                    !languages->empty() && languages->front().is_object()) {
                    result.detectedLanguage =
                        twoLettersOf(textIn(languages->front(), "languageCode"));
                }
            }
            if (!segment.text.empty()) {
                finals[index] = std::move(segment);
            }
            continue;
        }

        if (const auto refinement = update.find("finalRefinement");
            refinement != update.end() && refinement->is_object()) {
            const auto normalised = refinement->find("normalizedText");
            if (normalised == refinement->end() || !normalised->is_object()) {
                continue;
            }
            const auto *alternative = firstAlternative(*normalised);
            if (alternative != nullptr) {
                refined[integerIn(*refinement, "finalIndex")] = textIn(*alternative, "text");
            }
        }
    }

    for (auto &[index, segment] : finals) {
        if (const auto better = refined.find(index);
            better != refined.end() && !better->second.empty()) {
            segment.text = better->second;
        }
        result.segments.push_back(std::move(segment));
    }
    if (!result.segments.empty()) {
        result.audio = result.segments.back().end;
    }
    return result;
}

YandexAsrProvider::YandexAsrProvider(core::IHttpClient &client, Sleeper sleep)
    : client_{client}, sleep_{sleep ? std::move(sleep) : Sleeper{[](std::chrono::milliseconds how) {
          std::this_thread::sleep_for(how);
      }}}
{}

core::AsrCapabilities YandexAsrProvider::capabilities(const core::AsrEndpoint &where) const
{
    core::AsrCapabilities can;

    can.multichannel = false;
    can.diarisation = false;
    can.wordTimestamps = true;
    can.languageHint = true;
    can.detectsLanguage = true;
    can.formats = kFormats;
    can.maxBytes = numberOf(where, "max_bytes", kUploadCeiling);
    can.maxDuration = std::chrono::seconds{
        static_cast<std::int64_t>(numberOf(where, "max_duration", kLongestRecording))};
    can.maxConcurrent =
        static_cast<unsigned>(std::max<std::uint64_t>(numberOf(where, "max_concurrent", 2), 1));
    return can;
}

core::AsrResult YandexAsrProvider::transcribe(const core::AsrEndpoint &where,
                                              const core::AsrInput &input,
                                              const core::AsrOptions &options,
                                              const core::AsrCancel &cancelled)
{
    if (where.url.empty()) {
        return refuse(core::AsrStatus::Unsupported, "this provider has no address");
    }

    std::string audio;
    {
        std::ifstream file{input.file, std::ios::binary};
        if (!file) {
            return refuse(core::AsrStatus::Failed, "the audio could not be read");
        }
        audio.assign(std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{});
    }
    if (audio.size() > numberOf(where, "max_bytes", kUploadCeiling)) {
        return refuse(core::AsrStatus::TooLarge,
                      "the recording is larger than this provider takes in one request");
    }

    std::string operation;
    {
        nlohmann::json body;
        body["content"] = base64Of(audio);
        auto &model = body["recognitionModel"];

        if (!where.model.empty()) {
            model["model"] = where.model;
        }
        model["audioFormat"]["containerAudio"]["containerAudioType"] = containerFor(input.mimeType);
        model["textNormalization"]["textNormalization"] = flagOf(where, "text_normalization", true)
                                                              ? "TEXT_NORMALIZATION_ENABLED"
                                                              : "TEXT_NORMALIZATION_DISABLED";
        model["textNormalization"]["profanityFilter"] = false;
        model["textNormalization"]["literatureText"] = false;
        model["audioProcessingType"] = "FULL_DATA";
        if (const auto language = yandexLanguageFor(options.language); !language.empty()) {
            model["languageRestriction"]["restrictionType"] = "WHITELIST";
            model["languageRestriction"]["languageCode"] = nlohmann::json::array({language});
        }
        for (const auto &[name, value] : where.params) {
            if (!ours(name)) {
                body[name] = jsonValueOf(value);
            }
        }

        core::HttpRequest request;
        request.verb = core::HttpVerb::Post;
        request.url = join(where.url, "stt/v3/recognizeFileAsync");
        request.timeout = where.timeout;
        request.contentType = "application/json";
        request.body = body.dump();
        authorise(request, where);

        const auto answer = client_.send(request, cancelled);
        if (cancelled && cancelled()) {
            return refuse(core::AsrStatus::Cancelled, "cancelled");
        }
        if (const auto status = statusOf(answer); status != core::AsrStatus::Ok) {
            auto result = refuse(status, saidBy(answer));
            result.raw = answer.body;
            return result;
        }

        const auto parsed = nlohmann::json::parse(answer.body, nullptr, false);
        operation = parsed.is_object() ? textIn(parsed, "id") : std::string{};
        if (operation.empty()) {
            return refuse(core::AsrStatus::Malformed,
                          "the recognition was accepted without an operation");
        }
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
            request.url = join(paramOf(where, "operations_url"), "operations/" + operation);
            request.timeout = std::chrono::seconds{30};
            authorise(request, where);

            const auto answer = client_.send(request, cancelled);
            if (const auto status = statusOf(answer); status != core::AsrStatus::Ok) {
                return refuse(status, saidBy(answer));
            }

            const auto parsed = nlohmann::json::parse(answer.body, nullptr, false);
            if (parsed.is_discarded() || !parsed.is_object()) {
                return refuse(core::AsrStatus::Malformed,
                              "the operation said nothing about itself");
            }
            if (const auto error = parsed.find("error");
                error != parsed.end() && error->is_object() && !error->empty()) {
                auto said = textIn(*error, "message");
                return refuse(core::AsrStatus::Unsupported,
                              said.empty() ? std::string{"the provider could not recognise it"}
                                           : std::move(said));
            }
            if (streaming::readOr(parsed, "done", false)) {
                break;
            }

            if (waited >= limit) {
                return refuse(core::AsrStatus::Timeout,
                              "the recognition was still running when the deadline passed");
            }
            sleep_(wait);
            waited += wait;
            wait = std::min(wait * 2,
                            std::chrono::duration_cast<std::chrono::milliseconds>(kLongestWait));
        }
    }

    core::HttpRequest request;
    request.verb = core::HttpVerb::Get;
    request.url = join(where.url, "stt/v3/getRecognition") + "?operationId=" + operation;
    request.timeout = where.timeout;
    authorise(request, where);

    const auto answer = client_.send(request, cancelled);
    if (cancelled && cancelled()) {
        return refuse(core::AsrStatus::Cancelled, "cancelled");
    }
    if (const auto status = statusOf(answer); status != core::AsrStatus::Ok) {
        auto result = refuse(status, saidBy(answer));
        result.raw = answer.body;
        return result;
    }

    auto result = readYandexTranscription(answer.body);
    if (result.detectedLanguage.empty()) {
        result.detectedLanguage = options.language;
    }
    result.model = where.model.empty() ? std::string{"general"} : where.model;
    return result;
}

core::AsrProbe YandexAsrProvider::probe(const core::AsrEndpoint &where)
{
    core::AsrProbe probe;
    if (where.url.empty()) {
        probe.status = core::AsrStatus::Unsupported;
        probe.detail = "this provider has no address";
        return probe;
    }

    core::HttpRequest request;
    request.verb = core::HttpVerb::Get;
    request.url = join(where.url, "stt/v3/getRecognition") + "?operationId=probe";
    request.timeout = std::chrono::seconds{15};
    authorise(request, where);

    const auto answer = client_.send(request, {});
    if (!answer.answered()) {
        probe.status = statusOf(answer);
        probe.detail = answer.error;
        return probe;
    }
    if (answer.status == 401 || answer.status == 403) {
        probe.status = core::AsrStatus::Unauthorised;
        probe.detail = saidBy(answer);
        return probe;
    }

    probe.status = core::AsrStatus::Ok;
    probe.reachable = true;

    return probe;
}

}
