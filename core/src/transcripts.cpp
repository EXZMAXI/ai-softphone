// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/transcripts.hpp>

#include <softphone/core/turn_engine.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <utility>

namespace softphone::core {
namespace {

using namespace std::chrono_literals;

[[nodiscard]] std::string voiceOf(const AsrInput &input, const AsrSegment &segment)
{
    if (!input.channelVoices.empty() && segment.channel.has_value()) {
        const auto index = static_cast<std::size_t>(*segment.channel);
        if (index < input.channelVoices.size()) {
            return voiceText(input.channelVoices[index]);
        }
    }
    if (!input.channelVoices.empty()) {

        return voiceText(Voice::Mixed);
    }
    return voiceText(input.voice, input.stemId);
}

}

std::string_view toString(TranscriptStatus status) noexcept
{
    switch (status) {
    case TranscriptStatus::Queued:
        return "queued";
    case TranscriptStatus::Running:
        return "running";
    case TranscriptStatus::Done:
        return "done";
    case TranscriptStatus::Failed:
        return "failed";
    case TranscriptStatus::Cancelled:
        return "cancelled";
    }
    return "queued";
}

std::optional<TranscriptStatus> parseTranscriptStatus(std::string_view text) noexcept
{
    if (text == "queued") {
        return TranscriptStatus::Queued;
    }
    if (text == "running") {
        return TranscriptStatus::Running;
    }
    if (text == "done") {
        return TranscriptStatus::Done;
    }
    if (text == "failed") {
        return TranscriptStatus::Failed;
    }
    if (text == "cancelled") {
        return TranscriptStatus::Cancelled;
    }
    return std::nullopt;
}

std::vector<TranscriptSegment> mergeRuns(const std::vector<AsrRun> &runs)
{
    std::vector<TranscriptSegment> merged;

    for (const auto &run : runs) {
        for (const auto &came : run.result.segments) {
            TranscriptSegment segment;
            segment.start = came.start + run.input.offset;
            segment.end = std::max(came.end + run.input.offset, segment.start);
            segment.voice = voiceOf(run.input, came);
            segment.speaker = came.speaker;
            segment.text = came.text;
            segment.confidence = came.confidence;
            segment.words.reserve(came.words.size());
            for (const auto &word : came.words) {
                TranscriptWord shifted;
                shifted.start = word.start + run.input.offset;
                shifted.end = std::max(word.end + run.input.offset, shifted.start);
                shifted.text = word.text;
                shifted.confidence = word.confidence;
                segment.words.push_back(std::move(shifted));
            }
            merged.push_back(std::move(segment));
        }
    }

    std::stable_sort(merged.begin(), merged.end(),
                     [](const TranscriptSegment &left, const TranscriptSegment &right) {
                         if (left.start != right.start) {
                             return left.start < right.start;
                         }
                         if (left.end != right.end) {
                             return left.end < right.end;
                         }
                         return left.voice < right.voice;
                     });

    int ordinal = 0;
    for (auto &segment : merged) {
        segment.ordinal = ordinal++;
    }
    return merged;
}

std::string encodeWords(const std::vector<TranscriptWord> &words)
{
    if (words.empty()) {
        return {};
    }
    auto array = nlohmann::json::array();
    for (const auto &word : words) {
        array.push_back(nlohmann::json::array(
            {word.start.count(), word.end.count(), word.text, word.confidence}));
    }
    return array.dump();
}

std::vector<TranscriptWord> decodeWords(std::string_view text)
{
    std::vector<TranscriptWord> words;
    if (text.empty()) {
        return words;
    }
    const auto parsed = nlohmann::json::parse(text, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_array()) {
        return words;
    }
    for (const auto &entry : parsed) {
        if (!entry.is_array() || entry.size() < 3 || !entry[2].is_string()) {
            return {};
        }
        TranscriptWord word;
        word.start = std::chrono::milliseconds{entry[0].get<std::int64_t>()};
        word.end = std::chrono::milliseconds{entry[1].get<std::int64_t>()};
        word.text = entry[2].get<std::string>();
        if (entry.size() > 3 && entry[3].is_number()) {
            word.confidence = entry[3].get<double>();
        }
        words.push_back(std::move(word));
    }
    return words;
}

std::size_t countWords(const std::vector<TranscriptSegment> &segments)
{
    std::size_t words = 0;
    for (const auto &segment : segments) {
        bool inside = false;
        for (const auto letter : segment.text) {
            const auto blank = std::isspace(static_cast<unsigned char>(letter)) != 0;
            if (!blank && !inside) {
                ++words;
            }
            inside = !blank;
        }
    }
    return words;
}

Transcript asRealtimeTranscript(const std::string &recordingId, const std::vector<AsrTurn> &replies,
                                const std::string &providerId, const std::string &providerType,
                                const std::string &model, const std::string &language)
{
    Transcript reading;
    reading.recordingId = recordingId;
    reading.providerId = providerId;
    reading.providerType = providerType;
    reading.model = model;
    reading.requestedLanguage = language;
    reading.status = TranscriptStatus::Done;

    reading.isPrimary = false;
    reading.source = "realtime";
    reading.requestedAt = std::chrono::system_clock::now();
    reading.completedAt = reading.requestedAt;

    auto inOrder = replies;
    std::stable_sort(inOrder.begin(), inOrder.end(),
                     [](const AsrTurn &a, const AsrTurn &b) { return a.start < b.start; });

    int ordinal = 0;
    for (const auto &one : inOrder) {
        TranscriptSegment segment;
        segment.ordinal = ordinal++;
        segment.start = one.start;
        segment.end = one.end;

        segment.voice = voiceText(one.voice);
        segment.text = one.text;
        segment.confidence = one.confidence;
        reading.wordCount += wordsIn(one.text);
        reading.segments.push_back(std::move(segment));
    }

    reading.audio = std::chrono::milliseconds{0};
    for (const auto &one : inOrder) {
        reading.audio = std::max(reading.audio, one.end);
    }
    return reading;
}

}
