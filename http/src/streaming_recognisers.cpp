// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/streaming_recognisers.hpp>

#include "streaming_protocols.hpp"
#include "streaming_session.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace softphone::http {
namespace {

struct Known {
    std::string_view type;
    ProtocolRecogniser::Make make;
};

[[nodiscard]] const std::vector<Known> &known()
{
    static const std::vector<Known> all{
        {"xai", &makeXaiStreaming},
        {"elevenlabs", &makeElevenLabsStreaming},
        {"deepgram", &makeDeepgramStreaming},
        {"openai", &makeOpenAiStreaming},
        {"speechmatics", &makeSpeechmaticsStreaming},
        {"soniox", &makeSonioxStreaming},
        {"assemblyai", &makeAssemblyAiStreaming},
        {"vosk", &makeVoskStreaming},
        {"whisper-live", &makeWhisperStreaming},
        {"riva", &makeRivaStreaming},
    };
    return all;
}

}

const std::vector<std::string> &streamingAsrTypes()
{
    static const std::vector<std::string> names = [] {
        std::vector<std::string> made;
        made.reserve(known().size());
        for (const auto &one : known()) {
            made.emplace_back(one.type);
        }
        return made;
    }();
    return names;
}

bool canStream(std::string_view type)
{

    return core::canStreamType(type);
}

std::unique_ptr<core::IStreamingAsr> makeStreamingAsr(std::string_view type,
                                                      core::IWebSocketFactory &sockets)
{

    const auto found =
        std::ranges::find_if(known(), [type](const Known &one) { return one.type == type; });
    if (found == known().end()) {
        return nullptr;
    }
    return std::make_unique<ProtocolRecogniser>(sockets, found->make);
}

}
