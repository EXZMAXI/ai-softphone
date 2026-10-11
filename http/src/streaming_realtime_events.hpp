// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#pragma once

#include "streaming_json.hpp"
#include "streaming_session.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

namespace softphone::http::streaming {

struct RealtimeGrowing {
    std::string said;

    std::size_t handshakes{1};

    std::size_t refused{0};

    bool configured{false};
};

constexpr std::string_view kOurHandshake = "softphone-configure-";

inline bool readRealtimeEvent(const nlohmann::json &message, StreamingHeard &heard,
                              RealtimeGrowing &growing)
{
    const auto kind = readOr<std::string>(message, "type", {});
    heard.shape = kind;

    if (kind == "error" || kind == "conversation.item.input_audio_transcription.failed") {
        heard.understood = true;

        const auto what = message.find("error");

        const auto about = what != message.end() && what->is_object()
                               ? readOr<std::string>(*what, "event_id", {})
                               : readOr<std::string>(message, "event_id", {});
        if (about.rfind(kOurHandshake, 0) == 0) {
            ++growing.refused;
            if (growing.configured || growing.refused < growing.handshakes) {
                heard.said = what != message.end() && what->is_object()
                                 ? firstString(*what, "message", "code")
                                 : firstString(message, "message", "code");
                heard.remark = true;
                return true;
            }
        }
        heard.said = what != message.end() && what->is_object()
                         ? firstString(*what, "message", "code")
                         : firstString(message, "message", "code");

        const auto code = what != message.end() && what->is_object()
                              ? readOr<std::string>(*what, "code", {})
                              : readOr<std::string>(message, "code", {});
        heard.fatal = code == "invalid_request_error" || code == "unsupported_value"
                          ? core::AsrStatus::Unsupported
                          : core::AsrStatus::Failed;
        if (heard.said.empty()) {
            heard.said = "the recogniser refused the session";
        }
        return true;
    }

    if (kind == "conversation.item.input_audio_transcription.delta") {
        heard.understood = true;

        const auto added = readOr<std::string>(message, "delta", {});
        if (!added.empty()) {
            growing.said += added;
        } else {
            const auto whole = firstString(message, "transcript", "text");
            if (!whole.empty()) {
                growing.said = whole;
            }
        }

        if (!growing.said.empty()) {
            core::AsrPartial said;
            said.text = growing.said;
            heard.partials.push_back(std::move(said));
        }
        return true;
    }

    if (kind == "conversation.item.input_audio_transcription.completed") {
        heard.understood = true;

        const auto text = firstString(message, "transcript", "text");
        growing.said.clear();
        if (!text.empty()) {
            core::AsrTurn said;
            said.text = text;
            said.boundary = core::AsrTurn::Boundary::Provider;
            heard.turns.push_back(std::move(said));
        }
        return true;
    }

    if (kind == "input_audio_buffer.committed" || kind == "input_audio_buffer.cleared" ||
        kind == "input_audio_buffer.speech_started") {
        heard.understood = true;
        growing.said.clear();
        return true;
    }

    if (kind == "transcription_session.updated" || kind == "session.updated" ||
        kind == "transcription_session.created" || kind == "session.created") {
        heard.understood = true;
        growing.configured = true;
        return true;
    }

    if (kind.rfind("transcription_session.", 0) == 0 || kind.rfind("session.", 0) == 0 ||
        kind.rfind("input_audio_buffer.", 0) == 0 || kind.rfind("conversation.", 0) == 0 ||
        kind == "rate_limits.updated") {
        heard.understood = true;
        return true;
    }

    return false;
}

[[nodiscard]] std::string appendAudio(std::span<const std::byte> audio);

[[nodiscard]] std::string base64Frame(std::string_view before, std::span<const std::byte> audio,
                                      std::string_view after);

}
