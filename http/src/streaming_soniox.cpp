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
#include "streaming_session.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <utility>
#include <vector>

namespace softphone::http {
namespace {

using namespace std::chrono_literals;
using namespace streaming;

struct Tokens {

    std::string settled;

    std::string guess;

    std::string translation;
    std::chrono::milliseconds start{0};
    std::chrono::milliseconds end{0};
    double confidence{0.0};
    std::string language;
    bool anyFinal{false};
    bool anyProvisional{false};

    bool endOfUtterance{false};
};

[[nodiscard]] Tokens readTokens(const nlohmann::json &message)
{
    Tokens heard;

    const auto tokens = message.find("tokens");
    if (tokens == message.end() || !tokens->is_array()) {
        return heard;
    }

    bool first = true;
    double total = 0.0;
    std::size_t counted = 0;

    for (const auto &one : *tokens) {
        if (!one.is_object()) {
            continue;
        }

        const auto text = readOr<std::string>(one, "text", {});
        if (text.empty()) {
            continue;
        }

        if (text == "<end>") {
            heard.endOfUtterance = true;
            continue;
        }

        const auto status = readOr<std::string>(one, "translation_status", {});
        const auto start = fromMilliseconds(readOr<double>(one, "start_ms", 0.0));
        const auto end = fromMilliseconds(readOr<double>(one, "end_ms", 0.0));

        const auto settled = readOr<bool>(one, "is_final", false);
        if (settled) {
            heard.anyFinal = true;
        } else {
            heard.anyProvisional = true;
        }

        if (status == "translation") {
            heard.translation += text;
            if (heard.language.empty()) {
                heard.language = readOr<std::string>(one, "source_language", {});
            }
            continue;
        }

        (settled ? heard.settled : heard.guess) += text;
        if (first) {
            heard.start = start;
            first = false;
        }
        heard.end = end;

        if (const auto confidence = readOr<double>(one, "confidence", 0.0); confidence > 0.0) {
            total += confidence;
            ++counted;
        }
        if (heard.language.empty()) {
            heard.language = readOr<std::string>(one, "language", {});
        }
    }

    heard.confidence = counted == 0 ? 0.0 : total / static_cast<double>(counted);
    return heard;
}

class SonioxStreaming final : public IStreamingProtocol {
public:
    [[nodiscard]] std::string_view type() const noexcept override
    {
        return "soniox";
    }

    [[nodiscard]] core::StreamingCapabilities capabilities(const core::AsrEndpoint &) const override
    {
        core::StreamingCapabilities can;

        can.translates = true;
        can.interim = true;
        can.languageHint = true;
        can.detectsLanguage = true;
        can.diarisation = true;

        can.marksTurns = true;

        can.accepts = {{core::AsrEncoding::Pcm16, 16000, 1},
                       {core::AsrEncoding::Pcm16, 8000, 1},
                       {core::AsrEncoding::Pcm16, 44100, 1},
                       {core::AsrEncoding::Pcm16, 48000, 1}};

        can.reportsCost = false;
        can.idleCloses = 0s;

        can.languages = {"en", "es", "fr", "de", "it", "pt", "nl", "pl", "ru", "uk",
                         "cs", "sk", "sl", "hr", "sr", "bg", "ro", "hu", "el", "tr",
                         "fi", "sv", "da", "nb", "et", "lv", "lt", "id", "vi", "ja",
                         "ko", "zh", "hi", "ar", "he", "ta", "th", "ms", "ca"};
        return can;
    }

    [[nodiscard]] std::string url(const core::AsrEndpoint &where,
                                  const core::StreamingOptions &) const override
    {
        return asSocket(where.url.empty() ? "wss://stt-rt.soniox.com" : where.url,
                        "/transcribe-websocket");
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
        nlohmann::json say;

        if (!where.model.empty()) {
            say["model"] = where.model;
        }

        say["audio_format"] =
            options.wire.encoding == core::AsrEncoding::Mulaw ? "mulaw" : "pcm_s16le";
        say["sample_rate"] = options.wire.sampleRate;
        say["num_channels"] = 1;

        if (!options.language.empty()) {
            say["language_hints"] = nlohmann::json::array({options.language});
        } else {
            say["enable_language_identification"] = true;
        }

        if (options.diarise) {
            say["enable_speaker_diarization"] = true;
        }

        say["enable_endpoint_detection"] = true;

        if (!options.into.empty()) {
            nlohmann::json translation;
            translation["type"] = "one_way";
            translation["target_language"] = options.into;
            say["translation"] = std::move(translation);
        }

        return {say.dump()};
    }

    [[nodiscard]] std::vector<std::string> closing(std::size_t) const override
    {

        return {""};
    }

    void read(std::string_view payload, StreamingHeard &heard) override
    {
        const auto message = nlohmann::json::parse(payload, nullptr, false);
        if (message.is_discarded() || !message.is_object()) {
            heard.shape = "(not json)";
            return;
        }

        if (message.contains("error_code") || message.contains("error_message")) {
            heard.shape = "error";
            heard.understood = true;
            heard.said = firstString(message, "error_message", "error_type");
            const auto code = readOr<int>(message, "error_code", 0);
            heard.fatal = code == 401 || code == 403 ? core::AsrStatus::Unauthorised
                                                     : core::AsrStatus::Failed;
            if (heard.said.empty()) {
                heard.said = "the recogniser refused the session";
            }
            return;
        }

        if (!message.contains("tokens")) {
            heard.shape = firstString(message, "type", "message");
            return;
        }

        heard.shape = "tokens";
        heard.understood = true;

        const auto said = readTokens(message);

        if (readOr<bool>(message, "finished", false)) {
            heard.ended = true;
        }

        if (said.anyFinal) {
            if (settled_.empty()) {
                began_ = said.start;
            }
            settled_ += said.settled;
            spoke_ = said.end;
            if (!said.translation.empty()) {
                translated_ += said.translation;
            }
            if (said.confidence > 0.0) {
                sure_ = said.confidence;
            }
            if (!said.language.empty()) {
                language_ = said.language;
            }
        }

        const auto line = settled_ + said.guess;

        if (said.endOfUtterance && !settled_.empty()) {
            core::AsrTurn turn;
            turn.start = began_;
            turn.end = spoke_;
            turn.text = settled_;
            turn.translation = translated_;
            turn.confidence = sure_;
            turn.language = language_;

            turn.boundary = core::AsrTurn::Boundary::Provider;
            heard.turns.push_back(std::move(turn));

            settled_.clear();
            translated_.clear();
            language_.clear();
            sure_ = 0.0;
            return;
        }

        if (line.empty() && said.translation.empty()) {
            return;
        }

        core::AsrPartial partial;
        partial.start = began_;
        partial.end = said.end;
        partial.text = line.empty() ? said.translation : line;
        heard.partials.push_back(std::move(partial));
    }

private:

    std::string settled_;
    std::string translated_;
    std::string language_;
    std::chrono::milliseconds began_{0};
    std::chrono::milliseconds spoke_{0};
    double sure_{0.0};
};

}

std::unique_ptr<IStreamingProtocol> makeSonioxStreaming()
{
    return std::make_unique<SonioxStreaming>();
}

}
