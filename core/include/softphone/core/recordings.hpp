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

#include <softphone/core/call.hpp>
#include <softphone/core/store.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

enum class RecordingSource {

    Native,

    External,

    Imported,
};

[[nodiscard]] std::string_view toString(RecordingSource source) noexcept;
[[nodiscard]] std::optional<RecordingSource> parseRecordingSource(std::string_view text) noexcept;

enum class ConsentPosture {

    Announce,

    Tone,

    Confirm,

    Silent,
};

[[nodiscard]] std::string_view toString(ConsentPosture posture) noexcept;
[[nodiscard]] std::optional<ConsentPosture> parseConsentPosture(std::string_view text) noexcept;

struct RecordingLeg {
    std::string callId;

    std::chrono::milliseconds joined{0};

    std::chrono::milliseconds left{0};

    std::string remoteNumber;
    std::string remoteUri;
    std::string remoteName;
};

struct RecordingStem {
    std::string id;
    std::string callId;

    std::string filePath;

    std::chrono::milliseconds offset{0};
    std::chrono::milliseconds duration{0};
    std::uint64_t bytes{0};

    std::string remoteNumber;
    std::string remoteName;

    std::chrono::system_clock::time_point droppedAt{};
};

struct Recording {
    std::string id;
    RecordingSource source{RecordingSource::Native};
    std::string accountId;

    std::string seanceId;

    CallDirection direction{CallDirection::Outgoing};

    std::chrono::system_clock::time_point startedAt{};
    std::chrono::milliseconds duration{0};

    unsigned partyCount{0};

    std::string remoteNumber;
    std::string remoteUri;
    std::string remoteName;

    std::string title;

    std::string filePath;

    std::string format{"mp3"};
    unsigned sampleRate{0};
    unsigned channels{2};

    std::string channelLayout{"near-far"};

    std::uint64_t bytes{0};

    bool hasGaps{false};

    bool unfinished{false};

    ConsentPosture consent{ConsentPosture::Announce};
    std::chrono::system_clock::time_point consentAt{};

    bool pinned{false};

    std::string categoryCode;
    std::string superSummary;

    std::string contentLanguage;

    unsigned redFlagCount{0};

    int score{0};

    unsigned transcriptCount{0};

    std::chrono::system_clock::time_point createdAt{};
    std::chrono::system_clock::time_point updatedAt{};

    std::chrono::system_clock::time_point deletedAt{};

    std::vector<RecordingLeg> legs;
    std::vector<RecordingStem> stems;

    std::vector<std::string> tags;

    std::vector<std::string> redFlags;

    [[nodiscard]] bool isDeleted() const noexcept
    {
        return deletedAt.time_since_epoch().count() != 0;
    }

    [[nodiscard]] bool hasAudio() const noexcept
    {
        return !isDeleted() && !filePath.empty();
    }
};

void keepWhatAPersonChanged(const Recording &stored, Recording &writing);

struct RecordingQuery {

    std::size_t limit{0};
    std::size_t offset{0};

    std::string numberKey;

    std::string callId;

    std::optional<RecordingSource> source;

    std::optional<std::chrono::system_clock::time_point> startedFrom;
    std::optional<std::chrono::system_clock::time_point> startedBefore;

    std::optional<CallDirection> direction;

    std::optional<bool> pinned;

    std::optional<bool> conference;

    std::string text;

    std::optional<bool> transcribed;

    std::optional<bool> processed;

    std::string language;

    std::string category;

    std::string tag;

    std::string redFlag;

    std::optional<bool> hasRedFlags;

    bool includeDeleted{false};
};

enum class RecordingAccess {
    Created,
    Played,
    Exported,
    Deleted,

    Expired,
};

[[nodiscard]] std::string_view toString(RecordingAccess what) noexcept;

struct AccessEntry {
    std::string id;
    std::string recordingId;
    RecordingAccess what{RecordingAccess::Played};

    std::string detail;

    std::chrono::system_clock::time_point at{};
};

class IRecordingStore {
public:
    IRecordingStore() = default;
    IRecordingStore(const IRecordingStore &) = delete;
    IRecordingStore &operator=(const IRecordingStore &) = delete;
    IRecordingStore(IRecordingStore &&) = delete;
    IRecordingStore &operator=(IRecordingStore &&) = delete;
    virtual ~IRecordingStore() = default;

    [[nodiscard]] virtual StoreStatus save(Recording &recording) = 0;

    [[nodiscard]] virtual StoreStatus recordings(const RecordingQuery &query,
                                                 std::vector<Recording> &out) = 0;

    [[nodiscard]] virtual StoreStatus recording(const std::string &id, Recording &out) = 0;

    [[nodiscard]] virtual StoreStatus removeRecording(const std::string &id,
                                                      std::vector<std::string> &files) = 0;

    [[nodiscard]] virtual StoreStatus dropAudio(const std::string &id,
                                                std::vector<std::string> &files) = 0;

    [[nodiscard]] virtual StoreStatus dropStems(const std::string &recordingId,
                                                std::vector<std::string> &files) = 0;

    [[nodiscard]] virtual StoreStatus
    noteAccess(const std::string &recordingId, RecordingAccess what, const std::string &detail) = 0;

    [[nodiscard]] virtual StoreStatus accessLog(const std::string &recordingId,
                                                std::vector<AccessEntry> &out) = 0;
};

}
