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

#include <softphone/plugins/webui/words.hpp>

#include <softphone/core/recordings.hpp>
#include <softphone/core/transcripts.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::plugins::webui {

[[nodiscard]] std::string escape(std::string_view text);

[[nodiscard]] std::string asClock(std::chrono::milliseconds length);

[[nodiscard]] std::string asStamp(std::chrono::system_clock::time_point when, const Words &words);

[[nodiscard]] std::string listPage(const std::vector<core::Recording> &recordings,
                                   const Words &words);

[[nodiscard]] std::string recordingPage(const Words &words, const core::Recording &recording,
                                        const std::optional<core::Transcript> &transcript);

[[nodiscard]] std::string missingPage(const Words &words);

[[nodiscard]] std::string unavailablePage(const Words &words, std::string_view what);

}
