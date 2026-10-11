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

#include <softphone/core/settings.hpp>

#include <softphone/plugins/capture/control.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::ui {

[[nodiscard]] std::string groupText(const core::StoredConfiguration &configuration,
                                    std::string_view name);

void putGroup(core::StoredConfiguration &configuration, std::string_view name, std::string text,
              const std::string &owner);

inline constexpr std::string_view kPipelineGroup = "pipeline";

struct PipelineSettings {

    bool automatic{false};

    std::int64_t monthlyCeilingMicros{0};

    std::int64_t monthlyCeilingTokens{0};

    std::string currency;

    friend bool operator==(const PipelineSettings &, const PipelineSettings &) = default;
};

[[nodiscard]] std::string asText(const PipelineSettings &settings);
[[nodiscard]] PipelineSettings pipelineFrom(const std::string &text);

inline constexpr std::string_view kRealtimeGroup = "realtime";

inline constexpr int kLeastColumnTextPixels = 13;
inline constexpr int kMostColumnTextPixels = 40;

inline constexpr int kLeastRepeatTextPixels = 15;
inline constexpr int kMostRepeatTextPixels = 50;

struct RealtimeSettings {

    std::int64_t monthlyRecogniserMicros{0};

    std::int64_t monthlyModelMicros{0};

    std::string currency;

    std::string recogniserId;

    std::string modelId;

    bool offered{false};

    int columnTextPixels{13};

    bool repeatTheLast{true};

    int repeatTextPixels{20};

    friend bool operator==(const RealtimeSettings &, const RealtimeSettings &) = default;
};

[[nodiscard]] std::string asText(const RealtimeSettings &settings);
[[nodiscard]] RealtimeSettings realtimeFrom(const std::string &text);

inline constexpr std::string_view kCaptureGroup = "capture";

inline constexpr unsigned kMaxCapturePreRollSeconds = 300;

struct CaptureSettings {

    bool enabled{false};

    plugins::capture::OnFinding onFinding{plugins::capture::OnFinding::Ask};

    std::vector<plugins::capture::ApplicationPolicy> perApplication;

    unsigned preRollSeconds{15};

    bool remind{true};

    std::string outputDevice;
    std::string inputDevice;

    core::Visualisation pageDrawing{core::Visualisation::Wave};

    core::Visualisation stripDrawing{core::Visualisation::Levels};

    [[nodiscard]] bool operator==(const CaptureSettings &) const = default;
};

[[nodiscard]] std::string asText(const CaptureSettings &settings);
[[nodiscard]] CaptureSettings captureFrom(const std::string &text);

inline constexpr std::string_view kTranscriptionGroup = "transcription";

struct TranscriptionSettings {

    bool automatic{false};

    std::string language;

    friend bool operator==(const TranscriptionSettings &, const TranscriptionSettings &) = default;
};

[[nodiscard]] std::string asText(const TranscriptionSettings &settings);
[[nodiscard]] TranscriptionSettings transcriptionFrom(const std::string &text);

[[nodiscard]] plugins::capture::ICaptureControl::Settings
asControllerSettings(const CaptureSettings &settings, std::uint32_t processId);

}
