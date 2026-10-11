// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/call.hpp>

#include <softphone/core/call_backend.hpp>

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

namespace softphone::core {
namespace {

[[nodiscard]] std::string_view withoutSpace(std::string_view text) noexcept
{
    while (!text.empty() && (std::isspace(static_cast<unsigned char>(text.front())) != 0)) {
        text.remove_prefix(1);
    }
    while (!text.empty() && (std::isspace(static_cast<unsigned char>(text.back())) != 0)) {
        text.remove_suffix(1);
    }
    return text;
}

[[nodiscard]] bool sameWord(std::string_view one, std::string_view other) noexcept
{
    return std::ranges::equal(one, other, [](char left, char right) {
        return std::tolower(static_cast<unsigned char>(left)) ==
               std::tolower(static_cast<unsigned char>(right));
    });
}

[[nodiscard]] std::optional<int> causeIn(std::string_view parameters) noexcept
{
    constexpr std::string_view kCause = "cause";

    while (!parameters.empty()) {
        const auto end = parameters.find(';');
        const auto one = withoutSpace(parameters.substr(0, end));
        parameters =
            end == std::string_view::npos ? std::string_view{} : parameters.substr(end + 1);

        const auto equals = one.find('=');
        if (equals == std::string_view::npos ||
            !sameWord(withoutSpace(one.substr(0, equals)), kCause)) {
            continue;
        }

        auto digits = withoutSpace(one.substr(equals + 1));
        if (digits.size() >= 2 && digits.front() == '"' && digits.back() == '"') {
            digits = digits.substr(1, digits.size() - 2);
        }
        if (digits.empty() || !std::ranges::all_of(digits, [](char at) {
                return std::isdigit(static_cast<unsigned char>(at)) != 0;
            })) {
            return std::nullopt;
        }

        int value = 0;
        for (const char at : digits) {

            if (value > 9999) {
                return std::nullopt;
            }
            value = value * 10 + (at - '0');
        }
        return value;
    }

    return std::nullopt;
}

}

std::string sipUserPart(std::string_view uri)
{
    auto text = uri;

    if (const auto scheme = text.find(':'); scheme != std::string_view::npos) {
        const auto prefix = text.substr(0, scheme);
        if (prefix == "sip" || prefix == "sips" || prefix == "tel") {
            text.remove_prefix(scheme + 1);
        }
    }

    if (const auto at = text.find('@'); at != std::string_view::npos) {
        text = text.substr(0, at);
    }

    if (const auto semicolon = text.find(';'); semicolon != std::string_view::npos) {
        text = text.substr(0, semicolon);
    }

    return std::string{text};
}

std::string dialable(std::string_view typed)
{

    if (const auto opened = typed.find('<'); opened != std::string_view::npos) {
        if (const auto closed = typed.find('>', opened); closed != std::string_view::npos) {
            typed = typed.substr(opened + 1, closed - opened - 1);
        }
    }

    while (!typed.empty() && (typed.front() == ' ' || typed.front() == '\t')) {
        typed.remove_prefix(1);
    }
    while (!typed.empty() && (typed.back() == ' ' || typed.back() == '\t')) {
        typed.remove_suffix(1);
    }

    const auto isLetter = [](char character) {
        return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
    };

    for (const std::string_view scheme : {"tel:", "callto:"}) {
        if (typed.size() > scheme.size() &&
            std::equal(scheme.begin(), scheme.end(), typed.begin(),
                       [](char a, char b) { return a == b || a == b + ('a' - 'A'); })) {
            typed.remove_prefix(scheme.size());
            break;
        }
    }

    const auto anAddress =
        typed.find('@') != std::string_view::npos ||
        std::any_of(typed.begin(), typed.end(), [&](char one) { return isLetter(one); });

    std::string dialled;
    dialled.reserve(typed.size());

    for (const char character : typed) {
        if (anAddress) {

            if (static_cast<unsigned char>(character) > ' ') {
                dialled.push_back(character);
            }
            continue;
        }

        if (character >= '0' && character <= '9') {
            dialled.push_back(character);
        } else if (character == '*' || character == '#') {
            dialled.push_back(character);
        } else if (character == '+' && dialled.empty()) {

            dialled.push_back(character);
        }
    }

    return dialled;
}

namespace {

[[nodiscard]] std::string_view trimmed(std::string_view text)
{
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) {
        text.remove_prefix(1);
    }
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) {
        text.remove_suffix(1);
    }
    return text;
}

}

SipAddress parseSipAddress(std::string_view address)
{
    auto text = trimmed(address);

    const auto open = text.find('<');
    if (open == std::string_view::npos) {

        return SipAddress{{}, std::string{text}};
    }

    auto name = trimmed(text.substr(0, open));
    if (name.size() >= 2 && name.front() == '"' && name.back() == '"') {
        name = name.substr(1, name.size() - 2);
    }

    auto uri = text.substr(open + 1);
    if (const auto close = uri.find('>'); close != std::string_view::npos) {
        uri = uri.substr(0, close);
    }

    return SipAddress{std::string{name}, std::string{uri}};
}

std::string dialTarget(std::string_view input, const AccountConfig &account)
{
    const auto target = trimmed(input);
    if (target.empty()) {
        return {};
    }

    const auto named = [&account](std::string uri) {
        return sayingTheTransport(std::move(uri), account.transport);
    };

    if (target.starts_with("sip:") || target.starts_with("sips:") || target.starts_with("tel:")) {
        return named(std::string{target});
    }

    if (target.find('@') != std::string_view::npos) {
        return named("sip:" + std::string{target});
    }

    auto uri = "sip:" + std::string{target} + '@' + account.domain;
    if (account.port != 0 && account.port != AccountConfig::defaultPortFor(account.transport)) {
        uri += ':' + std::to_string(account.port);
    }
    return named(std::move(uri));
}

std::string CallInfo::remoteLabel() const
{
    if (!remoteDisplayName.empty()) {
        return remoteDisplayName;
    }
    return sipUserPart(remoteUri);
}

std::string CallInfo::remoteDetail() const
{
    auto number = sipUserPart(remoteUri);
    if (number == remoteLabel()) {
        return {};
    }
    return number;
}

bool saysCompletedElsewhere(std::span<const std::string> reasons) noexcept
{

    for (const auto &whole : reasons) {
        std::string_view rest{whole};
        while (!rest.empty()) {
            const auto comma = rest.find(',');
            const auto one = withoutSpace(rest.substr(0, comma));
            rest = comma == std::string_view::npos ? std::string_view{} : rest.substr(comma + 1);

            const auto semicolon = one.find(';');
            if (semicolon == std::string_view::npos) {
                continue;
            }

            const auto protocol = withoutSpace(one.substr(0, semicolon));
            const auto cause = causeIn(one.substr(semicolon + 1));
            if (!cause.has_value()) {
                continue;
            }

            if ((sameWord(protocol, "sip") && *cause == 200) ||
                (sameWord(protocol, "q.850") && *cause == 26)) {
                return true;
            }
        }
    }
    return false;
}

CallEndReason classifyCallEnd(int sipStatus, bool endedLocally, bool completedElsewhere,
                              LocalFailure local) noexcept
{

    if (local == LocalFailure::NoAudioDevice) {
        return CallEndReason::NoAudioDevice;
    }

    if (endedLocally) {
        return CallEndReason::LocalHangup;
    }

    if (completedElsewhere) {
        return CallEndReason::PickedUpElsewhere;
    }

    switch (sipStatus) {
    case 486:
    case 600:
        return CallEndReason::Busy;

    case 603:
        return CallEndReason::Declined;

    case 408:
        return CallEndReason::NoAnswer;

    case 480:
        return CallEndReason::Unavailable;

    case 487:
        return CallEndReason::Cancelled;

    case 404:
    case 484:
        return CallEndReason::NumberNotFound;

    case 401:
    case 403:
    case 407:
        return CallEndReason::NotAllowed;

    case 488:
    case 606:
        return CallEndReason::MediaMismatch;

    default:
        break;
    }

    if (sipStatus >= 200 && sipStatus < 300) {
        return CallEndReason::RemoteHangup;
    }

    if (sipStatus >= 500 && sipStatus <= 599) {
        return CallEndReason::ServerError;
    }

    if (sipStatus == 0) {
        return CallEndReason::Unreachable;
    }

    return CallEndReason::Failed;
}

std::vector<CallInfo> transferCandidates(std::span<const CallInfo> calls, const std::string &callId)
{
    const CallInfo *self = nullptr;
    for (const auto &call : calls) {
        if (call.id == callId) {
            self = &call;
            break;
        }
    }

    if (self == nullptr || !self->canTransfer()) {
        return {};
    }

    std::vector<CallInfo> targets;
    for (const auto &call : calls) {
        if (call.id != callId && call.canTransfer() && call.accountId == self->accountId) {
            targets.push_back(call);
        }
    }

    return targets;
}

std::vector<CallInfo> conferenceCandidates(std::span<const CallInfo> calls)
{
    std::vector<CallInfo> joining;
    for (const auto &call : calls) {
        if (call.state == CallState::Active && !call.transfer.inFlight()) {
            joining.push_back(call);
        }
    }

    if (joining.size() < 2) {
        return {};
    }

    const bool anyParked =
        std::ranges::any_of(joining, [](const CallInfo &call) { return call.heldByUs; });
    if (!anyParked) {
        return {};
    }

    return joining;
}

std::string_view toString(CallState state) noexcept
{
    switch (state) {
    case CallState::Idle:
        return "idle";
    case CallState::Dialing:
        return "dialing";
    case CallState::RingingOut:
        return "ringing-out";
    case CallState::RingingIn:
        return "ringing-in";
    case CallState::Active:
        return "active";
    case CallState::Ended:
        return "ended";
    }
    return "unknown";
}

std::string_view toString(CallEndReason reason) noexcept
{
    switch (reason) {
    case CallEndReason::None:
        return "none";
    case CallEndReason::LocalHangup:
        return "local-hangup";
    case CallEndReason::RemoteHangup:
        return "remote-hangup";
    case CallEndReason::Transferred:
        return "transferred";
    case CallEndReason::Replaced:
        return "replaced";
    case CallEndReason::Busy:
        return "busy";
    case CallEndReason::Declined:
        return "declined";
    case CallEndReason::DoNotDisturb:
        return "do-not-disturb";
    case CallEndReason::TooManyCalls:
        return "too-many-calls";
    case CallEndReason::CallWaitingOff:
        return "call-waiting-off";
    case CallEndReason::PickedUpElsewhere:
        return "picked-up-elsewhere";
    case CallEndReason::NoAnswer:
        return "no-answer";
    case CallEndReason::Unavailable:
        return "unavailable";
    case CallEndReason::Cancelled:
        return "cancelled";
    case CallEndReason::NumberNotFound:
        return "number-not-found";
    case CallEndReason::NotAllowed:
        return "not-allowed";
    case CallEndReason::MediaMismatch:
        return "media-mismatch";
    case CallEndReason::ServerError:
        return "server-error";
    case CallEndReason::Unreachable:
        return "unreachable";
    case CallEndReason::NoAudioDevice:
        return "no-audio-device";
    case CallEndReason::Failed:
        return "failed";
    }
    return "unknown";
}

std::string_view toString(TransferState state) noexcept
{
    switch (state) {
    case TransferState::None:
        return "none";
    case TransferState::Requested:
        return "requested";
    case TransferState::Ringing:
        return "ringing";
    case TransferState::Succeeded:
        return "succeeded";
    case TransferState::Failed:
        return "failed";
    }
    return "unknown";
}

std::string_view toString(CallDirection direction) noexcept
{
    switch (direction) {
    case CallDirection::Outgoing:
        return "outgoing";
    case CallDirection::Incoming:
        return "incoming";
    }
    return "unknown";
}

bool isDtmfDigit(char digit) noexcept
{
    return (digit >= '0' && digit <= '9') || digit == '*' || digit == '#' ||
           (digit >= 'A' && digit <= 'D');
}

std::optional<CallDirection> parseCallDirection(std::string_view text) noexcept
{
    for (const auto direction : {CallDirection::Outgoing, CallDirection::Incoming}) {
        if (toString(direction) == text) {
            return direction;
        }
    }
    return std::nullopt;
}

std::optional<CallEndReason> parseCallEndReason(std::string_view text) noexcept
{

    for (const auto reason : {
             CallEndReason::None,
             CallEndReason::LocalHangup,
             CallEndReason::RemoteHangup,
             CallEndReason::Transferred,

             CallEndReason::Replaced,
             CallEndReason::Busy,
             CallEndReason::Declined,
             CallEndReason::DoNotDisturb,
             CallEndReason::TooManyCalls,
             CallEndReason::CallWaitingOff,
             CallEndReason::PickedUpElsewhere,
             CallEndReason::NoAnswer,
             CallEndReason::Cancelled,
             CallEndReason::Unavailable,
             CallEndReason::NumberNotFound,
             CallEndReason::NotAllowed,
             CallEndReason::MediaMismatch,
             CallEndReason::ServerError,
             CallEndReason::Unreachable,
             CallEndReason::NoAudioDevice,
             CallEndReason::Failed,
         }) {
        if (toString(reason) == text) {
            return reason;
        }
    }
    return std::nullopt;
}

bool streamRestarted(const CallMediaStats &previous, const CallMediaStats &current) noexcept
{

    return current.sent.packets < previous.sent.packets ||
           current.received.packets < previous.received.packets;
}

std::optional<CallInfo> callToTalk(std::span<const CallInfo> calls, std::string_view accountId)
{
    for (const auto &call : calls) {
        if (call.accountId == accountId && call.state == CallState::RingingIn) {
            return call;
        }
    }

    for (const auto &call : calls) {
        if (call.accountId == accountId && call.state == CallState::Active && call.heldByUs) {
            return call;
        }
    }

    return std::nullopt;
}

std::optional<CallInfo> callToHold(std::span<const CallInfo> calls, std::string_view accountId)
{
    for (const auto &call : calls) {
        if (call.accountId == accountId && call.holdsTheAudio()) {
            return call;
        }
    }
    return std::nullopt;
}

namespace {

[[nodiscard]] bool sameText(std::string_view left, std::string_view right)
{
    return std::ranges::equal(left, right, [](char one, char other) {
        return std::tolower(static_cast<unsigned char>(one)) ==
               std::tolower(static_cast<unsigned char>(other));
    });
}

[[nodiscard]] std::string hostPart(std::string_view uri)
{
    for (const auto *const scheme : {"sips:", "sip:"}) {
        if (uri.starts_with(scheme)) {
            uri.remove_prefix(std::string_view{scheme}.size());
            break;
        }
    }

    if (const auto at = uri.find('@'); at != std::string_view::npos) {
        uri.remove_prefix(at + 1);
    } else {

        if (uri.find('.') == std::string_view::npos && uri.find(':') == std::string_view::npos) {
            return {};
        }
    }

    if (const auto end = uri.find_first_of(";?"); end != std::string_view::npos) {
        uri = uri.substr(0, end);
    }
    if (uri.starts_with('[')) {
        if (const auto close = uri.find(']'); close != std::string_view::npos) {
            return std::string{uri.substr(1, close - 1)};
        }
        return std::string{uri};
    }
    if (const auto colon = uri.rfind(':'); colon != std::string_view::npos) {
        uri = uri.substr(0, colon);
    }
    return std::string{uri};
}

}

bool somebodyElseTransferred(std::string_view referredBy, const AccountConfig &account)
{
    if (referredBy.empty()) {
        return false;
    }

    const auto uri = parseSipAddress(referredBy).uri;
    if (!sameText(sipUserPart(uri), account.username)) {
        return true;
    }

    const auto host = hostPart(uri);
    if (host.empty()) {
        return false;
    }

    for (const auto &known : {account.domain, account.effectiveRegistrar(), switchHost(account)}) {
        if (!known.empty() && sameText(host, known)) {
            return false;
        }
    }
    return true;
}

bool talkingToOurselves(const CallInfo &call, std::string_view ourNumber) noexcept
{
    return !ourNumber.empty() && sameText(sipUserPart(call.remoteUri), ourNumber);
}

bool carriesTheTransferInto(const CallInfo &call, std::string_view newCallId) noexcept
{
    return call.fromThisLine && call.transfer.askedByThem && call.transfer.inFlight() &&
           call.transfer.otherCallId == newCallId;
}

bool continuedElsewhere(const CallInfo &call, std::chrono::steady_clock::time_point endedAt,
                        std::string_view ourNumber) noexcept
{
    if (call.end.reason == CallEndReason::Replaced) {
        return true;
    }

    if (call.end.reason != CallEndReason::Transferred || !call.transfer.askedByThem ||
        call.transfer.otherCallId.empty()) {
        return false;
    }

    if (!call.connectedAt.has_value()) {
        return true;
    }

    if (talkingToOurselves(call, ourNumber)) {
        return true;
    }

    return endedAt - *call.connectedAt < kSpokenAtAll;
}

bool onTheLine(const CallInfo &call) noexcept
{
    return call.state != CallState::Idle && call.state != CallState::Ended;
}

bool aCallIsUp(std::span<const CallInfo> calls) noexcept
{
    return std::any_of(calls.begin(), calls.end(),
                       [](const CallInfo &call) { return onTheLine(call); });
}

std::string_view standingOf(const CallInfo &call) noexcept
{
    if (call.state == CallState::Active) {
        if (call.inConference) {
            return "conference";
        }
        if (call.heldByUs) {
            return "hold";
        }
        if (call.heldByThem) {
            return "onhold";
        }
    }
    return toString(call.state);
}

bool sameStanding(const CallInfo &before, const CallInfo &after) noexcept
{
    return standingOf(before) == standingOf(after);
}

}
