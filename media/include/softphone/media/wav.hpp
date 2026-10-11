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

#include <softphone/media/mp3.hpp>

#include <cstdint>
#include <filesystem>
#include <vector>

namespace softphone::media {

struct WavInfo {
    unsigned sampleRate{0};
    unsigned channels{0};

    std::uint64_t frames{0};
};

[[nodiscard]] DecoderStatus decodeWav(const std::filesystem::path &file,
                                      std::vector<std::int16_t> &interleaved, WavInfo &info);

[[nodiscard]] DecoderStatus inspectWav(const std::filesystem::path &file, WavInfo &info);

}
