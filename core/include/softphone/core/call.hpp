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

#include <softphone/core/account_config.hpp>
#include <softphone/core/auto_answer.hpp>

#include <chrono>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

enum class CallDirection { Outgoing, Incoming };

[[nodiscard]] bool isDtmfDigit(char digit) noexcept;

enum class AnswerMode {

    HoldOthers,

    EndOthers,
};

enum class CallState {
    Idle,

    Dialing,

    RingingOut,

    RingingIn,

    Active,
    Ended,
};

enum class CallEndReason {
    None,

    LocalHangup,
    RemoteHangup,

    Replaced,

    Transferred,
    Busy,
    Declined,

    DoNotDisturb,

    TooManyCalls,

    CallWaitingOff,

    PickedUpElsewhere,

    NoAnswer,

    Cancelled,

    Unavailable,
    NumberNotFound,
    NotAllowed,

    MediaMismatch,
    ServerError,
    Unreachable,

    NoAudioDevice,

    Failed,
};

enum class LocalFailure {
    None,

    NoAudioDevice,
};

enum class TransferState {
    None,

    Requested,

    Ringing,

    Succeeded,

    Failed,
};

struct TransferInfo {
    TransferState state{TransferState::None};

    std::string target;

    std::string referredBy;

    int sipStatus{0};

    std::string otherCallId;

    bool askedByThem{false};

    bool continuation{false};

    [[nodiscard]] bool inFlight() const noexcept
    {
        return state == TransferState::Requested || state == TransferState::Ringing;
    }

    [[nodiscard]] bool switchNeverAnswered() const noexcept
    {
        return state == TransferState::Failed && sipStatus == 0;
    }

    friend bool operator==(const TransferInfo &, const TransferInfo &) = default;
};

struct CallEndInfo {
    CallEndReason reason{CallEndReason::None};
    int sipStatus{0};
    std::string detail;

    friend bool operator==(const CallEndInfo &, const CallEndInfo &) = default;
};

struct CallInfo {
    std::string id;

    std::string uuid;

    std::string seanceId;
    std::string accountId;
    CallDirection direction{CallDirection::Outgoing};

    std::string remoteUri;
    std::string remoteDisplayName;

    std::string dialedNumber;

    CallState state{CallState::Idle};
    CallEndInfo end;
    bool muted{false};

    bool heldByUs{false};
    bool heldByThem{false};

    bool earlyMedia{false};

    bool farEndRinging{false};

    bool fromThisLine{false};

    TransferInfo transfer;

    AutoAnswer autoAnswer{AutoAnswer::No};

    std::chrono::seconds autoAnswerAfter{0};

    [[nodiscard]] bool answeredAutomatically() const noexcept
    {
        return autoAnswer != AutoAnswer::No;
    }

    bool inConference{false};

    [[nodiscard]] bool canTransfer() const noexcept
    {
        return state == CallState::Active && !transfer.inFlight() && !inConference;
    }

    [[nodiscard]] bool holdsTheAudio() const noexcept
    {
        return state == CallState::Active && !heldByUs;
    }

    std::optional<std::chrono::steady_clock::time_point> connectedAt;

    std::chrono::system_clock::time_point wallStartedAt{};

    std::chrono::system_clock::time_point wallStateSince{};

    int redialAttempt{0};

    std::string redialContinues;

    std::optional<std::chrono::steady_clock::time_point> startedAt;

    [[nodiscard]] bool everConnected() const noexcept
    {
        return connectedAt.has_value();
    }

    [[nodiscard]] bool canSendDtmf() const noexcept
    {
        return state == CallState::Active && !heldByUs && !heldByThem && !inConference;
    }

    [[nodiscard]] std::string remoteLabel() const;

    [[nodiscard]] std::string remoteDetail() const;

    friend bool operator==(const CallInfo &, const CallInfo &) = default;
};

[[nodiscard]] std::string_view standingOf(const CallInfo &call) noexcept;

[[nodiscard]] bool sameStanding(const CallInfo &before, const CallInfo &after) noexcept;

struct SipAddress {
    std::string displayName;
    std::string uri;

    friend bool operator==(const SipAddress &, const SipAddress &) = default;
};

[[nodiscard]] SipAddress parseSipAddress(std::string_view address);

[[nodiscard]] std::string sipUserPart(std::string_view uri);

[[nodiscard]] std::string dialable(std::string_view typed);

[[nodiscard]] std::string dialTarget(std::string_view input, const AccountConfig &account);

[[nodiscard]] bool somebodyElseTransferred(std::string_view referredBy,
                                           const AccountConfig &account);

[[nodiscard]] bool saysCompletedElsewhere(std::span<const std::string> reasons) noexcept;

[[nodiscard]] CallEndReason classifyCallEnd(int sipStatus, bool endedLocally,
                                            bool completedElsewhere,
                                            LocalFailure local = LocalFailure::None) noexcept;

inline constexpr std::chrono::seconds kSpokenAtAll{1};

[[nodiscard]] bool talkingToOurselves(const CallInfo &call, std::string_view ourNumber) noexcept;

[[nodiscard]] bool carriesTheTransferInto(const CallInfo &call,
                                          std::string_view newCallId) noexcept;

[[nodiscard]] bool continuedElsewhere(const CallInfo &call,
                                      std::chrono::steady_clock::time_point endedAt,
                                      std::string_view ourNumber = {}) noexcept;

[[nodiscard]] bool onTheLine(const CallInfo &call) noexcept;

[[nodiscard]] std::vector<CallInfo> transferCandidates(std::span<const CallInfo> calls,
                                                       const std::string &callId);

[[nodiscard]] std::vector<CallInfo> conferenceCandidates(std::span<const CallInfo> calls);

[[nodiscard]] std::optional<CallInfo> callToTalk(std::span<const CallInfo> calls,
                                                 std::string_view accountId);

[[nodiscard]] std::optional<CallInfo> callToHold(std::span<const CallInfo> calls,
                                                 std::string_view accountId);

[[nodiscard]] bool aCallIsUp(std::span<const CallInfo> calls) noexcept;

[[nodiscard]] std::string_view toString(CallState state) noexcept;
[[nodiscard]] std::string_view toString(CallEndReason reason) noexcept;
[[nodiscard]] std::string_view toString(TransferState state) noexcept;
[[nodiscard]] std::string_view toString(CallDirection direction) noexcept;

[[nodiscard]] std::optional<CallDirection> parseCallDirection(std::string_view text) noexcept;
[[nodiscard]] std::optional<CallEndReason> parseCallEndReason(std::string_view text) noexcept;
}
