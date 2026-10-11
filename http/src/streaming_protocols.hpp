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

#include "streaming_session.hpp"

#include <memory>

namespace softphone::http {

[[nodiscard]] std::unique_ptr<IStreamingProtocol> makeDeepgramStreaming();
[[nodiscard]] std::unique_ptr<IStreamingProtocol> makeXaiStreaming();
[[nodiscard]] std::unique_ptr<IStreamingProtocol> makeElevenLabsStreaming();
[[nodiscard]] std::unique_ptr<IStreamingProtocol> makeOpenAiStreaming();
[[nodiscard]] std::unique_ptr<IStreamingProtocol> makeSpeechmaticsStreaming();
[[nodiscard]] std::unique_ptr<IStreamingProtocol> makeSonioxStreaming();
[[nodiscard]] std::unique_ptr<IStreamingProtocol> makeVoskStreaming();
[[nodiscard]] std::unique_ptr<IStreamingProtocol> makeWhisperStreaming();
[[nodiscard]] std::unique_ptr<IStreamingProtocol> makeRivaStreaming();
[[nodiscard]] std::unique_ptr<IStreamingProtocol> makeAssemblyAiStreaming();

}
