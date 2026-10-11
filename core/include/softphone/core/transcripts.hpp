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
#include <softphone/core/store.hpp>
#include <softphone/core/streaming_asr.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

enum class TranscriptStatus {
    Queued,
    Running,
    Done,
    Failed,
    Cancelled,
};

[[nodiscard]] std::string_view toString(TranscriptStatus status) noexcept;
[[nodiscard]] std::optional<TranscriptStatus> parseTranscriptStatus(std::string_view text) noexcept;

struct TranscriptWord {
    std::chrono::milliseconds start{0};
    std::chrono::milliseconds end{0};
    std::string text;
    double confidence{0.0};
};

struct TranscriptSegment {
    int ordinal{0};
    std::chrono::milliseconds start{0};
    std::chrono::milliseconds end{0};

    std::string voice;

    std::string speaker;

    std::string text;

    std::string editedText;

    double confidence{0.0};

    std::vector<TranscriptWord> words;
};

struct Transcript {
    std::string id;
    std::string recordingId;

    std::string providerId;

    std::string providerType;
    std::string model;

    std::string requestedLanguage;
    std::string detectedLanguage;

    TranscriptStatus status{TranscriptStatus::Queued};
    bool isPrimary{false};

    std::string source{"batch"};

    std::chrono::system_clock::time_point requestedAt{};
    std::chrono::system_clock::time_point completedAt{};
    std::string error;

    std::size_t wordCount{0};

    std::chrono::milliseconds audio{0};

    std::int64_t costMicros{0};

    std::string rawPath;

    std::vector<TranscriptSegment> segments;
};

struct AsrRun {
    AsrInput input;
    AsrResult result;
};

[[nodiscard]] std::vector<TranscriptSegment> mergeRuns(const std::vector<AsrRun> &runs);

[[nodiscard]] Transcript
asRealtimeTranscript(const std::string &recordingId, const std::vector<AsrTurn> &replies,
                     const std::string &providerId, const std::string &providerType,
                     const std::string &model, const std::string &language);

[[nodiscard]] std::string encodeWords(const std::vector<TranscriptWord> &words);

[[nodiscard]] std::vector<TranscriptWord> decodeWords(std::string_view text);

[[nodiscard]] std::size_t countWords(const std::vector<TranscriptSegment> &segments);

struct TranscriptSearch {

    std::string words;

    std::string recordingId;

    std::size_t limit{200};
};

struct TranscriptHit {
    std::string recordingId;
    std::string transcriptId;
    int ordinal{0};
    std::chrono::milliseconds start{0};
    std::string voice;

    std::string text;
};

class ITranscriptStore {
public:
    ITranscriptStore() = default;
    ITranscriptStore(const ITranscriptStore &) = delete;
    ITranscriptStore &operator=(const ITranscriptStore &) = delete;
    ITranscriptStore(ITranscriptStore &&) = delete;
    ITranscriptStore &operator=(ITranscriptStore &&) = delete;
    virtual ~ITranscriptStore() = default;

    [[nodiscard]] virtual StoreStatus save(Transcript &transcript) = 0;

    [[nodiscard]] virtual StoreStatus transcripts(const std::string &recordingId,
                                                  std::vector<Transcript> &out) = 0;

    [[nodiscard]] virtual StoreStatus transcript(const std::string &id, Transcript &out) = 0;

    [[nodiscard]] virtual StoreStatus primaryTranscript(const std::string &recordingId,
                                                        Transcript &out) = 0;

    [[nodiscard]] virtual StoreStatus makePrimary(const std::string &id) = 0;

    [[nodiscard]] virtual StoreStatus removeTranscript(const std::string &id,
                                                       std::vector<std::string> &files) = 0;

    [[nodiscard]] virtual StoreStatus search(const TranscriptSearch &query,
                                             std::vector<TranscriptHit> &out) = 0;

    [[nodiscard]] virtual StoreStatus transcriptLanguages(std::vector<std::string> &languages) = 0;
};

inline constexpr char kHitOpen = '\x02';
inline constexpr char kHitClose = '\x03';

}
