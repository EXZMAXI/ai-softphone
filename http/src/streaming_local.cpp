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

#include <nlohmann/json.hpp>

#include <string>
#include <utility>
#include <vector>

namespace softphone::http {
namespace {

using namespace std::chrono_literals;
using namespace streaming;

class VoskStreaming final : public IStreamingProtocol {
public:
    [[nodiscard]] std::string_view type() const noexcept override
    {
        return "vosk";
    }

    [[nodiscard]] core::StreamingCapabilities capabilities(const core::AsrEndpoint &) const override
    {
        core::StreamingCapabilities can;

        can.marksTurns = false;
        can.translates = false;
        can.interim = true;
        can.languageHint = false;
        can.detectsLanguage = false;
        can.diarisation = false;

        can.accepts = {{core::AsrEncoding::Pcm16, 16000, 1}, {core::AsrEncoding::Pcm16, 8000, 1}};

        can.reportsCost = false;
        can.idleCloses = 0s;

        can.languages = {};
        return can;
    }

    [[nodiscard]] std::string url(const core::AsrEndpoint &where,
                                  const core::StreamingOptions &) const override
    {

        return asSocket(where.url, "/");
    }

    [[nodiscard]] std::vector<std::string>
    opening(const core::AsrEndpoint &where, const core::StreamingOptions &options) const override
    {
        nlohmann::json config;
        config["sample_rate"] = options.wire.sampleRate;

        config["words"] = true;

        if (!where.model.empty()) {
            config["model"] = where.model;
        }

        nlohmann::json say;
        say["config"] = std::move(config);
        return {say.dump()};
    }

    [[nodiscard]] std::vector<std::string> closing(std::size_t) const override
    {

        return {R"({"eof":1})"};
    }

    void read(std::string_view payload, StreamingHeard &heard) override
    {
        const auto message = nlohmann::json::parse(payload, nullptr, false);
        if (message.is_discarded() || !message.is_object()) {
            heard.shape = "(not json)";
            return;
        }

        if (message.contains("partial")) {
            heard.shape = "partial";
            heard.understood = true;
            const auto text = readOr<std::string>(message, "partial", {});
            if (text.empty()) {
                return;
            }
            core::AsrPartial said;
            said.text = text;
            heard.partials.push_back(std::move(said));
            return;
        }

        if (!message.contains("text")) {
            heard.shape = firstString(message, "type", "message");
            return;
        }

        heard.shape = "text";
        heard.understood = true;

        const auto text = readOr<std::string>(message, "text", {});
        if (text.empty()) {

            return;
        }

        core::AsrTurn said;
        said.text = text;
        said.words = wordsOf(message);

        if (!said.words.empty()) {
            said.start = said.words.front().start;
            said.end = said.words.back().end;
            double total = 0.0;
            for (const auto &word : said.words) {
                total += word.confidence;
            }
            said.confidence = total / static_cast<double>(said.words.size());
        }

        said.boundary = core::AsrTurn::Boundary::Silence;
        heard.turns.push_back(std::move(said));
    }

private:
    [[nodiscard]] static std::vector<core::AsrWord> wordsOf(const nlohmann::json &message)
    {
        std::vector<core::AsrWord> words;
        const auto found = message.find("result");
        if (found == message.end() || !found->is_array()) {
            return words;
        }
        words.reserve(found->size());
        for (const auto &one : *found) {
            if (!one.is_object()) {
                continue;
            }
            core::AsrWord word;
            word.text = readOr<std::string>(one, "word", {});
            word.start = fromSeconds(readOr<double>(one, "start", 0.0));
            word.end = fromSeconds(readOr<double>(one, "end", 0.0));
            word.confidence = readOr<double>(one, "conf", 0.0);
            if (!word.text.empty()) {
                words.push_back(std::move(word));
            }
        }
        return words;
    }
};

class WhisperStreaming final : public IStreamingProtocol {
public:
    [[nodiscard]] std::string_view type() const noexcept override
    {
        return "whisper-live";
    }

    [[nodiscard]] core::StreamingCapabilities capabilities(const core::AsrEndpoint &) const override
    {
        core::StreamingCapabilities can;

        can.marksTurns = false;
        can.translates = false;
        can.interim = true;
        can.languageHint = true;
        can.detectsLanguage = true;
        can.diarisation = false;

        can.accepts = {{core::AsrEncoding::Pcm16, 16000, 1}};

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
        return asSocket(where.url, "/");
    }

    [[nodiscard]] std::vector<std::string>
    opening(const core::AsrEndpoint &where, const core::StreamingOptions &options) const override
    {
        nlohmann::json say;

        say["uid"] = "softphone";
        say["task"] = "transcribe";

        if (!where.model.empty()) {
            say["model"] = where.model;
        }
        say["use_vad"] = true;

        say["audio_format"] = "int16";

        if (!options.language.empty()) {
            say["language"] = options.language;
        }

        say["send_last_n_segments"] = 1;
        return {say.dump()};
    }

    [[nodiscard]] bool closingIsBinary() const noexcept override
    {
        return true;
    }

    [[nodiscard]] std::vector<std::string> closing(std::size_t) const override
    {
        return {"END_OF_AUDIO"};
    }

    void read(std::string_view payload, StreamingHeard &heard) override
    {
        const auto message = nlohmann::json::parse(payload, nullptr, false);
        if (message.is_discarded() || !message.is_object()) {
            heard.shape = "(not json)";
            return;
        }

        if (const auto said = readOr<std::string>(message, "message", {}); !said.empty()) {
            heard.shape = said;
            heard.understood = true;
            if (said == "DISCONNECT" || said == "ENDED") {
                heard.ended = true;
            } else if (said == "ERROR") {
                heard.fatal = core::AsrStatus::Failed;
                heard.said = firstString(message, "detail", "error");
                if (heard.said.empty()) {
                    heard.said = "the recogniser refused the session";
                }
            }
            return;
        }

        if (const auto status = readOr<std::string>(message, "status", {}); status == "WAIT") {
            heard.shape = "status";
            heard.understood = true;
            heard.fatal = core::AsrStatus::Unavailable;
            heard.said = "the recogniser is busy with another client";
            return;
        }

        const auto segments = message.find("segments");
        if (segments == message.end() || !segments->is_array()) {
            heard.shape = firstString(message, "type", "status");
            return;
        }

        heard.shape = "segments";
        heard.understood = true;

        for (const auto &one : *segments) {
            if (!one.is_object()) {
                continue;
            }
            const auto text = readOr<std::string>(one, "text", {});
            if (text.empty()) {
                continue;
            }

            const auto start = fromSeconds(asNumber(one, "start"));
            const auto end = fromSeconds(asNumber(one, "end"));

            if (readOr<bool>(one, "completed", false)) {
                core::AsrTurn said;
                said.start = start;
                said.end = end;
                said.text = text;
                said.language = readOr<std::string>(message, "language", {});
                said.boundary = core::AsrTurn::Boundary::Silence;
                heard.turns.push_back(std::move(said));
                continue;
            }

            core::AsrPartial said;
            said.start = start;
            said.end = end;
            said.text = text;
            heard.partials.push_back(std::move(said));
        }
    }

private:

    [[nodiscard]] static double asNumber(const nlohmann::json &from, std::string_view name)
    {
        if (const auto number = readOr<double>(from, name, 0.0); number != 0.0) {
            return number;
        }
        const auto text = readOr<std::string>(from, name, {});
        if (text.empty()) {
            return 0.0;
        }
        try {
            return std::stod(text);
        } catch (const std::exception &) {
            return 0.0;
        }
    }
};

class RivaStreaming final : public IStreamingProtocol {
public:
    [[nodiscard]] std::string_view type() const noexcept override
    {
        return "riva";
    }

    [[nodiscard]] core::StreamingCapabilities capabilities(const core::AsrEndpoint &) const override
    {
        core::StreamingCapabilities can;

        can.marksTurns = true;
        can.translates = false;
        can.interim = true;
        can.languageHint = true;
        can.detectsLanguage = false;
        can.diarisation = true;

        can.accepts = {{core::AsrEncoding::Pcm16, 16000, 1}, {core::AsrEncoding::Pcm16, 8000, 1}};

        can.reportsCost = false;
        can.idleCloses = 0s;
        can.languages = {"en", "es", "fr", "de", "it", "pt", "ru", "ja", "ko", "zh", "ar", "hi"};
        return can;
    }

    [[nodiscard]] std::string url(const core::AsrEndpoint &where,
                                  const core::StreamingOptions &) const override
    {
        auto host = asSocket(where.url, "/v1/realtime");
        if (host.empty()) {
            return host;
        }
        host += host.find('?') == std::string::npos ? "?" : "&";

        host += "intent=transcription";
        return host;
    }

    [[nodiscard]] std::vector<std::string>
    opening(const core::AsrEndpoint &where, const core::StreamingOptions &options) const override
    {
        growing_ = RealtimeGrowing{};

        nlohmann::json audio;
        audio["sample_rate_hz"] = options.wire.sampleRate;
        audio["num_channels"] = 1;

        nlohmann::json transcription;
        if (!options.language.empty()) {
            transcription["language"] = options.language;
        }
        if (!where.model.empty()) {
            transcription["model"] = where.model;
        }

        nlohmann::json recognition;
        recognition["enable_automatic_punctuation"] = true;
        recognition["enable_word_time_offsets"] = true;

        nlohmann::json session;
        session["modalities"] = nlohmann::json::array({"text"});
        session["input_audio_format"] = "pcm16";
        session["input_audio_params"] = std::move(audio);
        if (!transcription.empty()) {
            session["input_audio_transcription"] = std::move(transcription);
        }
        session["recognition_config"] = std::move(recognition);
        if (options.diarise) {
            nlohmann::json diarisation;
            diarisation["enable_speaker_diarization"] = true;
            session["speaker_diarization"] = std::move(diarisation);
        }

        nlohmann::json say;
        say["event_id"] = std::string{kOurHandshake} + "riva";
        say["type"] = "transcription_session.update";
        say["session"] = std::move(session);
        return {say.dump()};
    }

    [[nodiscard]] bool audioIsBinary() const noexcept override
    {
        return false;
    }

    [[nodiscard]] std::string wrapAudio(std::span<const std::byte> audio) const override
    {
        return appendAudio(audio);
    }

    [[nodiscard]] std::vector<std::string> closing(std::size_t) const override
    {
        return {R"({"type":"input_audio_buffer.done"})"};
    }

    void read(std::string_view payload, StreamingHeard &heard) override
    {
        const auto message = nlohmann::json::parse(payload, nullptr, false);
        if (message.is_discarded() || !message.is_object()) {
            heard.shape = "(not json)";
            return;
        }

        (void)readRealtimeEvent(message, heard, growing_);
    }

private:

    mutable RealtimeGrowing growing_;
};

}

std::unique_ptr<IStreamingProtocol> makeVoskStreaming()
{
    return std::make_unique<VoskStreaming>();
}

std::unique_ptr<IStreamingProtocol> makeWhisperStreaming()
{
    return std::make_unique<WhisperStreaming>();
}

std::unique_ptr<IStreamingProtocol> makeRivaStreaming()
{
    return std::make_unique<RivaStreaming>();
}

}
