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
#include "streaming_protocols.hpp"
#include "streaming_realtime_events.hpp"
#include "streaming_session.hpp"

#include <softphone/core/logging.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <ios>
#include <locale>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::http {
namespace {

using namespace std::chrono_literals;
using namespace streaming;

[[nodiscard]] std::vector<core::AsrWord> flatWords(const nlohmann::json &from,
                                                   std::string_view field)
{
    std::vector<core::AsrWord> words;
    if (!from.is_object()) {
        return words;
    }
    const auto found = from.find(field);
    if (found == from.end() || !found->is_array()) {
        return words;
    }

    words.reserve(found->size());
    for (const auto &one : *found) {
        if (!one.is_object()) {
            continue;
        }
        if (const auto kind = readOr<std::string>(one, "type", {});
            kind == "spacing" || kind == "audio_event") {
            continue;
        }
        core::AsrWord word;
        word.text = firstString(one, "text", "word", "punctuated_word");
        word.start = fromSeconds(readOr<double>(one, "start", 0.0));
        word.end = fromSeconds(readOr<double>(one, "end", 0.0));
        word.confidence = readOr<double>(one, "confidence", readOr<double>(one, "logprob", 0.0));
        if (!word.text.empty()) {
            words.push_back(std::move(word));
        }
    }
    return words;
}

[[nodiscard]] std::string paramOf(const core::AsrEndpoint &where, std::string_view name)
{
    const auto found =
        std::ranges::find_if(where.params, [name](const auto &pair) { return pair.first == name; });
    return found == where.params.end() ? std::string{} : found->second;
}

void fromTheRow(std::string &host, const core::AsrEndpoint &where, std::string_view name,
                double least, double most)
{
    const auto said = paramOf(where, name);
    if (said.empty()) {
        return;
    }

    std::istringstream reading{said};
    reading.imbue(std::locale::classic());
    double value = 0.0;
    reading >> value;
    const bool read = !reading.fail() && reading.peek() == std::char_traits<char>::eof();
    if (!read || value < least || value > most) {

        const core::LogAttribute refused[]{{"parameter", name}, {"said", said}};
        core::Logger::instance().log(core::LogLevel::Warning, "realtime",
                                     "xai: a parameter from the recogniser's row is outside what "
                                     "the service takes, so nothing was sent for it",
                                     refused);
        return;
    }

    host += host.find('?') == std::string::npos ? '?' : '&';
    host += name;
    host += '=';
    host += said;
}

class XaiStreaming final : public IStreamingProtocol {
public:
    [[nodiscard]] std::string_view type() const noexcept override
    {
        return "xai";
    }

    [[nodiscard]] core::StreamingCapabilities capabilities(const core::AsrEndpoint &) const override
    {
        core::StreamingCapabilities can;

        can.marksTurns = true;
        can.interim = true;
        can.translates = false;
        can.languageHint = true;
        can.detectsLanguage = true;

        can.diarisation = true;

        can.accepts = {{core::AsrEncoding::Pcm16, 16000, 1},
                       {core::AsrEncoding::Pcm16, 8000, 1},
                       {core::AsrEncoding::Pcm16, 24000, 1},
                       {core::AsrEncoding::Pcm16, 48000, 1}};
        can.reportsCost = false;
        can.idleCloses = 20s;
        can.languages = {"en", "es", "fr", "de", "it", "pt", "nl", "pl", "ru", "uk",
                         "tr", "ar", "hi", "ja", "ko", "zh", "id", "vi", "sv", "da",
                         "no", "fi", "cs", "el", "he", "ro", "hu", "th"};
        return can;
    }

    [[nodiscard]] std::string url(const core::AsrEndpoint &where,
                                  const core::StreamingOptions &options) const override
    {
        auto host = asSocket(where.url.empty() ? "wss://api.x.ai" : where.url, "/v1/stt");
        if (host.empty()) {
            return host;
        }
        const auto join = [&host] { host += host.find('?') == std::string::npos ? '?' : '&'; };

        if (!where.model.empty()) {
            join();
            host += "model=" + where.model;
        }
        join();
        host += "encoding=";
        host += options.wire.encoding == core::AsrEncoding::Mulaw ? "mulaw" : "pcm";
        join();
        host += "sample_rate=" + std::to_string(options.wire.sampleRate);
        if (!options.language.empty()) {
            join();
            host += "language=" + options.language;
        }

        join();
        host += "interim_results=true";
        if (options.diarise) {
            join();
            host += "diarize=true";
        }

        fromTheRow(host, where, "smart_turn", 0.0, 1.0);
        fromTheRow(host, where, "smart_turn_timeout", 1.0, 5000.0);
        return host;
    }

    [[nodiscard]] std::vector<std::pair<std::string, std::string>>
    headers(const core::AsrEndpoint &where) const override
    {
        if (where.token.empty()) {
            return {};
        }
        return {{"Authorization", "Bearer " + where.token}};
    }

    [[nodiscard]] std::vector<std::string>
    opening(const core::AsrEndpoint &where, const core::StreamingOptions &options) const override
    {

        (void)where;
        (void)options;
        return {};
    }

    [[nodiscard]] bool waitsToBeTold() const noexcept override
    {
        return true;
    }

    [[nodiscard]] std::vector<std::string> closing(std::size_t) const override
    {
        return {R"({"type":"finalize"})", R"({"type":"audio.done"})"};
    }

    void read(std::string_view payload, StreamingHeard &heard) override
    {
        const auto message = nlohmann::json::parse(payload, nullptr, false);
        if (message.is_discarded() || !message.is_object()) {
            heard.shape = "(not json)";
            return;
        }

        const auto kind = firstString(message, "type", "event");
        heard.shape = kind;

        if (kind == "error" || kind == "session.error") {
            heard.understood = true;
            heard.fatal = core::AsrStatus::Failed;
            heard.said = firstString(message, "message", "error", "detail");
            if (heard.said.empty()) {
                heard.said = "the recogniser refused the session";
            }
            return;
        }

        if (kind == "transcript.created") {
            heard.understood = true;
            heard.ready = true;
            return;
        }

        if (kind == "session.updated" || kind == "session.created" || kind == "speech.started") {
            heard.understood = true;
            return;
        }

        if (kind == "session.closed") {
            heard.understood = true;
            heard.ended = true;
            return;
        }

        if (kind == "transcript.done" && firstString(message, "text", "transcript").empty()) {
            heard.understood = true;
            heard.ended = true;
            return;
        }

        const auto text = firstString(message, "text", "transcript", "delta");
        if (text.empty()) {

            heard.understood = kind.rfind("session.", 0) == 0 ||
                               kind.rfind("transcript.", 0) == 0 || kind == "pong";
            return;
        }

        heard.understood = true;

        const auto startSeconds = readOr<double>(message, "start", 0.0);
        const auto start = startSeconds > 0.0
                               ? fromSeconds(startSeconds)
                               : fromMilliseconds(readOr<double>(message, "start_ms", 0.0));

        const auto spoken = readOr<double>(message, "duration", 0.0);
        const auto end = spoken > 0.0 ? fromSeconds(startSeconds + spoken)
                                      : fromMilliseconds(readOr<double>(message, "end_ms", 0.0));

        const auto finished =
            kind == "transcript.done" || readOr<bool>(message, "speech_final", false);

        if (!finished) {
            core::AsrPartial said;
            said.start = start;
            said.end = end;
            said.text = text;
            said.words = flatWords(message, "words");
            heard.partials.push_back(std::move(said));
            return;
        }

        core::AsrTurn said;
        said.start = start;
        said.end = end;
        said.text = text;
        said.confidence = readOr<double>(message, "confidence", 0.0);
        said.language = readOr<std::string>(message, "language", {});
        said.words = flatWords(message, "words");
        said.boundary = core::AsrTurn::Boundary::Provider;
        heard.turns.push_back(std::move(said));

        if (kind == "transcript.done") {
            heard.ended = true;
        }
    }
};

class ElevenLabsStreaming final : public IStreamingProtocol {
public:
    [[nodiscard]] std::string_view type() const noexcept override
    {
        return "elevenlabs";
    }

    [[nodiscard]] core::StreamingCapabilities capabilities(const core::AsrEndpoint &) const override
    {
        core::StreamingCapabilities can;

        can.marksTurns = true;
        can.interim = true;
        can.translates = false;
        can.languageHint = true;
        can.detectsLanguage = true;
        can.diarisation = false;

        can.accepts = {{core::AsrEncoding::Pcm16, 16000, 1},
                       {core::AsrEncoding::Pcm16, 8000, 1},
                       {core::AsrEncoding::Pcm16, 24000, 1},
                       {core::AsrEncoding::Pcm16, 48000, 1},
                       {core::AsrEncoding::Mulaw, 8000, 1}};
        can.reportsCost = false;
        can.idleCloses = 20s;

        can.languages = {"en", "es", "fr", "de", "it", "pt", "nl", "pl", "ru", "uk",
                         "cs", "sk", "sl", "hr", "sr", "bg", "ro", "hu", "el", "tr",
                         "fi", "sv", "da", "nb", "et", "lv", "lt", "id", "vi", "ja",
                         "ko", "zh", "hi", "ar", "he", "ta", "th", "ms"};
        return can;
    }

    [[nodiscard]] std::string url(const core::AsrEndpoint &where,
                                  const core::StreamingOptions &options) const override
    {
        auto host = asSocket(where.url.empty() ? "wss://api.elevenlabs.io" : where.url,
                             "/v1/speech-to-text/realtime");

        host += "?";
        if (!where.model.empty()) {
            host += "model_id=" + where.model + "&";
        }

        host += "audio_format=";
        host += options.wire.encoding == core::AsrEncoding::Mulaw ? "ulaw_" : "pcm_";
        host += std::to_string(options.wire.sampleRate);
        if (!options.language.empty()) {
            host += "&language_code=" + options.language;
        }

        host += "&include_timestamps=true";
        host += "&include_language_detection=true";
        return host;
    }

    [[nodiscard]] std::vector<std::pair<std::string, std::string>>
    headers(const core::AsrEndpoint &where) const override
    {
        if (where.token.empty()) {
            return {};
        }

        return {{"xi-api-key", where.token}};
    }

    [[nodiscard]] bool audioIsBinary() const noexcept override
    {
        return false;
    }

    [[nodiscard]] std::string wrapAudio(std::span<const std::byte> audio) const override
    {
        return base64Frame(R"({"message_type":"input_audio_chunk","audio_base_64":")", audio,
                           R"("})");
    }

    [[nodiscard]] std::vector<std::string> closing(std::size_t) const override
    {
        return {R"({"message_type":"input_audio_chunk","audio_base_64":"","commit":true})"};
    }

    void read(std::string_view payload, StreamingHeard &heard) override
    {
        const auto message = nlohmann::json::parse(payload, nullptr, false);
        if (message.is_discarded() || !message.is_object()) {
            heard.shape = "(not json)";
            return;
        }

        const auto kind = firstString(message, "type", "message_type");
        heard.shape = kind;

        if (kind == "error") {
            heard.understood = true;
            heard.fatal = core::AsrStatus::Failed;
            heard.said = firstString(message, "message", "detail", "error");
            if (heard.said.empty()) {
                heard.said = "the recogniser refused the session";
            }
            return;
        }

        if (kind == "session_started" || kind == "audio_received" || kind == "pong") {
            heard.understood = true;
            return;
        }

        if (kind == "partial_transcript" || kind == "committed_transcript" ||
            kind == "committed_transcript_with_timestamps") {
            heard.understood = true;

            if (kind != "partial_transcript") {
                const auto words = firstString(message, "text", "transcript");
                if (words == committed_ && kind != committedAs_) {
                    committedAs_ = kind;
                    return;
                }
                committed_ = words;
                committedAs_ = kind;
            }
            const auto said = firstString(message, "text", "transcript");
            if (said.empty()) {
                return;
            }
            const auto from = fromSeconds(readOr<double>(message, "start", 0.0));
            const auto until = fromSeconds(readOr<double>(message, "end", 0.0));

            if (kind == "partial_transcript") {
                core::AsrPartial drawing;
                drawing.start = from;
                drawing.end = until;
                drawing.text = said;
                drawing.words = flatWords(message, "words");
                heard.partials.push_back(std::move(drawing));
                return;
            }

            core::AsrTurn turn;
            turn.start = from;
            turn.end = until;
            turn.text = said;
            turn.confidence = readOr<double>(message, "confidence", 0.0);
            turn.language = readOr<std::string>(message, "language_code", {});
            turn.words = flatWords(message, "words");
            turn.boundary = core::AsrTurn::Boundary::Provider;
            heard.turns.push_back(std::move(turn));
            return;
        }

        if (kind == "session_ended" || kind == "closed") {
            heard.understood = true;
            heard.ended = true;
            return;
        }

        const auto text = firstString(message, "text", "transcript");
        if (text.empty()) {
            return;
        }
        heard.understood = true;

        const auto start = fromSeconds(readOr<double>(message, "start", 0.0));
        const auto end = fromSeconds(readOr<double>(message, "end", 0.0));

        const auto finished = kind == "final_transcript" || kind == "transcript" ||
                              readOr<bool>(message, "is_final", false);

        if (!finished) {
            core::AsrPartial said;
            said.start = start;
            said.end = end;
            said.text = text;
            said.words = flatWords(message, "words");
            heard.partials.push_back(std::move(said));
            return;
        }

        core::AsrTurn said;
        said.start = start;
        said.end = end;
        said.text = text;
        said.confidence = readOr<double>(message, "confidence", 0.0);
        said.language = firstString(message, "language_code", "language");
        said.words = flatWords(message, "words");
        said.boundary = core::AsrTurn::Boundary::Provider;
        heard.turns.push_back(std::move(said));
    }

private:

    std::string committed_;
    std::string committedAs_;
};

}

std::unique_ptr<IStreamingProtocol> makeXaiStreaming()
{
    return std::make_unique<XaiStreaming>();
}

std::unique_ptr<IStreamingProtocol> makeElevenLabsStreaming()
{
    return std::make_unique<ElevenLabsStreaming>();
}

}
