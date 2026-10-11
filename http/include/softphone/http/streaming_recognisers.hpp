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

#include <softphone/core/streaming_asr.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {
class IWebSocketFactory;
}

namespace softphone::http {

[[nodiscard]] const std::vector<std::string> &streamingAsrTypes();

[[nodiscard]] bool canStream(std::string_view type);

[[nodiscard]] std::unique_ptr<core::IStreamingAsr>
makeStreamingAsr(std::string_view type, core::IWebSocketFactory &sockets);

}
