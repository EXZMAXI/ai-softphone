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
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace softphone::core {

struct SpeechSpan {

    std::optional<unsigned> channel;

    std::chrono::milliseconds from{0};
    std::chrono::milliseconds to{0};

    [[nodiscard]] std::chrono::milliseconds length() const noexcept
    {
        return to > from ? to - from : std::chrono::milliseconds::zero();
    }

    [[nodiscard]] bool operator==(const SpeechSpan &) const = default;
};

inline constexpr int kSpeechAboveFloor = 25;

inline constexpr auto kPhraseSilence = std::chrono::milliseconds{600};

inline constexpr auto kPhrasePadding = std::chrono::milliseconds{200};

inline constexpr auto kShortestPiece = std::chrono::milliseconds{1200};

inline constexpr auto kLongestPiece = std::chrono::seconds{20};

[[nodiscard]] std::vector<SpeechSpan> speechIn(std::span<const std::uint8_t> loudness,
                                               std::chrono::milliseconds over,
                                               std::optional<unsigned> channel);

[[nodiscard]] std::vector<SpeechSpan> piecesOfSpeech(std::span<const SpeechSpan> phrases,
                                                     std::chrono::milliseconds longest,
                                                     std::chrono::milliseconds endOfRecording);

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

        std::vector<SpeechSpan> speech;
    };

    std::vector<Stem> stems;

    std::vector<SpeechSpan> speech;
};

struct AsrStep {
    AsrInput input;
    AsrOptions options;
};

struct AsrPlan {
    std::vector<AsrStep> steps;

    bool cutAtSilences{false};

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

using SpeechFinder = std::function<std::vector<SpeechSpan>(const std::filesystem::path &,
                                                           std::chrono::milliseconds)>;

using AudioPreparer = std::function<PreparedAudio(const AsrInput &)>;

[[nodiscard]] AsrPlan planTranscription(const AsrCapabilities &can, const AsrSubject &subject,
                                        const AsrOptions &wanted);

}
