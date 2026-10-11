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
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

using WallClock = std::function<std::chrono::system_clock::time_point()>;

[[nodiscard]] WallClock defaultWallClock();

struct CallRecord {

    std::string id;

    std::chrono::system_clock::time_point startedAt{};

    std::chrono::seconds duration{0};

    int attempts{1};

    int recordingCount{0};

    std::int64_t sequence{0};

    std::string callId;

    std::string seanceId;

    CallDirection direction{CallDirection::Outgoing};

    bool answered{false};

    AutoAnswer autoAnswer{AutoAnswer::No};

    CallEndReason endReason{CallEndReason::None};

    std::string number;

    std::string remoteUri;

    std::string name;

    std::string accountId;
    std::string accountAddress;

    std::string dialedNumber;

    friend bool operator==(const CallRecord &, const CallRecord &) = default;
};

enum class CallOutcome {
    Answered,

    Missed,

    Declined,

    Failed,

    PickedUpElsewhere,
};

[[nodiscard]] CallOutcome outcome(const CallRecord &record) noexcept;

[[nodiscard]] std::span<const CallEndReason> reasonsMeaningMissed() noexcept;

[[nodiscard]] std::span<const CallEndReason> reasonsMeaningDeclined() noexcept;

[[nodiscard]] std::string_view toString(CallOutcome value) noexcept;

struct CallRecordContext {
    std::string accountId;
    std::string accountAddress;
    std::string name;
    std::chrono::system_clock::time_point startedAt{};

    std::chrono::steady_clock::time_point endedAt{};
};

[[nodiscard]] CallRecord callRecord(const CallInfo &call, const CallRecordContext &context);

[[nodiscard]] std::string redialTarget(const CallRecord &record);

struct CallLogCursor {
    std::chrono::system_clock::time_point startedAt{};
    std::int64_t sequence{0};

    friend bool operator==(const CallLogCursor &, const CallLogCursor &) = default;
};

struct CallLogQuery {

    int limit{100};

    int offset{0};

    std::optional<CallLogCursor> before;

    std::optional<CallDirection> direction;

    bool missedOnly{false};

    bool declinedOnly{false};

    std::string numberKey;

    std::string search;

    friend bool operator==(const CallLogQuery &, const CallLogQuery &) = default;
};

class ICallLog {
public:
    ICallLog() = default;
    ICallLog(const ICallLog &) = delete;
    ICallLog &operator=(const ICallLog &) = delete;
    ICallLog(ICallLog &&) = delete;
    ICallLog &operator=(ICallLog &&) = delete;
    virtual ~ICallLog() = default;

    [[nodiscard]] virtual StoreStatus record(CallRecord &record) = 0;

    [[nodiscard]] virtual StoreStatus calls(const CallLogQuery &query,
                                            std::vector<CallRecord> &out) = 0;

    [[nodiscard]] virtual StoreStatus removeCall(const std::string &id) = 0;

    [[nodiscard]] virtual StoreStatus clearCalls() = 0;

    [[nodiscard]] virtual StoreStatus
    removeCallsBefore(std::chrono::system_clock::time_point before, int &removed) = 0;
};

}
