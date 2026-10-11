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

#include <softphone/core/asr.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace softphone::core {

struct AsrSubject {
    std::filesystem::path file;
    std::string mimeType{"audio/mpeg"};

    unsigned channels{2};

    bool nearFar{true};

    std::chrono::milliseconds duration{0};
    std::uint64_t bytes{0};

    unsigned parties{2};

    struct Stem {
        std::string id;
        std::filesystem::path file;
        std::chrono::milliseconds offset{0};
        std::chrono::milliseconds duration{0};
        std::uint64_t bytes{0};

        bool present{true};
    };

    std::vector<Stem> stems;
};

struct AsrStep {
    AsrInput input;
    AsrOptions options;
};

struct AsrPlan {
    std::vector<AsrStep> steps;

    std::string refusal;

    [[nodiscard]] bool usable() const noexcept
    {
        return refusal.empty() && !steps.empty();
    }
};

struct PreparedAudio {
    std::filesystem::path file;

    bool temporary{false};

    std::string error;
};

using AudioPreparer = std::function<PreparedAudio(const AsrInput &)>;

[[nodiscard]] AsrPlan planTranscription(const AsrCapabilities &can, const AsrSubject &subject,
                                        const AsrOptions &wanted);

}
