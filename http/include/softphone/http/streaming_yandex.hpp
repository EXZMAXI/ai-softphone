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

#include <softphone/core/grpc_stream.hpp>
#include <softphone/core/streaming_asr.hpp>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::http {

[[nodiscard]] std::unique_ptr<core::IStreamingAsr>
makeYandexStreaming(core::IGrpcTransport &transport);

namespace yandex {

[[nodiscard]] std::string sessionOptions(const core::AsrEndpoint &where,
                                         const core::StreamingOptions &options);

[[nodiscard]] std::string audioChunk(std::string_view pcm);

struct Heard {

    bool partial{false};

    bool settled{false};

    std::string text;
    std::string language;
    std::chrono::milliseconds start{0};
    std::chrono::milliseconds end{0};

    bool begun{false};
    double confidence{0.0};
    std::vector<core::AsrWord> words;

    std::string said;

    bool understood{false};

    std::string shape;

    std::optional<unsigned> refines;
};

[[nodiscard]] Heard readResponse(std::string_view message);

}

}
