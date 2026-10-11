// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/tones.hpp>

namespace softphone::core {

bool ringsHere(const CallInfo &call) noexcept
{
    return call.state == CallState::RingingIn &&
           (!call.answeredAutomatically() || call.autoAnswerAfter > std::chrono::seconds{0});
}

CallTone continuousTone(std::span<const CallInfo> calls) noexcept
{
    for (const auto &call : calls) {
        if (ringsHere(call)) {
            return CallTone::Ringtone;
        }
    }

    for (const auto &call : calls) {

        if (call.state == CallState::RingingOut && call.farEndRinging) {
            return CallTone::Ringback;
        }
    }

    return CallTone::None;
}

ToneLevel toneLevel(CallTone tone) noexcept
{

    return tone == CallTone::Ringtone ? ToneLevel::Ring : ToneLevel::Notifications;
}

ToneOutput toneOutput(CallTone tone) noexcept
{

    return tone == CallTone::Ringtone ? ToneOutput::Ringer : ToneOutput::Speakers;
}

bool testSoundAllowed(std::span<const CallInfo> calls) noexcept
{
    for (const auto &call : calls) {
        switch (call.state) {
        case CallState::Idle:
        case CallState::Ended:

            break;
        case CallState::Dialing:
        case CallState::RingingOut:
        case CallState::RingingIn:
        case CallState::Active:
            return false;
        }
    }

    return true;
}

bool answerIsAudible(const CallInfo &call) noexcept
{

    return call.state == CallState::Active && call.answeredAutomatically() && !call.fromThisLine;
}

bool failureIsAudible(const CallInfo &call) noexcept
{

    if (call.direction != CallDirection::Outgoing || call.state != CallState::Ended ||
        call.everConnected()) {
        return false;
    }

    switch (call.end.reason) {
    case CallEndReason::LocalHangup:
    case CallEndReason::Cancelled:

        return false;
    case CallEndReason::Transferred:
    case CallEndReason::Replaced:
    case CallEndReason::None:

        return false;
    case CallEndReason::DoNotDisturb:
    case CallEndReason::TooManyCalls:
    case CallEndReason::CallWaitingOff:
    case CallEndReason::PickedUpElsewhere:

        return false;
    case CallEndReason::Busy:
    case CallEndReason::Declined:
    case CallEndReason::NoAnswer:
    case CallEndReason::Unavailable:
    case CallEndReason::NumberNotFound:
    case CallEndReason::NotAllowed:
    case CallEndReason::MediaMismatch:
    case CallEndReason::ServerError:
    case CallEndReason::Unreachable:
    case CallEndReason::NoAudioDevice:
    case CallEndReason::RemoteHangup:
    case CallEndReason::Failed:

        return true;
    }

    return false;
}

bool endingIsAudible(const CallInfo &call) noexcept
{

    return call.state == CallState::Ended && call.everConnected() &&
           call.end.reason != CallEndReason::LocalHangup &&
           call.end.reason != CallEndReason::Transferred &&
           call.end.reason != CallEndReason::Replaced;
}

}
