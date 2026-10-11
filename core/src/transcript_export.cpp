// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/transcript_export.hpp>

#include <algorithm>
#include <cstdio>
#include <string>

namespace softphone::core {
namespace {

using namespace std::chrono_literals;

[[nodiscard]] std::string stamp(std::chrono::milliseconds at, char point)
{
    if (at < 0ms) {
        at = 0ms;
    }
    const auto whole = at.count();
    const auto hours = whole / 3'600'000;
    const auto minutes = (whole / 60'000) % 60;
    const auto seconds = (whole / 1000) % 60;
    const auto rest = whole % 1000;

    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%02lld:%02lld:%02lld%c%03lld",
                  static_cast<long long>(hours), static_cast<long long>(minutes),
                  static_cast<long long>(seconds), point, static_cast<long long>(rest));
    return buffer;
}

[[nodiscard]] std::string shortStamp(std::chrono::milliseconds at)
{
    if (at < 0ms) {
        at = 0ms;
    }
    const auto whole = at.count() / 1000;
    char buffer[24];
    std::snprintf(buffer, sizeof buffer, "%02lld:%02lld", static_cast<long long>(whole / 60),
                  static_cast<long long>(whole % 60));
    return buffer;
}

[[nodiscard]] std::string lineOf(const TranscriptSegment &segment)
{
    const auto &said = segment.editedText.empty() ? segment.text : segment.editedText;
    const auto first = said.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = said.find_last_not_of(" \t\r\n");
    return said.substr(first, last - first + 1);
}

}

std::string_view toString(TranscriptFormat format) noexcept
{
    switch (format) {
    case TranscriptFormat::Text:
        return "text";
    case TranscriptFormat::SubRip:
        return "srt";
    case TranscriptFormat::WebVtt:
        return "vtt";
    }
    return "text";
}

std::string_view extensionFor(TranscriptFormat format) noexcept
{
    switch (format) {
    case TranscriptFormat::Text:
        return ".txt";
    case TranscriptFormat::SubRip:
        return ".srt";
    case TranscriptFormat::WebVtt:
        return ".vtt";
    }
    return ".txt";
}

std::string TranscriptNames::forVoice(std::string_view voice) const
{
    const auto parsed = parseVoice(voice);
    if (!parsed.has_value()) {
        return std::string{voice};
    }
    switch (parsed->first) {
    case Voice::Near:
        return nearSide;
    case Voice::Far:
        return farSide;
    case Voice::Mixed:
        return mixed;
    case Voice::Stem:
        break;
    }
    const auto named = std::find_if(stems.begin(), stems.end(),
                                    [&parsed](const std::pair<std::string, std::string> &entry) {
                                        return entry.first == parsed->second;
                                    });
    return named != stems.end() && !named->second.empty() ? named->second : farSide;
}

std::string renderTranscript(const Transcript &transcript, TranscriptFormat format,
                             const TranscriptNames &names)
{
    std::string out;

    if (format == TranscriptFormat::WebVtt) {
        out += "WEBVTT\n\n";
    }

    int numbered = 0;
    for (const auto &segment : transcript.segments) {
        const auto line = lineOf(segment);
        if (line.empty()) {

            continue;
        }
        auto who = names.forVoice(segment.voice);
        if (!segment.speaker.empty()) {
            who += " · " + segment.speaker;
        }

        switch (format) {
        case TranscriptFormat::Text:
            out += '[' + shortStamp(segment.start) + "] " + who + ": " + line + '\n';
            break;
        case TranscriptFormat::SubRip:
            out += std::to_string(++numbered) + '\n';
            out += stamp(segment.start, ',') + " --> " + stamp(segment.end, ',') + '\n';
            out += who + ": " + line + "\n\n";
            break;
        case TranscriptFormat::WebVtt:
            out += stamp(segment.start, '.') + " --> " + stamp(segment.end, '.') + '\n';
            out += "<v " + who + ">" + line + "\n\n";
            break;
        }
    }
    return out;
}

}
