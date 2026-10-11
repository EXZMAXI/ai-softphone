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

class AssemblyAiStreaming final : public IStreamingProtocol {
public:
    [[nodiscard]] std::string_view type() const noexcept override
    {
        return "assemblyai";
    }

    [[nodiscard]] core::StreamingCapabilities capabilities(const core::AsrEndpoint &) const override
    {
        core::StreamingCapabilities can;

        can.marksTurns = true;

        can.revisesTurns = true;

        can.leastPerMessage = 50ms;

        can.translates = false;
        can.interim = true;

        can.languageHint = false;
        can.detectsLanguage = true;
        can.diarisation = false;

        can.accepts = {{core::AsrEncoding::Pcm16, 16000, 1},
                       {core::AsrEncoding::Pcm16, 8000, 1},
                       {core::AsrEncoding::Mulaw, 8000, 1}};

        can.reportsCost = false;
        can.idleCloses = 0s;

        can.languages = {"en", "es", "fr", "de", "it", "pt", "nl", "pl", "ru",   "uk",
                         "tr", "fi", "ja", "ko", "zh", "hi", "vi", "id", "multi"};
        return can;
    }

    [[nodiscard]] std::string url(const core::AsrEndpoint &where,
                                  const core::StreamingOptions &options) const override
    {
        auto host =
            asSocket(where.url.empty() ? "wss://streaming.assemblyai.com" : where.url, "/v3/ws");
        host += host.find('?') == std::string::npos ? "?" : "&";
        host += "sample_rate=" + std::to_string(options.wire.sampleRate);
        host += "&encoding=";
        host += options.wire.encoding == core::AsrEncoding::Mulaw ? "pcm_mulaw" : "pcm_s16le";

        host += "&format_turns=true";

        if (!where.model.empty()) {
            host += "&speech_model=" + where.model;
        }
        return host;
    }

    [[nodiscard]] std::vector<std::pair<std::string, std::string>>
    headers(const core::AsrEndpoint &where) const override
    {
        if (where.token.empty()) {
            return {};
        }

        return {{"Authorization", where.token}};
    }

    [[nodiscard]] std::vector<std::string> closing(std::size_t) const override
    {
        return {R"({"type":"Terminate"})"};
    }

    void read(std::string_view payload, StreamingHeard &heard) override
    {
        const auto message = nlohmann::json::parse(payload, nullptr, false);
        if (message.is_discarded() || !message.is_object()) {
            heard.shape = "(not json)";
            return;
        }

        const auto kind = readOr<std::string>(message, "type", {});
        heard.shape = kind;

        if (kind == "Error") {
            heard.understood = true;
            heard.said = firstString(message, "error", "message");
            heard.fatal = core::AsrStatus::Failed;
            if (heard.said.empty()) {
                heard.said = "the recogniser refused the session";
            }
            return;
        }

        if (kind == "Begin") {
            heard.understood = true;
            return;
        }

        if (kind == "Termination") {
            heard.understood = true;
            heard.ended = true;
            return;
        }

        if (kind != "Turn") {
            return;
        }

        heard.understood = true;

        const auto text = firstString(message, "transcript", "text");
        if (text.empty()) {
            return;
        }

        auto words = wordsOf(message);
        const auto start = words.empty() ? std::chrono::milliseconds::zero() : words.front().start;
        const auto end = words.empty() ? std::chrono::milliseconds::zero() : words.back().end;

        if (!turnIsComplete(message)) {
            core::AsrPartial said;
            said.start = start;
            said.end = end;
            said.text = text;
            said.words = std::move(words);
            heard.partials.push_back(std::move(said));
            return;
        }

        core::AsrTurn said;
        said.start = start;
        said.end = end;
        said.text = text;
        said.confidence = readOr<double>(message, "end_of_turn_confidence", 0.0);
        said.words = std::move(words);
        said.boundary = core::AsrTurn::Boundary::Provider;

        if (const auto order = readOr<double>(message, "turn_order", -1.0); order >= 0.0) {
            said.ordinal = static_cast<unsigned>(order);
        }
        heard.turns.push_back(std::move(said));
    }

private:

    [[nodiscard]] static bool turnIsComplete(const nlohmann::json &message)
    {
        return readOr<bool>(message, "end_of_turn", false) &&
               readOr<bool>(message, "turn_is_formatted", false);
    }

    [[nodiscard]] static std::vector<core::AsrWord> wordsOf(const nlohmann::json &message)
    {
        std::vector<core::AsrWord> words;
        const auto found = message.find("words");
        if (found == message.end() || !found->is_array()) {
            return words;
        }
        words.reserve(found->size());
        for (const auto &one : *found) {
            if (!one.is_object()) {
                continue;
            }
            core::AsrWord word;
            word.text = firstString(one, "text", "word");

            word.start = fromMilliseconds(readOr<double>(one, "start", 0.0));
            word.end = fromMilliseconds(readOr<double>(one, "end", 0.0));
            word.confidence = readOr<double>(one, "confidence", 0.0);
            if (!word.text.empty()) {
                words.push_back(std::move(word));
            }
        }
        return words;
    }
};

}

std::unique_ptr<IStreamingProtocol> makeAssemblyAiStreaming()
{
    return std::make_unique<AssemblyAiStreaming>();
}

}
