// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/call_controller.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/uuid.hpp>

#include <algorithm>
#include <ranges>
#include <softphone/core/uuid.hpp>
#include <string>
#include <utility>
#include <vector>

namespace softphone::core {
namespace {

constexpr int kBusyHere = 486;

constexpr int kDecline = 603;

constexpr int kRinging = 180;

}

void CallController::note(LogLevel level, const std::string &callId, const std::string &line,
                          const std::string &what)
{

    std::vector<LogAttribute> fields;
    if (!callId.empty()) {
        fields.push_back(LogAttribute{"call", callId});
    }
    if (!line.empty()) {
        fields.push_back(LogAttribute{"line", line});
    }

    Logger::instance().log(level, "call", what, fields);
}

void CallController::note(LogLevel level, const CallInfo &call, const std::string &what)
{
    note(level, call.id, call.accountId, what);
}

CallController::CallController(ICallBackend &backend, IScheduler &scheduler, CallPolicy policy,
                               CallCallback onCallChanged,
                               std::function<std::chrono::system_clock::time_point()> wallClock)
    : backend_{backend}, scheduler_{scheduler}, policy_{policy}, wallClock_{std::move(wallClock)},
      onCallChanged_{std::move(onCallChanged)}
{
    if (!wallClock_) {
        wallClock_ = [] { return std::chrono::system_clock::now(); };
    }
}

CallController::~CallController()
{

    for (const auto &[callId, timer] : ringTimers_) {
        scheduler_.cancel(timer);
    }
    for (const auto &[callId, timer] : transferTimers_) {
        scheduler_.cancel(timer);
    }
}

void CallController::setPolicy(CallPolicy policy)
{
    policy_ = std::move(policy);
}

void CallController::setCallWaiting(CallWaiting waiting)
{
    policy_.callWaiting = waiting;
    tellTheBackendWhatASecondCallerMeets();
}

void CallController::tellTheBackendWhatASecondCallerMeets()
{
    const auto anyCall = std::ranges::any_of(calls_, [](const auto &entry) {
        return entry.second.state != CallState::Idle && entry.second.state != CallState::Ended;
    });
    backend_.refuseSecondCalls(policy_.callWaiting == CallWaiting::Busy && anyCall);
}

bool busyBecauseCallWaitingIsOff(CallWaiting waiting, const AutoAnswerRequest &asked,
                                 const OtherCalls &others) noexcept
{
    if (waiting != CallWaiting::Busy || !others.any) {
        return false;
    }

    return !asked.asked();
}

namespace {

[[nodiscard]] std::string seanceFrom(const IncomingCall &incoming)
{
    return isUuid(incoming.seanceId) ? incoming.seanceId : makeUuid();
}

}

std::string CallController::place(const std::string &accountId, const std::string &target,
                                  std::string dialed, int redialAttempt,
                                  std::string redialContinues, std::string seance)
{
    if (calls_.size() >= policy_.maxConcurrentCalls) {
        note(LogLevel::Warning, std::string{}, accountId,
             "not placing a call: " + std::to_string(calls_.size()) + " are already up");
        return {};
    }

    if (redialAttempt == 0) {
        const std::lock_guard guard{lastDialledLock_};
        lastDialled_ = dialed.empty() ? target : dialed;
    }

    auto callId = "call-" + std::to_string(++nextCallNumber_);

    holdOthers(callId);

    CallInfo call;
    call.id = callId;

    call.uuid = makeUuid();
    call.accountId = accountId;
    call.direction = CallDirection::Outgoing;
    call.remoteUri = target;
    call.dialedNumber = dialed.empty() ? target : std::move(dialed);
    call.state = CallState::Dialing;
    call.startedAt = scheduler_.now();
    call.redialAttempt = redialAttempt;
    call.redialContinues = std::move(redialContinues);
    call.seanceId = seance.empty() ? makeUuid() : std::move(seance);

    calls_[callId] = call;

    note(LogLevel::Info, call, "calling " + call.remoteUri);

    scheduleRingTimeout(callId);

    notify(call);

    backend_.noteSeance(callId, call.seanceId);
    backend_.placeCall(callId, accountId, target);
    return callId;
}

void CallController::refused(const IncomingCall &incoming, CallEndReason why)
{

    CallInfo call;
    call.id = incoming.callId;
    call.uuid = makeUuid();
    call.accountId = incoming.accountId;
    call.direction = CallDirection::Incoming;
    call.remoteUri = incoming.remoteUri;
    call.remoteDisplayName = incoming.remoteDisplayName;
    call.startedAt = scheduler_.now();
    call.state = CallState::Ended;
    call.end = CallEndInfo{why, kBusyHere, {}};
    call.seanceId = seanceFrom(incoming);

    notify(call);
}

void CallController::onIncomingCall(const IncomingCall &incoming)
{

    if (incoming.refusedBeforeItRang != CallEndReason::None) {
        note(LogLevel::Info, incoming.callId, incoming.accountId,
             std::string{"refused before it rang: "} +
                 (incoming.refusedBeforeItRang == CallEndReason::DoNotDisturb
                      ? "do not disturb is on"
                      : "call waiting is off and this phone is on a call"));
        refused(incoming, incoming.refusedBeforeItRang);
        return;
    }

    if (doNotDisturb()) {
        note(LogLevel::Info, incoming.callId, incoming.accountId,
             "busy signal to an incoming call: do not disturb is on");
        backend_.rejectCall(incoming.callId, kBusyHere);
        refused(incoming, CallEndReason::DoNotDisturb);
        return;
    }

    if (busyBecauseCallWaitingIsOff(policy_.callWaiting, incoming.autoAnswer,
                                    otherCalls(incoming.callId, incoming.accountId))) {

        note(LogLevel::Info, incoming.callId, incoming.accountId,
             "busy signal to an incoming call: call waiting is off and this phone is on a call");
        backend_.rejectCall(incoming.callId, kBusyHere);
        refused(incoming, CallEndReason::CallWaitingOff);
        return;
    }

    if (calls_.size() >= policy_.maxConcurrentCalls) {

        note(LogLevel::Warning, incoming.callId, incoming.accountId,
             "busy signal to an incoming call: " + std::to_string(calls_.size()) +
                 " are already up");
        backend_.rejectCall(incoming.callId, kBusyHere);
        refused(incoming, CallEndReason::TooManyCalls);
        return;
    }

    CallInfo call;
    call.id = incoming.callId;
    call.uuid = makeUuid();
    call.accountId = incoming.accountId;
    call.direction = CallDirection::Incoming;
    call.remoteUri = incoming.remoteUri;
    call.remoteDisplayName = incoming.remoteDisplayName;
    call.state = CallState::RingingIn;
    call.startedAt = scheduler_.now();
    call.seanceId = seanceFrom(incoming);

    call.fromThisLine = incoming.fromThisLine;
    backend_.noteSeance(incoming.callId, call.seanceId);

    if (!incoming.referredBy.empty()) {
        call.transfer.referredBy = incoming.referredBy;
        call.transfer.askedByThem = true;
    }

    const auto decision =
        decideAutoAnswer(incoming.autoAnswer, accountPolicy(incoming.accountId).autoAnswer,
                         otherCalls(incoming.callId, incoming.accountId));
    call.autoAnswer = decision.why;
    call.autoAnswerAfter = decision.after;

    calls_[incoming.callId] = call;

    note(LogLevel::Info, call, "ringing, from " + call.remoteUri);

    notify(call);

    if (decision.answering()) {
        note(LogLevel::Info, call,
             "answering by itself after " + std::to_string(decision.after.count()) + "s (" +
                 std::string{toString(decision.why)} + ')');
        scheduleAutoAnswer(call.id, decision.after);
    }
}

void CallController::answer(const std::string &callId, AnswerMode mode)
{
    const auto it = calls_.find(callId);
    if (it == calls_.end() || it->second.state != CallState::RingingIn) {
        return;
    }

    if (autoAnswerTimers_.contains(callId)) {
        cancelAutoAnswer(callId);
        it->second.autoAnswer = AutoAnswer::No;
    }

    if (mode == AnswerMode::EndOthers) {
        std::vector<std::string> doomed;
        for (const auto &[otherId, other] : calls_) {
            if (otherId != callId) {
                doomed.push_back(otherId);
            }
        }
        for (const auto &otherId : doomed) {
            hangUp(otherId);
        }
    } else {
        holdOthers(callId);
    }

    backend_.answerCall(callId);
}

void CallController::hold(const std::string &callId)
{
    const auto it = calls_.find(callId);
    if (it == calls_.end() || it->second.state != CallState::Active || it->second.heldByUs) {
        return;
    }

    it->second.heldByUs = true;
    note(LogLevel::Info, it->second, "held");
    backend_.holdCall(callId);

    routeAudio();
    refreshConference();

    notify(it->second);
}

void CallController::activate(const std::string &callId)
{
    const auto it = calls_.find(callId);
    if (it == calls_.end() || it->second.state != CallState::Active) {
        return;
    }

    holdOthers(callId);

    if (!it->second.heldByUs) {
        return;
    }

    it->second.heldByUs = false;
    note(LogLevel::Info, it->second, "taken off hold");
    backend_.resumeCall(callId);

    routeAudio();
    refreshConference();

    notify(it->second);
}

void CallController::audioDeviceChanged()
{
    backend_.reconnectAudio();
}

void CallController::routeAudio()
{

    std::vector<std::string> group;
    for (const auto &[callId, call] : calls_) {
        if (call.holdsTheAudio()) {
            group.push_back(callId);
        }
    }

    for (const auto &[callId, call] : calls_) {
        if (!call.earlyMedia || call.direction != CallDirection::Outgoing) {
            continue;
        }

        const bool nothingToTakeItFrom =
            group.empty() || std::ranges::all_of(group, [this, &callId](const std::string &held) {
                const auto it = calls_.find(held);
                return it != calls_.end() && carriesTheTransferInto(it->second, callId);
            });

        if (nothingToTakeItFrom) {
            group.assign(1, callId);
            break;
        }
    }

    backend_.routeAudioTo(group);
}

void CallController::refreshConference()
{
    std::size_t sharing = 0;
    for (const auto &[callId, call] : calls_) {
        if (call.holdsTheAudio()) {
            ++sharing;
        }
    }

    std::vector<std::string> changed;
    for (auto &[callId, call] : calls_) {
        const bool joined = sharing > 1 && call.holdsTheAudio();
        if (call.inConference != joined) {
            call.inConference = joined;
            changed.push_back(callId);
        }
    }

    for (const auto &callId : changed) {
        if (const auto it = calls_.find(callId); it != calls_.end()) {
            notify(it->second);
        }
    }
}

void CallController::merge(const std::vector<std::string> &callIds)
{

    std::vector<std::string> joining;
    for (const auto &callId : callIds) {
        const auto it = calls_.find(callId);

        if (it != calls_.end() && it->second.heldByUs) {
            joining.push_back(callId);
        }
    }

    for (const auto &callId : joining) {
        const auto it = calls_.find(callId);
        if (it == calls_.end()) {
            continue;
        }

        it->second.heldByUs = false;
        backend_.resumeCall(callId);
    }

    routeAudio();
    refreshConference();

    std::string room;
    for (const auto &[callId, call] : calls_) {
        if (call.inConference && std::ranges::find(joining, callId) == joining.end() &&
            !call.seanceId.empty()) {
            room = call.seanceId;
            break;
        }
    }
    for (const auto &callId : joining) {
        if (const auto it = calls_.find(callId); room.empty() && it != calls_.end()) {
            room = it->second.seanceId;
        }
    }
    std::vector<std::string> renamed;
    for (auto &[callId, call] : calls_) {
        if (call.inConference && !room.empty() && call.seanceId != room) {
            call.seanceId = room;
            backend_.noteSeance(callId, room);
            if (std::ranges::find(joining, callId) == joining.end()) {
                renamed.push_back(callId);
            }
        }
    }
    for (const auto &callId : renamed) {
        if (const auto it = calls_.find(callId); it != calls_.end()) {
            notify(it->second);
        }
    }

    const bool anyMuted = std::ranges::any_of(
        calls_, [](const auto &entry) { return entry.second.inConference && entry.second.muted; });
    if (anyMuted) {
        muteConference(true);
    }

    for (const auto &callId : joining) {
        if (const auto it = calls_.find(callId); it != calls_.end()) {
            notify(it->second);
        }
    }
}

void CallController::split(const std::string &keep)
{
    holdOthers(keep);
    routeAudio();
    refreshConference();
}

void CallController::endConference()
{

    std::vector<std::string> members;
    for (const auto &[callId, call] : calls_) {
        if (call.inConference) {
            members.push_back(callId);
        }
    }

    for (const auto &callId : members) {
        hangUp(callId);
    }
}

void CallController::holdOthers(const std::string &keep)
{

    std::vector<std::string> targets;
    for (const auto &[otherId, other] : calls_) {
        if (otherId != keep && other.state == CallState::Active && !other.heldByUs) {
            targets.push_back(otherId);
        }
    }

    for (const auto &otherId : targets) {
        const auto it = calls_.find(otherId);
        if (it == calls_.end()) {
            continue;
        }

        it->second.heldByUs = true;
        note(LogLevel::Info, it->second, "held, to make room for another call");
        backend_.holdCall(otherId);

        notify(it->second);
    }
}

void CallController::reject(const std::string &callId)
{
    const auto it = calls_.find(callId);
    if (it == calls_.end() || it->second.state != CallState::RingingIn) {
        return;
    }

    note(LogLevel::Info, it->second, "declined here");
    backend_.rejectCall(callId, kDecline);
    finish(callId, CallEndInfo{CallEndReason::LocalHangup, kDecline, {}});
}

void CallController::hangUp(const std::string &callId)
{
    const auto it = calls_.find(callId);
    if (it == calls_.end()) {
        return;
    }

    note(LogLevel::Info, it->second, "hung up here");
    backend_.hangUpCall(callId);
    finish(callId, CallEndInfo{CallEndReason::LocalHangup, 0, {}});
}

void CallController::setMuted(const std::string &callId, bool muted)
{
    const auto it = calls_.find(callId);
    if (it == calls_.end()) {
        return;
    }

    if (it->second.inConference) {
        muteConference(muted);
        return;
    }

    if (it->second.muted == muted) {
        return;
    }

    it->second.muted = muted;
    note(LogLevel::Info, it->second, muted ? "microphone off" : "microphone on");
    backend_.setCallMuted(callId, muted);

    notify(it->second);
}

void CallController::muteConference(bool muted)
{
    std::vector<std::string> changed;
    for (auto &[callId, call] : calls_) {
        if (!call.inConference || call.muted == muted) {
            continue;
        }

        call.muted = muted;
        note(LogLevel::Info, call,
             muted ? "microphone off, with the conference" : "microphone on, with the conference");
        backend_.setCallMuted(callId, muted);
        changed.push_back(callId);
    }

    for (const auto &callId : changed) {
        if (const auto it = calls_.find(callId); it != calls_.end()) {
            notify(it->second);
        }
    }
}

void CallController::sendDtmf(const std::string &callId, char digit)
{
    if (!isDtmfDigit(digit)) {
        return;
    }

    const auto it = calls_.find(callId);
    if (it == calls_.end() || !it->second.canSendDtmf()) {
        return;
    }

    note(LogLevel::Info, it->second, "a tone sent down the line");
    backend_.sendDtmf(callId, digit, accountPolicy(it->second.accountId).dtmfTransport);
}

OtherCalls CallController::otherCalls(const std::string &callId, const std::string &line) const
{
    OtherCalls others;
    for (const auto &[otherId, other] : calls_) {
        if (otherId == callId || other.state == CallState::Idle ||
            other.state == CallState::Ended) {
            continue;
        }
        others.any = true;
        others.onAnotherLine = others.onAnotherLine || other.accountId != line;
    }
    return others;
}

AccountCallPolicy callPolicyFor(const AccountConfig &account)
{
    return AccountCallPolicy{account.dtmfTransport, autoAnswerPolicy(account)};
}

void CallController::setAccountPolicy(const std::string &accountId, AccountCallPolicy policy)
{
    accountPolicies_[accountId] = policy;
}

void CallController::forgetAccountPolicy(const std::string &accountId)
{
    accountPolicies_.erase(accountId);
}

AccountCallPolicy CallController::accountPolicy(const std::string &accountId) const
{
    const auto it = accountPolicies_.find(accountId);
    if (it == accountPolicies_.end()) {

        return AccountCallPolicy{policy_.dtmfTransport, AutoAnswerPolicy{}};
    }
    return it->second;
}

void CallController::beginTransfer(const std::string &callId)
{
    const auto it = calls_.find(callId);
    if (it == calls_.end() || !it->second.canTransfer()) {
        return;
    }

    if (it->second.heldByUs) {
        return;
    }

    parkedForTransfer_.insert(callId);
    hold(callId);
}

void CallController::cancelTransfer(const std::string &callId)
{
    if (parkedForTransfer_.erase(callId) == 0) {
        return;
    }

    activate(callId);
}

void CallController::transferBlind(const std::string &callId, const std::string &target)
{
    if (target.empty()) {
        return;
    }

    {
        const auto it = calls_.find(callId);
        if (it == calls_.end() || !it->second.canTransfer()) {
            return;
        }
    }

    const auto it = calls_.find(callId);
    if (it == calls_.end()) {
        return;
    }

    parkedForTransfer_.erase(callId);

    TransferInfo asked;
    asked.state = TransferState::Requested;
    asked.target = target;

    it->second.transfer = asked;
    backend_.transferCall(callId, target);
    scheduleTransferTimeout(callId);

    notify(it->second);
}

void CallController::transferAttended(const std::string &callId, const std::string &otherCallId)
{
    if (callId == otherCallId) {
        return;
    }

    const auto first = calls_.find(callId);
    const auto second = calls_.find(otherCallId);
    if (first == calls_.end() || second == calls_.end()) {
        return;
    }
    if (!first->second.canTransfer() || !second->second.canTransfer()) {
        return;
    }

    if (first->second.accountId != second->second.accountId) {
        return;
    }

    const std::string handedOver =
        !first->second.heldByUs && second->second.heldByUs ? otherCallId : callId;
    const std::string consultation = handedOver == callId ? otherCallId : callId;

    const auto handed = calls_.find(handedOver);
    const auto consult = calls_.find(consultation);
    if (handed == calls_.end() || consult == calls_.end()) {
        return;
    }

    TransferInfo onTheWayOut;
    onTheWayOut.state = TransferState::Requested;
    onTheWayOut.target = consult->second.remoteUri;
    onTheWayOut.otherCallId = consultation;

    TransferInfo consulting;
    consulting.state = TransferState::Requested;
    consulting.target = handed->second.remoteUri;
    consulting.otherCallId = handedOver;

    handed->second.transfer = onTheWayOut;
    consult->second.transfer = consulting;

    parkedForTransfer_.erase(handedOver);
    parkedForTransfer_.erase(consultation);

    backend_.transferCallToCall(handedOver, consultation);
    scheduleTransferTimeout(handedOver);

    notify(handed->second);
    notify(consult->second);
}

std::vector<CallInfo> CallController::transferCandidates(const std::string &callId) const
{

    const auto all = calls();
    return core::transferCandidates(all, callId);
}

std::vector<CallInfo> CallController::conferenceCandidates() const
{

    const auto all = calls();
    return core::conferenceCandidates(all);
}

void CallController::onTransferProgress(const TransferProgress &progress)
{
    const auto it = calls_.find(progress.callId);
    if (it == calls_.end() || !it->second.transfer.inFlight()) {

        return;
    }

    const bool answered = progress.sipStatus >= 200 && progress.sipStatus < 300;

    const bool refused = progress.sipStatus >= 300;

    if (!answered) {
        if (!progress.finalNotify && !refused) {

            if (progress.sipStatus == 180 || progress.sipStatus == 183) {
                it->second.transfer.state = TransferState::Ringing;
                notify(it->second);
            }
            return;
        }

        failTransfer(progress.callId, progress.sipStatus);
        return;
    }

    cancelTransferTimeout(progress.callId);
    it->second.transfer.state = TransferState::Succeeded;

    note(LogLevel::Info, it->second,
         "transfer succeeded (" + std::to_string(progress.sipStatus) + "), ending it");

    const std::string consultation = it->second.transfer.otherCallId;

    if (const auto consult = calls_.find(consultation); consult != calls_.end()) {
        consult->second.transfer.state = TransferState::Succeeded;
    }

    finishTransferred(progress.callId);
    if (!consultation.empty()) {
        finishTransferred(consultation);
    }
}

void CallController::onTransferRequested(const std::string &callId, const std::string &newCallId,
                                         const std::string &target, const std::string &referredBy)
{
    const auto it = calls_.find(callId);
    if (it == calls_.end() || calls_.contains(newCallId)) {
        return;
    }

    CallInfo moved;
    moved.id = newCallId;
    moved.accountId = it->second.accountId;
    moved.direction = CallDirection::Outgoing;
    moved.remoteUri = target;

    moved.dialedNumber = target;
    moved.state = CallState::Dialing;

    moved.seanceId = it->second.seanceId;
    backend_.noteSeance(newCallId, moved.seanceId);
    TransferInfo beingMoved;
    beingMoved.state = TransferState::Requested;
    beingMoved.target = target;
    beingMoved.referredBy = referredBy;
    beingMoved.askedByThem = true;

    TransferInfo onTheOldCall = beingMoved;
    onTheOldCall.otherCallId = newCallId;

    onTheOldCall.referredBy.clear();

    beingMoved.otherCallId = callId;
    beingMoved.continuation = true;
    moved.transfer = beingMoved;

    it->second.transfer = onTheOldCall;

    calls_[newCallId] = moved;

    scheduleRingTimeout(newCallId);

    notify(it->second);
    notify(moved);
}

void CallController::onCallReplaced(const std::string &oldCallId, const std::string &newCallId)
{
    const auto replacement = calls_.find(newCallId);
    if (replacement == calls_.end()) {
        return;
    }

    replacement->second.transfer.state = TransferState::Succeeded;
    replacement->second.transfer.otherCallId = oldCallId;
    replacement->second.transfer.askedByThem = true;
    replacement->second.transfer.continuation = true;

    const auto replaced = calls_.find(oldCallId);

    if (replaced != calls_.end() && !replaced->second.seanceId.empty()) {
        replacement->second.seanceId = replaced->second.seanceId;
        backend_.noteSeance(newCallId, replaced->second.seanceId);
    }

    const bool keepParked = replaced != calls_.end() && !replaced->second.holdsTheAudio();

    const auto continuedFrom = replaced != calls_.end()
                                   ? replaced->second.connectedAt
                                   : std::optional<std::chrono::steady_clock::time_point>{};

    if (replaced != calls_.end()) {
        finish(oldCallId, CallEndInfo{CallEndReason::Replaced, 0, {}});
    }

    const auto adopted = calls_.find(newCallId);
    if (adopted == calls_.end()) {
        return;
    }

    if (adopted->second.state == CallState::RingingIn) {
        adopted->second.heldByUs = keepParked;

        adopted->second.state = CallState::Active;
        if (!adopted->second.everConnected()) {
            adopted->second.connectedAt = continuedFrom.value_or(scheduler_.now());
        }

        routeAudio();
        refreshConference();
    }

    if (const auto self = calls_.find(newCallId); self != calls_.end()) {
        notify(self->second);
    }
}

void CallController::finishTransferred(const std::string &callId)
{
    if (!calls_.contains(callId)) {
        return;
    }

    cancelTransferTimeout(callId);
    backend_.hangUpCall(callId);
    finish(callId, CallEndInfo{CallEndReason::Transferred, 0, {}});
}

void CallController::failTransfer(const std::string &callId, int sipStatus)
{
    cancelTransferTimeout(callId);

    const auto it = calls_.find(callId);

    note(LogLevel::Info, callId, it == calls_.end() ? std::string{} : it->second.accountId,
         "transfer failed (" +
             (sipStatus == 0 ? std::string{"nothing came back"} : std::to_string(sipStatus)) +
             "); giving the call back");

    if (it == calls_.end()) {
        return;
    }

    const std::string consultation = it->second.transfer.otherCallId;

    it->second.transfer.state = TransferState::Failed;
    it->second.transfer.sipStatus = sipStatus;

    if (const auto consult = calls_.find(consultation); consult != calls_.end()) {
        consult->second.transfer.state = TransferState::Failed;
        consult->second.transfer.sipStatus = sipStatus;
        notify(consult->second);
    }

    if (const auto self = calls_.find(callId); self != calls_.end()) {
        notify(self->second);
    }
}

void CallController::scheduleTransferTimeout(const std::string &callId)
{
    if (policy_.transferTimeout <= std::chrono::milliseconds::zero()) {
        return;
    }

    transferTimers_[callId] = scheduler_.scheduleAfter(policy_.transferTimeout, [this, callId]() {
        transferTimers_.erase(callId);

        const auto it = calls_.find(callId);
        if (it == calls_.end() || !it->second.transfer.inFlight()) {
            return;
        }

        note(LogLevel::Warning, it->second,
             "nothing came back about the transfer within " +
                 std::to_string(policy_.transferTimeout.count()) + "ms");

        failTransfer(callId, 0);
    });
}

void CallController::cancelTransferTimeout(const std::string &callId)
{
    const auto it = transferTimers_.find(callId);
    if (it == transferTimers_.end()) {
        return;
    }

    scheduler_.cancel(it->second);
    transferTimers_.erase(it);
}

void CallController::onCallProgress(const CallProgress &progress)
{
    const auto it = calls_.find(progress.callId);
    if (it == calls_.end()) {
        return;
    }

    switch (progress.state) {
    case BackendCallState::Calling:
    case BackendCallState::Ringing:

        if (it->second.direction == CallDirection::Incoming) {
            return;
        }

        it->second.state = progress.state == BackendCallState::Calling ? CallState::Dialing
                                                                       : CallState::RingingOut;

        if (progress.sipStatus == kRinging) {
            it->second.farEndRinging = true;
        }

        note(LogLevel::Info, it->second,
             progress.state == BackendCallState::Calling ? "dialling" : "ringing at the far end");
        notify(it->second);
        return;

    case BackendCallState::Connected: {

        cancelRingTimeout(progress.callId);

        if (it->second.state != CallState::Active) {
            note(LogLevel::Info, it->second, "connected");
        }

        it->second.state = CallState::Active;

        if (!it->second.everConnected()) {
            it->second.connectedAt = scheduler_.now();
        }

        if (it->second.heldByUs) {

            backend_.holdCall(progress.callId);
        } else {

            holdOthers(progress.callId);

            routeAudio();
            refreshConference();
        }

        if (const auto self = calls_.find(progress.callId); self != calls_.end()) {
            notify(self->second);
        }

        if (const auto self = calls_.find(progress.callId);
            self != calls_.end() && self->second.transfer.continuation &&
            calls_.contains(self->second.transfer.otherCallId)) {
            const std::string origin = self->second.transfer.otherCallId;
            self->second.transfer.state = TransferState::Succeeded;
            cancelTransferTimeout(origin);
            finishTransferred(origin);
            if (const auto moved = calls_.find(progress.callId); moved != calls_.end()) {
                notify(moved->second);
            }
        }
        return;
    }

    case BackendCallState::Disconnected:
        break;
    }

    const TransferInfo transfer = it->second.transfer;
    const bool endedByTheTransfer =
        !progress.endedLocally && !transfer.continuation &&
        (transfer.state == TransferState::Succeeded || transfer.inFlight());

    const std::string origin =
        transfer.continuation && transfer.inFlight() ? transfer.otherCallId : std::string{};

    finish(progress.callId,
           CallEndInfo{endedByTheTransfer
                           ? CallEndReason::Transferred
                           : classifyCallEnd(progress.sipStatus, progress.endedLocally,
                                             progress.completedElsewhere, progress.localFailure),
                       progress.sipStatus, progress.reason});

    if (!origin.empty()) {
        failTransfer(origin, progress.sipStatus);
    }
}

void CallController::finish(const std::string &callId, CallEndInfo end)
{
    const auto it = calls_.find(callId);
    if (it == calls_.end()) {
        return;
    }

    cancelRingTimeout(callId);
    cancelAutoAnswer(callId);
    cancelTransferTimeout(callId);
    parkedForTransfer_.erase(callId);

    auto call = it->second;
    call.state = CallState::Ended;
    call.end = std::move(end);

    const auto talked =
        call.everConnected()
            ? std::chrono::duration_cast<std::chrono::seconds>(scheduler_.now() - *call.connectedAt)
            : std::chrono::seconds{0};
    note(LogLevel::Info, call,
         "ended: " + std::string{toString(call.end.reason)} +
             (call.end.sipStatus == 0 ? std::string{}
                                      : " (" + std::to_string(call.end.sipStatus) + ')') +
             ", after " + std::to_string(talked.count()) + "s");

    calls_.erase(it);

    routeAudio();
    refreshConference();

    notify(call);
}

void CallController::scheduleRingTimeout(const std::string &callId)
{
    if (policy_.ringTimeout <= std::chrono::milliseconds::zero()) {
        return;
    }

    ringTimers_[callId] = scheduler_.scheduleAfter(policy_.ringTimeout, [this, callId]() {
        ringTimers_.erase(callId);

        const auto it = calls_.find(callId);
        if (it == calls_.end() || it->second.state == CallState::Active) {
            return;
        }

        backend_.hangUpCall(callId);
        finish(callId, CallEndInfo{CallEndReason::NoAnswer, 0, {}});
    });
}

void CallController::onSwitchRequest(const std::string &accountId, SwitchRequest what,
                                     const std::string &callId)
{

    const auto named = calls_.find(callId);
    if (!callId.empty() && named != calls_.end() && named->second.accountId == accountId) {
        actOnSwitchRequest(named->second, what);
        return;
    }

    const auto lines = calls();

    if (what == SwitchRequest::Hold) {
        if (const auto target = callToHold(lines, accountId)) {
            actOnSwitchRequest(*target, what);
        }
        return;
    }

    if (const auto target = callToTalk(lines, accountId)) {
        actOnSwitchRequest(*target, what);
    }
}

void CallController::actOnSwitchRequest(const CallInfo &target, SwitchRequest what)
{
    const auto it = calls_.find(target.id);
    if (it == calls_.end()) {
        return;
    }

    if (what == SwitchRequest::Hold) {
        if (it->second.holdsTheAudio()) {
            note(LogLevel::Info, target, "the switch asked for hold");
            hold(target.id);
        }
        return;
    }

    if (it->second.state == CallState::RingingIn) {
        note(LogLevel::Info, target, "the switch asked us to answer");

        it->second.autoAnswer = AutoAnswer::SwitchRequest;
        cancelAutoAnswer(target.id);
        answerAutomatically(target.id);
        return;
    }

    if (it->second.state == CallState::Active && it->second.heldByUs) {
        note(LogLevel::Info, target, "the switch asked us to resume");
        activate(target.id);
    }
}

void CallController::scheduleAutoAnswer(const std::string &callId, std::chrono::seconds after)
{
    autoAnswerTimers_[callId] = scheduler_.scheduleAfter(after, [this, callId]() {
        autoAnswerTimers_.erase(callId);

        const auto it = calls_.find(callId);
        if (it == calls_.end() || it->second.state != CallState::RingingIn) {
            return;
        }

        if (!mayInterrupt(it->second.autoAnswer, otherCalls(callId, it->second.accountId))) {
            note(LogLevel::Info, it->second, "not answering by itself — something else is up");
            it->second.autoAnswer = AutoAnswer::No;
            notify(it->second);
            return;
        }

        answerAutomatically(callId);
    });
}

void CallController::answerAutomatically(const std::string &callId)
{
    const auto it = calls_.find(callId);
    if (it == calls_.end() || it->second.state != CallState::RingingIn) {
        return;
    }

    holdOthers(callId);
    backend_.answerCall(callId);
}

void CallController::cancelAutoAnswer(const std::string &callId)
{
    const auto it = autoAnswerTimers_.find(callId);
    if (it == autoAnswerTimers_.end()) {
        return;
    }

    scheduler_.cancel(it->second);
    autoAnswerTimers_.erase(it);
}

void CallController::cancelRingTimeout(const std::string &callId)
{
    const auto it = ringTimers_.find(callId);
    if (it == ringTimers_.end()) {
        return;
    }

    scheduler_.cancel(it->second);
    ringTimers_.erase(it);
}

void CallController::onRemoteHoldChanged(const std::string &callId, bool held)
{
    const auto it = calls_.find(callId);
    if (it == calls_.end() || it->second.heldByThem == held) {
        return;
    }

    it->second.heldByThem = held;
    note(LogLevel::Info, it->second,
         held ? "the far end put us on hold" : "the far end took us off hold");

    notify(it->second);
}

void CallController::onEarlyMediaChanged(const std::string &callId, bool arriving)
{
    const auto it = calls_.find(callId);
    if (it == calls_.end() || it->second.earlyMedia == arriving) {
        return;
    }

    it->second.earlyMedia = arriving;
    note(LogLevel::Info, it->second,
         arriving ? "early media: the far end is playing us something" : "early media has stopped");

    if (arriving && it->second.direction == CallDirection::Outgoing) {
        routeAudio();
    }

    notify(it->second);
}

void CallController::onRemoteIdentityChanged(const std::string &callId, const SipAddress &identity)
{
    const auto it = calls_.find(callId);
    if (it == calls_.end() || identity.uri.empty()) {
        return;
    }

    if (it->second.state == CallState::Ended) {
        return;
    }

    if (it->second.remoteUri == identity.uri &&
        it->second.remoteDisplayName == identity.displayName) {
        return;
    }

    it->second.remoteUri = identity.uri;
    it->second.remoteDisplayName = identity.displayName;

    notify(it->second);
}

std::optional<CallInfo> CallController::active() const
{
    for (const auto &[callId, call] : calls_) {
        if (call.holdsTheAudio()) {
            return call;
        }
    }
    return std::nullopt;
}

std::optional<CallInfo> CallController::current() const
{
    if (calls_.empty()) {
        return std::nullopt;
    }

    if (auto call = active(); call.has_value()) {
        return call;
    }

    for (const auto &[callId, call] : calls_) {
        if (call.state != CallState::Active) {
            return call;
        }
    }

    return calls_.begin()->second;
}

std::vector<CallInfo> CallController::calls() const
{
    std::vector<CallInfo> result;
    result.reserve(calls_.size());
    for (const auto &[callId, call] : calls_) {
        result.push_back(call);
    }
    return result;
}

void CallController::notify(const CallInfo &call)
{

    tellTheBackendWhatASecondCallerMeets();

    const auto previous = lastNotified_.find(call.id);
    const bool known = previous != lastNotified_.end();

    auto stamped = call;
    const auto now = wallClock_();
    stamped.wallStartedAt = known ? previous->second.wallStartedAt : now;
    stamped.wallStateSince =
        known && sameStanding(previous->second, call) ? previous->second.wallStateSince : now;
    if (const auto live = calls_.find(call.id); live != calls_.end()) {
        live->second.wallStartedAt = stamped.wallStartedAt;
        live->second.wallStateSince = stamped.wallStateSince;
    }

    if (known && previous->second == stamped) {
        return;
    }

    if (stamped.state == CallState::Ended) {
        lastNotified_.erase(call.id);
    } else {
        lastNotified_[call.id] = stamped;
    }

    if (onCallChanged_) {
        onCallChanged_(stamped);
    }
}

}
