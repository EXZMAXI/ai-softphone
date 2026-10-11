// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/call_log.hpp>

#include <algorithm>
#include <array>

namespace softphone::core {

WallClock defaultWallClock()
{
    return []() { return std::chrono::system_clock::now(); };
}

CallOutcome outcome(const CallRecord &record) noexcept
{
    if (record.answered) {
        return CallOutcome::Answered;
    }

    switch (record.endReason) {
    case CallEndReason::Declined:

        return CallOutcome::Declined;
    case CallEndReason::DoNotDisturb:
    case CallEndReason::TooManyCalls:
    case CallEndReason::CallWaitingOff:

        return CallOutcome::Declined;
    case CallEndReason::Busy:

        return record.direction == CallDirection::Outgoing ? CallOutcome::Declined
                                                           : CallOutcome::Failed;
    case CallEndReason::LocalHangup:

        return record.direction == CallDirection::Incoming ? CallOutcome::Declined
                                                           : CallOutcome::Missed;
    case CallEndReason::PickedUpElsewhere:

        return CallOutcome::PickedUpElsewhere;
    case CallEndReason::RemoteHangup:
    case CallEndReason::Cancelled:
    case CallEndReason::NoAnswer:
    case CallEndReason::Transferred:
    case CallEndReason::Replaced:
    case CallEndReason::None:

        return CallOutcome::Missed;
    case CallEndReason::Unavailable:

        return CallOutcome::Declined;
    case CallEndReason::NumberNotFound:
    case CallEndReason::NotAllowed:
    case CallEndReason::MediaMismatch:
    case CallEndReason::ServerError:
    case CallEndReason::Unreachable:
    case CallEndReason::NoAudioDevice:
    case CallEndReason::Failed:
        return CallOutcome::Failed;
    }
    return CallOutcome::Failed;
}

std::span<const CallEndReason> reasonsMeaningMissed() noexcept
{

    static constexpr std::array kReasons{
        CallEndReason::RemoteHangup, CallEndReason::Cancelled, CallEndReason::NoAnswer,
        CallEndReason::Transferred,  CallEndReason::Replaced,  CallEndReason::None,
    };
    return kReasons;
}

std::span<const CallEndReason> reasonsMeaningDeclined() noexcept
{

    static constexpr std::array kReasons{
        CallEndReason::Declined,     CallEndReason::LocalHangup,  CallEndReason::Unavailable,
        CallEndReason::DoNotDisturb, CallEndReason::TooManyCalls, CallEndReason::CallWaitingOff,
    };
    return kReasons;
}

std::string_view toString(CallOutcome value) noexcept
{
    switch (value) {
    case CallOutcome::Answered:
        return "answered";
    case CallOutcome::Missed:
        return "missed";
    case CallOutcome::Declined:
        return "declined";
    case CallOutcome::Failed:
        return "failed";
    case CallOutcome::PickedUpElsewhere:
        return "picked-up-elsewhere";
    }
    return "unknown";
}

std::string redialTarget(const CallRecord &record)
{
    if (record.direction == CallDirection::Outgoing) {
        return record.dialedNumber.empty() ? record.number : record.dialedNumber;
    }
    return record.remoteUri.empty() ? record.number : record.remoteUri;
}

CallRecord callRecord(const CallInfo &call, const CallRecordContext &context)
{
    CallRecord record;

    record.callId = call.uuid;
    record.seanceId = call.seanceId;
    record.startedAt = context.startedAt;
    record.direction = call.direction;
    record.answered = call.everConnected();

    record.autoAnswer = call.everConnected() ? call.autoAnswer : AutoAnswer::No;
    record.endReason = call.end.reason;
    record.remoteUri = call.remoteUri;
    record.number = sipUserPart(call.remoteUri);
    record.accountId = context.accountId;
    record.accountAddress = context.accountAddress;

    record.name = context.name.empty() ? call.remoteDisplayName : context.name;
    if (record.name == record.number) {
        record.name.clear();
    }

    if (call.direction == CallDirection::Outgoing) {
        record.dialedNumber = call.dialedNumber;
    }

    if (call.connectedAt.has_value()) {
        const auto elapsed = context.endedAt - *call.connectedAt;

        record.duration = std::max(std::chrono::seconds{0},
                                   std::chrono::duration_cast<std::chrono::seconds>(elapsed));
    }

    return record;
}

}
