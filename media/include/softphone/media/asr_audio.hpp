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

#include <softphone/core/asr_plan.hpp>
#include <softphone/core/streamed_transcription.hpp>

#include <cstdint>
#include <filesystem>
#include <vector>

namespace softphone::media {

[[nodiscard]] core::PreparedAudio prepareAsrAudio(const core::AsrInput &input,
                                                  const std::filesystem::path &scratch);

[[nodiscard]] core::AudioPreparer makeAsrPreparer(std::filesystem::path scratch);

[[nodiscard]] bool readPcmForAsr(const std::filesystem::path &file, unsigned rate,
                                 std::vector<std::int16_t> &mono);

[[nodiscard]] core::PcmReader makePcmReader();

}
