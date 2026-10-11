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

#include <softphone/core/transcripts.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

enum class TranscriptFormat {

    Text,

    SubRip,

    WebVtt,
};

[[nodiscard]] std::string_view toString(TranscriptFormat format) noexcept;
[[nodiscard]] std::string_view extensionFor(TranscriptFormat format) noexcept;

struct TranscriptNames {

    std::string nearSide{"Near"};
    std::string farSide{"Far"};
    std::string mixed{"Mixed"};

    std::vector<std::pair<std::string, std::string>> stems;

    [[nodiscard]] std::string forVoice(std::string_view voice) const;
};

[[nodiscard]] std::string renderTranscript(const Transcript &transcript, TranscriptFormat format,
                                           const TranscriptNames &names = {});

}
