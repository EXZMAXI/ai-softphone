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

#include <atomic>
#include <string>
#include <utility>
#include <vector>

namespace softphone::http {
namespace {

using namespace std::chrono_literals;
using namespace streaming;

class OpenAiStreaming final : public IStreamingProtocol {
public:
    [[nodiscard]] std::string_view type() const noexcept override
    {
        return "openai";
    }

    [[nodiscard]] core::StreamingCapabilities capabilities(const core::AsrEndpoint &) const override
    {
        core::StreamingCapabilities can;

        can.marksTurns = false;

        can.takesOurTurnMark = true;

        can.interim = true;
        can.translates = false;
        can.languageHint = true;
        can.detectsLanguage = false;
        can.diarisation = false;

        can.accepts = {{core::AsrEncoding::Pcm16, 24000, 1}};

        can.reportsCost = false;
        can.idleCloses = 0s;
        can.languages = {"en", "es", "fr", "de", "it", "pt", "nl", "pl", "ru", "uk",
                         "cs", "sk", "ro", "hu", "el", "tr", "fi", "sv", "da", "nb",
                         "id", "vi", "ja", "ko", "zh", "hi", "ar", "he"};
        return can;
    }

    [[nodiscard]] std::string url(const core::AsrEndpoint &where,
                                  const core::StreamingOptions &) const override
    {

        auto host = asSocket(where.url, "/v1/realtime");
        if (host.empty()) {
            return host;
        }
        host += host.find('?') == std::string::npos ? '?' : '&';
        host += "intent=transcription";
        return host;
    }

    [[nodiscard]] std::vector<std::pair<std::string, std::string>>
    headers(const core::AsrEndpoint &where) const override
    {
        std::vector<std::pair<std::string, std::string>> sending;
        if (!where.token.empty()) {
            sending.emplace_back("Authorization", "Bearer " + where.token);
        }

        return sending;
    }

    [[nodiscard]] std::vector<std::string>
    opening(const core::AsrEndpoint &where, const core::StreamingOptions &options) const override
    {
        growing_ = RealtimeGrowing{};
        growing_.handshakes = 2;

        nlohmann::json transcription;

        if (!where.model.empty()) {
            transcription["model"] = where.model;
        }
        if (!options.language.empty()) {
            transcription["language"] = options.language;
        }

        nlohmann::json format;
        format["type"] = "audio/pcm";
        format["rate"] = options.wire.sampleRate;

        nlohmann::json input;
        input["format"] = std::move(format);
        input["transcription"] = transcription;

        input["turn_detection"] = nullptr;

        nlohmann::json audio;
        audio["input"] = std::move(input);

        nlohmann::json session;
        session["type"] = "transcription";
        session["audio"] = std::move(audio);

        nlohmann::json now;
        now["event_id"] = std::string{kOurHandshake} + "ga";
        now["type"] = "session.update";
        now["session"] = std::move(session);

        nlohmann::json before;
        before["input_audio_format"] = "pcm16";
        before["input_audio_transcription"] = std::move(transcription);
        before["turn_detection"] = nullptr;

        nlohmann::json older;
        older["event_id"] = std::string{kOurHandshake} + "preview";
        older["type"] = "transcription_session.update";
        older["session"] = std::move(before);

        return {now.dump(), older.dump()};
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
        return {R"({"type":"input_audio_buffer.commit"})"};
    }

    [[nodiscard]] std::vector<std::string> endOfTurn() override
    {

        forget_.store(true, std::memory_order_release);
        return {R"({"type":"input_audio_buffer.commit"})"};
    }

    void read(std::string_view payload, StreamingHeard &heard) override
    {
        const auto message = nlohmann::json::parse(payload, nullptr, false);
        if (message.is_discarded() || !message.is_object()) {
            heard.shape = "(not json)";
            return;
        }

        if (forget_.exchange(false, std::memory_order_acq_rel)) {
            growing_.said.clear();
        }

        (void)readRealtimeEvent(message, heard, growing_);

        const auto what = message.find("error");
        const auto code = what != message.end() && what->is_object()
                              ? readOr<std::string>(*what, "code", {})
                              : readOr<std::string>(message, "code", {});
        if (code == "input_audio_buffer_commit_empty") {
            heard.fatal.reset();
            heard.remark = true;
        }
    }

private:

    mutable RealtimeGrowing growing_;

    std::atomic<bool> forget_{false};
};

class SpeechmaticsStreaming final : public IStreamingProtocol {
public:
    [[nodiscard]] std::string_view type() const noexcept override
    {
        return "speechmatics";
    }

    [[nodiscard]] core::StreamingCapabilities
    capabilities(const core::AsrEndpoint &where) const override
    {
        core::StreamingCapabilities can;

        can.marksTurns = true;
        can.interim = true;
        can.translates = false;
        can.languageHint = true;
        can.detectsLanguage = false;
        can.diarisation = true;

        can.accepts = {{core::AsrEncoding::Pcm16, 16000, 1},
                       {core::AsrEncoding::Pcm16, 8000, 1},
                       {core::AsrEncoding::Pcm16, 44100, 1},
                       {core::AsrEncoding::Mulaw, 8000, 1}};
        can.reportsCost = false;
        can.idleCloses = 0s;
        can.languages = {"en", "es", "fr", "de", "it", "pt", "nl", "pl", "ru", "uk",
                         "cs", "sk", "sl", "hr", "bg", "ro", "hu", "el", "tr", "fi",
                         "sv", "da", "no", "et", "lv", "lt", "id", "vi", "ja", "ko",
                         "zh", "hi", "ar", "he", "ta", "th", "ms", "ca", "ga", "mt"};

        (void)where;
        return can;
    }

    [[nodiscard]] std::string url(const core::AsrEndpoint &where,
                                  const core::StreamingOptions &) const override
    {

        auto host = asSocket(where.url, "/v2");
        if (!where.region.empty() && host.find("rt.speechmatics.com") != std::string::npos &&
            host.find(where.region) == std::string::npos) {

            const auto at = host.find("rt.speechmatics.com");
            host.insert(at, where.region + ".");
        }
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
        nlohmann::json format;
        format["type"] = "raw";
        format["encoding"] =
            options.wire.encoding == core::AsrEncoding::Mulaw ? "mulaw" : "pcm_s16le";
        format["sample_rate"] = options.wire.sampleRate;

        nlohmann::json config;
        config["language"] = options.language.empty() ? "en" : options.language;
        config["enable_partials"] = true;

        config["max_delay"] = 1.0;

        if (options.endOfTurnSilence > 0ms) {
            const auto seconds = std::clamp(
                static_cast<double>(options.endOfTurnSilence.count()) / 1000.0, 0.1, 2.0);
            nlohmann::json conversation;
            conversation["end_of_utterance_silence_trigger"] = seconds;
            config["conversation_config"] = std::move(conversation);
        }
        if (options.diarise) {
            config["diarization"] = "speaker";
        }

        if (!where.model.empty()) {
            config["model"] = where.model;
        }

        nlohmann::json say;
        say["message"] = "StartRecognition";
        say["audio_format"] = std::move(format);
        say["transcription_config"] = std::move(config);
        return {say.dump()};
    }

    [[nodiscard]] std::vector<std::string> closing(std::size_t framesSent) const override
    {

        nlohmann::json say;
        say["message"] = "EndOfStream";
        say["last_seq_no"] = framesSent;
        return {say.dump()};
    }

    void read(std::string_view payload, StreamingHeard &heard) override
    {
        const auto message = nlohmann::json::parse(payload, nullptr, false);
        if (message.is_discarded() || !message.is_object()) {
            heard.shape = "(not json)";
            return;
        }

        const auto kind = readOr<std::string>(message, "message", {});
        heard.shape = kind;

        if (kind == "Error") {
            heard.understood = true;
            heard.said = firstString(message, "reason", "type");
            const auto what = readOr<std::string>(message, "type", {});
            heard.fatal = what == "not_authorised" || what == "invalid_audio_type"
                              ? core::AsrStatus::Unauthorised
                              : core::AsrStatus::Failed;
            if (what == "invalid_audio_type" || what == "invalid_model") {
                heard.fatal = core::AsrStatus::Unsupported;
            }
            if (heard.said.empty()) {
                heard.said = "the recogniser refused the session";
            }
            return;
        }

        if (kind == "EndOfTranscript") {
            heard.understood = true;

            closeTheUtterance(heard);
            heard.ended = true;
            return;
        }

        if (kind == "EndOfUtterance") {
            heard.understood = true;
            closeTheUtterance(heard);
            return;
        }

        if (kind == "AddPartialTranscript" || kind == "AddTranscript") {
            heard.understood = true;
            readTranscript(message, kind == "AddTranscript", heard);
            return;
        }

        heard.understood = kind == "RecognitionStarted" || kind == "AudioAdded" ||
                           kind == "Warning" || kind == "Info";
    }

private:
    void readTranscript(const nlohmann::json &message, bool final, StreamingHeard &heard)
    {
        const auto metadata = message.find("metadata");
        if (metadata == message.end() || !metadata->is_object()) {
            return;
        }

        const auto text = readOr<std::string>(*metadata, "transcript", {});
        if (text.empty()) {
            return;
        }

        const auto start = fromSeconds(readOr<double>(*metadata, "start_time", 0.0));
        const auto end = fromSeconds(readOr<double>(*metadata, "end_time", 0.0));

        std::vector<core::AsrWord> words;
        double confidence = 0.0;
        if (const auto results = message.find("results");
            results != message.end() && results->is_array()) {
            words.reserve(results->size());
            double total = 0.0;
            std::size_t counted = 0;
            for (const auto &one : *results) {
                if (!one.is_object() || readOr<std::string>(one, "type", {}) != "word") {
                    continue;
                }
                const auto alternatives = one.find("alternatives");
                if (alternatives == one.end() || !alternatives->is_array() ||
                    alternatives->empty()) {
                    continue;
                }
                core::AsrWord word;
                word.text = readOr<std::string>(alternatives->front(), "content", {});
                word.start = fromSeconds(readOr<double>(one, "start_time", 0.0));
                word.end = fromSeconds(readOr<double>(one, "end_time", 0.0));
                word.confidence = readOr<double>(alternatives->front(), "confidence", 0.0);
                if (word.text.empty()) {
                    continue;
                }
                total += word.confidence;
                ++counted;
                words.push_back(std::move(word));
            }

            confidence = counted == 0 ? 0.0 : total / static_cast<double>(counted);
        }

        if (!final) {

            core::AsrPartial said;
            said.start = settled_.empty() ? start : began_;
            said.end = end;
            said.text = settled_ + text;
            said.words = std::move(words);
            heard.partials.push_back(std::move(said));
            return;
        }

        if (settled_.empty()) {
            began_ = start;
        }
        settled_ += text;
        spoke_ = end;
        if (confidence > 0.0) {
            sure_ = confidence;
        }
        heldWords_.insert(heldWords_.end(), std::make_move_iterator(words.begin()),
                          std::make_move_iterator(words.end()));
    }

    void closeTheUtterance(StreamingHeard &heard)
    {
        if (settled_.empty()) {
            return;
        }

        core::AsrTurn said;
        said.start = began_;
        said.end = spoke_;
        said.text = settled_;
        said.confidence = sure_;
        said.words = std::move(heldWords_);

        said.boundary = core::AsrTurn::Boundary::Provider;
        heard.turns.push_back(std::move(said));

        settled_.clear();
        heldWords_.clear();
        sure_ = 0.0;
    }

    std::string settled_;
    std::vector<core::AsrWord> heldWords_;
    std::chrono::milliseconds began_{0};
    std::chrono::milliseconds spoke_{0};
    double sure_{0.0};
};

}

std::unique_ptr<IStreamingProtocol> makeOpenAiStreaming()
{
    return std::make_unique<OpenAiStreaming>();
}

std::unique_ptr<IStreamingProtocol> makeSpeechmaticsStreaming()
{
    return std::make_unique<SpeechmaticsStreaming>();
}

}
