// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "streaming_realtime_events.hpp"

#include "streaming_base64.hpp"

namespace softphone::http::streaming {

std::string appendAudio(std::span<const std::byte> audio)
{
    std::string out;
    out.reserve(audio.size() * 4 / 3 + 64);
    out += R"({"type":"input_audio_buffer.append","audio":")";
    out += asBase64(audio);
    out += R"("})";
    return out;
}

std::string base64Frame(std::string_view before, std::span<const std::byte> audio,
                        std::string_view after)
{
    std::string out;
    out.reserve(audio.size() * 4 / 3 + before.size() + after.size() + 8);
    out += before;
    out += asBase64(audio);
    out += after;
    return out;
}

}
