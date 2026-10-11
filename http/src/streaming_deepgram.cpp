// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "streaming_session.hpp"

#include <softphone/core/logging.hpp>
#include <softphone/core/provider_knobs.hpp>

#include "streaming_json.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <utility>
#include <vector>

namespace softphone::http {
namespace {

using namespace std::chrono_literals;
using namespace streaming;

constexpr std::string_view kDefaultHost = "wss://api.deepgram.com";

class Deepgram final : public IStreamingProtocol {
public:
    [[nodiscard]] std::string_view type() const noexcept override
    {
        return "deepgram";
    }

    [[nodiscard]] core::StreamingCapabilities
    capabilities(const core::AsrEndpoint &where) const override
    {
        core::StreamingCapabilities can;

        can.marksTurns = true;
        can.translates = false;
        can.interim = true;
        can.languageHint = true;
        can.detectsLanguage = true;
        can.diarisation = true;

        can.accepts = {{core::AsrEncoding::Pcm16, 16000, 1},
                       {core::AsrEncoding::Pcm16, 8000, 1},
                       {core::AsrEncoding::Pcm16, 48000, 1},
                       {core::AsrEncoding::Mulaw, 8000, 1}};

        can.idleCloses = 10s;

        can.reportsCost = false;

        can.languages = {"en", "en-US", "en-GB", "en-AU", "en-IN", "en-NZ", "es",    "es-419",
                         "fr", "fr-CA", "de",    "de-CH", "it",    "pt",    "pt-BR", "nl",
                         "hi", "ja",    "ko",    "zh",    "zh-CN", "zh-TW", "ru",    "uk",
                         "sv", "da",    "no",    "fi",    "pl",    "cs",    "sk",    "el",
                         "tr", "id",    "vi",    "th",    "ms",    "ta",    "multi"};

        (void)where;
        return can;
    }

    [[nodiscard]] std::string url(const core::AsrEndpoint &where,
                                  const core::StreamingOptions &options) const override
    {

        auto host =
            asSocket(where.url.empty() ? std::string{kDefaultHost} : where.url, "/v1/listen");

        std::string query = "?encoding=";
        query += options.wire.encoding == core::AsrEncoding::Mulaw ? "mulaw" : "linear16";
        query += "&sample_rate=" + std::to_string(options.wire.sampleRate);
        query += "&channels=1";
        query += "&interim_results=true";
        query += "&punctuate=true";
        query += "&smart_format=true";

        query += "&endpointing=300";

        if (!where.model.empty()) {
            query += "&model=" + where.model;
        }
        if (!options.language.empty()) {
            query += "&language=" + options.language;
        }
        if (options.diarise) {
            query += "&diarize=true";
        }

        return host + query;
    }

    [[nodiscard]] std::vector<std::pair<std::string, std::string>>
    headers(const core::AsrEndpoint &where) const override
    {
        if (where.token.empty()) {
            return {};
        }

        return {{"Authorization", "Token " + where.token}};
    }

    [[nodiscard]] std::vector<std::string> opening(const core::AsrEndpoint &,
                                                   const core::StreamingOptions &) const override
    {

        return {};
    }

    [[nodiscard]] std::vector<std::string> closing(std::size_t) const override
    {

        return {R"({"type":"Finalize"})", R"({"type":"CloseStream"})"};
    }

    void read(std::string_view payload, StreamingHeard &heard) override
    {
        const auto message = nlohmann::json::parse(payload, nullptr, false);
        if (message.is_discarded() || !message.is_object()) {
            return;
        }

        const auto kind = readOr<std::string>(message, "type", {});
        heard.shape = kind;

        if (kind == "Error") {
            heard.understood = true;
            heard.fatal = core::AsrStatus::Failed;
            heard.said = readOr<std::string>(message, "description",
                                             readOr<std::string>(message, "message", "refused"));
            return;
        }

        if (kind == "Metadata") {
            heard.understood = true;
            heard.ended = true;
            return;
        }

        if (kind == "TurnInfo") {
            heard.understood = true;
            readFlux(message, heard);
            return;
        }

        if (kind != "Results" && !message.contains("channel")) {

            heard.understood = kind == "SpeechStarted" || kind == "UtteranceEnd" ||
                               kind == "Warning" || kind == "KeepAlive";
            return;
        }

        heard.understood = true;
        readResults(message, heard);
    }

private:
    void readFlux(const nlohmann::json &message, StreamingHeard &heard) const
    {
        const auto event = readOr<std::string>(message, "event", {});
        const auto text = readOr<std::string>(message, "transcript", {});
        if (text.empty()) {
            return;
        }

        const auto start = fromSeconds(readOr<double>(message, "audio_window_start", 0.0));
        const auto end = fromSeconds(readOr<double>(message, "audio_window_end", 0.0));

        if (event == "EndOfTurn") {
            core::AsrTurn said;
            said.start = start;
            said.end = end;
            said.text = text;
            said.confidence = readOr<double>(message, "end_of_turn_confidence", 0.0);
            said.boundary = core::AsrTurn::Boundary::Provider;
            heard.turns.push_back(std::move(said));
            return;
        }

        core::AsrPartial said;
        said.start = start;
        said.end = end;
        said.text = text;
        heard.partials.push_back(std::move(said));
    }

    void readResults(const nlohmann::json &message, StreamingHeard &heard) const
    {
        const auto channel = message.find("channel");
        if (channel == message.end() || !channel->is_object()) {
            return;
        }
        const auto alternatives = channel->find("alternatives");
        if (alternatives == channel->end() || !alternatives->is_array() || alternatives->empty()) {
            return;
        }

        const auto &best = alternatives->front();
        const auto text = readOr<std::string>(best, "transcript", {});

        if (text.empty()) {
            return;
        }

        const auto start = fromSeconds(readOr<double>(message, "start", 0.0));
        const auto end = start + fromSeconds(readOr<double>(message, "duration", 0.0));

        const auto speechFinal = readOr<bool>(message, "speech_final", false);
        const auto isFinal = readOr<bool>(message, "is_final", false);

        if (!speechFinal) {
            core::AsrPartial said;
            said.start = start;
            said.end = end;
            said.text = text;
            said.words = wordsOf(best, start);
            heard.partials.push_back(std::move(said));
            (void)isFinal;
            return;
        }

        core::AsrTurn said;
        said.start = start;
        said.end = end;
        said.text = text;
        said.confidence = readOr<double>(best, "confidence", 0.0);
        said.words = wordsOf(best, start);
        said.boundary = core::AsrTurn::Boundary::Provider;
        heard.turns.push_back(std::move(said));
    }

    [[nodiscard]] static std::vector<core::AsrWord> wordsOf(const nlohmann::json &best,
                                                            std::chrono::milliseconds)
    {
        std::vector<core::AsrWord> words;
        const auto found = best.find("words");
        if (found == best.end() || !found->is_array()) {
            return words;
        }

        words.reserve(found->size());
        for (const auto &one : *found) {
            if (!one.is_object()) {
                continue;
            }
            core::AsrWord word;
            word.text =
                readOr<std::string>(one, "punctuated_word", readOr<std::string>(one, "word", {}));
            word.start = fromSeconds(readOr<double>(one, "start", 0.0));
            word.end = fromSeconds(readOr<double>(one, "end", 0.0));
            word.confidence = readOr<double>(one, "confidence", 0.0);
            if (!word.text.empty()) {
                words.push_back(std::move(word));
            }
        }
        return words;
    }
};

}

std::unique_ptr<IStreamingProtocol> makeDeepgramStreaming()
{
    return std::make_unique<Deepgram>();
}

}
