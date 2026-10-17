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

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::core {

enum class Voice {

    Near,

    Far,

    Mixed,

    Stem,
};

[[nodiscard]] std::string_view toString(Voice voice) noexcept;

[[nodiscard]] std::string_view speakerWord(Voice voice, std::string_view language) noexcept;

[[nodiscard]] std::string voiceText(Voice voice, std::string_view stemId = {});

[[nodiscard]] std::optional<std::pair<Voice, std::string>> parseVoice(std::string_view text);

struct AsrCapabilities {

    bool multichannel{false};

    bool diarisation{false};

    bool wordTimestamps{false};

    bool languageHint{true};

    bool detectsLanguage{true};

    std::vector<std::string> formats{"audio/mpeg"};

    std::uint64_t maxBytes{0};
    std::chrono::seconds maxDuration{0};

    unsigned maxConcurrent{1};

    bool answersWithTimes{true};
};

struct AsrInput {
    std::filesystem::path file;
    std::string mimeType{"audio/mpeg"};

    Voice voice{Voice::Mixed};

    std::string stemId;

    std::chrono::milliseconds offset{0};

    std::chrono::milliseconds from{0};
    std::chrono::milliseconds duration{0};

    std::optional<unsigned> sourceChannel;

    unsigned channels{1};

    std::vector<Voice> channelVoices;

    bool rewrite{false};
};

struct AsrEndpoint {
    std::string url;
    std::string token;
    std::string model;

    std::string region;

    std::vector<std::pair<std::string, std::string>> params;

    std::chrono::seconds timeout{std::chrono::minutes{20}};
};

inline constexpr auto kAsrTimeoutFloor = std::chrono::minutes{20};

inline constexpr int kAsrTimeoutFactor = 4;

inline constexpr auto kAsrTimeoutCeiling = std::chrono::hours{4};

[[nodiscard]] inline std::chrono::seconds asrTimeoutFor(std::chrono::milliseconds audio,
                                                        std::chrono::seconds chosen = {})
{
    if (chosen > std::chrono::seconds::zero()) {
        return chosen;
    }

    const auto scaled = std::chrono::duration_cast<std::chrono::seconds>(audio) * kAsrTimeoutFactor;
    const auto floor = std::chrono::duration_cast<std::chrono::seconds>(kAsrTimeoutFloor);
    const auto ceiling = std::chrono::duration_cast<std::chrono::seconds>(kAsrTimeoutCeiling);
    return std::min(ceiling, std::max(floor, scaled));
}

enum class AsrAnswerShape {

    Rich,

    Diarised,

    Plain,
};

[[nodiscard]] constexpr bool timesComeBack(AsrAnswerShape shape) noexcept
{
    return shape != AsrAnswerShape::Plain;
}

[[nodiscard]] AsrAnswerShape answerShapeFor(std::string_view type, std::string_view model);

[[nodiscard]] std::chrono::seconds longestAtOnceFor(std::string_view type, std::string_view model);

struct AsrOptions {

    std::string language;

    bool diarise{false};

    bool wordTimestamps{true};
};

struct AsrWord {
    std::chrono::milliseconds start{0};
    std::chrono::milliseconds end{0};
    std::string text;
    double confidence{0.0};
};

struct AsrSegment {
    std::chrono::milliseconds start{0};
    std::chrono::milliseconds end{0};

    std::string speaker;

    std::optional<unsigned> channel;

    std::string text;
    double confidence{0.0};

    std::vector<AsrWord> words;
};

enum class AsrStatus {
    Ok,

    Unauthorised,

    Unsupported,

    TooLarge,

    Unavailable,
    Timeout,

    Malformed,

    Cancelled,
    Failed,
};

[[nodiscard]] std::string_view toString(AsrStatus status) noexcept;

[[nodiscard]] constexpr bool succeeded(AsrStatus status) noexcept
{
    return status == AsrStatus::Ok;
}

[[nodiscard]] bool worthRetrying(AsrStatus status) noexcept;

struct AsrResult {
    AsrStatus status{AsrStatus::Ok};

    std::string error;

    std::string detectedLanguage;
    std::vector<AsrSegment> segments;

    std::string raw;

    std::chrono::milliseconds audio{0};

    std::int64_t costMicros{0};

    std::string model;

    std::vector<std::pair<std::string, std::string>> learnt;

    bool timed{true};
};

using AsrCancel = std::function<bool()>;

struct AsrProbe {
    bool reachable{false};
    AsrStatus status{AsrStatus::Ok};

    std::string detail;

    std::vector<std::string> models;
};

class IAsrProvider {
public:
    IAsrProvider() = default;
    IAsrProvider(const IAsrProvider &) = delete;
    IAsrProvider &operator=(const IAsrProvider &) = delete;
    IAsrProvider(IAsrProvider &&) = delete;
    IAsrProvider &operator=(IAsrProvider &&) = delete;
    virtual ~IAsrProvider() = default;

    [[nodiscard]] virtual std::string_view type() const noexcept = 0;

    [[nodiscard]] virtual AsrCapabilities capabilities(const AsrEndpoint &where) const = 0;

    [[nodiscard]] virtual AsrResult transcribe(const AsrEndpoint &where, const AsrInput &input,
                                               const AsrOptions &options,
                                               const AsrCancel &cancelled) = 0;

    [[nodiscard]] virtual AsrProbe probe(const AsrEndpoint &where) = 0;
};

}
