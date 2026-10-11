// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "call_view_model.hpp"

#include <softphone/core/logging.hpp>

#include <algorithm>
#include <chrono>
#include <utility>

namespace softphone::ui {
namespace {

[[nodiscard]] CallViewModel::State toViewState(core::CallState state) noexcept
{
    switch (state) {
    case core::CallState::Idle:
        return CallViewModel::State::Idle;
    case core::CallState::Dialing:
        return CallViewModel::State::Dialing;
    case core::CallState::RingingOut:
        return CallViewModel::State::RingingOut;
    case core::CallState::RingingIn:
        return CallViewModel::State::RingingIn;
    case core::CallState::Active:
        return CallViewModel::State::Active;
    case core::CallState::Ended:
        return CallViewModel::State::Ended;
    }
    return CallViewModel::State::Idle;
}

[[nodiscard]] CallViewModel::Ending toViewEnding(core::CallEndReason reason) noexcept
{
    switch (reason) {
    case core::CallEndReason::None:
        return CallViewModel::Ending::None;
    case core::CallEndReason::LocalHangup:
        return CallViewModel::Ending::LocalHangup;
    case core::CallEndReason::RemoteHangup:
        return CallViewModel::Ending::RemoteHangup;
    case core::CallEndReason::Transferred:
    case core::CallEndReason::Replaced:

        return CallViewModel::Ending::Transferred;
    case core::CallEndReason::Busy:
        return CallViewModel::Ending::Busy;
    case core::CallEndReason::Declined:
    case core::CallEndReason::DoNotDisturb:
    case core::CallEndReason::TooManyCalls:
    case core::CallEndReason::CallWaitingOff:

        return CallViewModel::Ending::Declined;
    case core::CallEndReason::PickedUpElsewhere:
        return CallViewModel::Ending::PickedUpElsewhere;
    case core::CallEndReason::NoAnswer:
        return CallViewModel::Ending::NoAnswer;
    case core::CallEndReason::Cancelled:
        return CallViewModel::Ending::Cancelled;
    case core::CallEndReason::Unavailable:
        return CallViewModel::Ending::Unavailable;
    case core::CallEndReason::NumberNotFound:
        return CallViewModel::Ending::NumberNotFound;
    case core::CallEndReason::NotAllowed:
        return CallViewModel::Ending::NotAllowed;
    case core::CallEndReason::MediaMismatch:
        return CallViewModel::Ending::MediaMismatch;
    case core::CallEndReason::ServerError:
        return CallViewModel::Ending::ServerError;
    case core::CallEndReason::Unreachable:
        return CallViewModel::Ending::Unreachable;
    case core::CallEndReason::NoAudioDevice:
        return CallViewModel::Ending::NoAudioDevice;
    case core::CallEndReason::Failed:
        break;
    }
    return CallViewModel::Ending::Failed;
}

[[nodiscard]] CallViewModel::Tone toViewTone(core::CallTone tone) noexcept
{
    switch (tone) {
    case core::CallTone::None:
        return CallViewModel::Tone::None;
    case core::CallTone::Ringtone:
        return CallViewModel::Tone::Ringtone;
    case core::CallTone::Ringback:
        return CallViewModel::Tone::Ringback;
    case core::CallTone::Disconnect:
        return CallViewModel::Tone::Disconnect;
    case core::CallTone::AutoAnswered:
        return CallViewModel::Tone::AutoAnswered;
    case core::CallTone::Busy:
        return CallViewModel::Tone::Engaged;
    }
    return CallViewModel::Tone::None;
}

[[nodiscard]] qint64 msSinceEpoch(std::chrono::steady_clock::time_point when) noexcept
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(when.time_since_epoch()).count();
}

}

core::CallTone toCoreTone(CallViewModel::Tone tone) noexcept
{
    switch (tone) {
    case CallViewModel::Tone::None:
        return core::CallTone::None;
    case CallViewModel::Tone::Ringtone:
        return core::CallTone::Ringtone;
    case CallViewModel::Tone::Ringback:
        return core::CallTone::Ringback;
    case CallViewModel::Tone::Disconnect:
        return core::CallTone::Disconnect;
    case CallViewModel::Tone::AutoAnswered:
        return core::CallTone::AutoAnswered;
    case CallViewModel::Tone::Engaged:
        return core::CallTone::Busy;
    }
    return core::CallTone::None;
}

CallViewModel::CallViewModel(core::CallController &calls, core::RedialController &redial,
                             Dispatcher dispatch, Clock clock, QObject *parent)
    : QObject{parent}, calls_{calls}, redial_{redial}, dispatch_{std::move(dispatch)},
      clock_{std::move(clock)}
{
    if (!clock_) {
        clock_ = []() { return std::chrono::steady_clock::now(); };
    }

    connect(this, &CallViewModel::callChanged, this, &CallViewModel::recordingChanged);
}

CallViewModel::~CallViewModel() = default;

bool CallViewModel::handedOverAndGone(const core::CallInfo &call) const noexcept
{

    if (core::talkingToOurselves(call, lineOf(call).username) && call.transfer.askedByThem &&
        call.transfer.inFlight()) {
        return true;
    }

    const bool ourOwnBlindTransfer =
        !call.transfer.askedByThem && call.transfer.otherCallId.empty();
    return ourOwnBlindTransfer &&
           (call.transfer.inFlight() || call.transfer.state == core::TransferState::Succeeded);
}

const core::CallInfo *CallViewModel::foreground() const noexcept
{

    const auto remember = [this](const core::CallInfo *call) {
        showing_ = call != nullptr ? call->id : std::string{};
        return call;
    };

    if (!selected_.empty()) {
        for (const auto &call : calls_mirror_) {
            if (call.id == selected_ && !handedOverAndGone(call)) {
                return remember(&call);
            }
        }
    }

    for (const auto &call : calls_mirror_) {
        if (call.holdsTheAudio() && !handedOverAndGone(call)) {
            return remember(&call);
        }
    }

    const auto eligible = [this](const core::CallInfo &call, bool activeToo) {
        if (handedOverAndGone(call) || call.state == core::CallState::Ended) {
            return false;
        }
        return activeToo || call.state != core::CallState::Active;
    };

    const auto onTheCardAlready = [this, &eligible](bool activeToo) -> const core::CallInfo * {
        if (showing_.empty()) {
            return nullptr;
        }
        for (const auto &call : calls_mirror_) {
            if (call.id == showing_ && eligible(call, activeToo)) {
                return &call;
            }
        }
        return nullptr;
    };

    if (const auto *staying = onTheCardAlready(false)) {
        return remember(staying);
    }
    for (const auto &call : calls_mirror_) {
        if (eligible(call, false)) {
            return remember(&call);
        }
    }
    if (const auto *staying = onTheCardAlready(true)) {
        return remember(staying);
    }
    for (const auto &call : calls_mirror_) {
        if (eligible(call, true)) {
            return remember(&call);
        }
    }

    const core::CallInfo *latest = nullptr;
    std::chrono::steady_clock::time_point when{};
    for (const auto &call : calls_mirror_) {
        const auto at = endedAt_.find(call.id);
        if (at == endedAt_.end() || handedOverAndGone(call)) {
            continue;
        }
        if (latest == nullptr || at->second >= when) {
            latest = &call;
            when = at->second;
        }
    }
    return remember(latest);
}

bool CallViewModel::busy() const noexcept
{

    return std::any_of(calls_mirror_.begin(), calls_mirror_.end(), [this](const auto &call) {
        return call.state != core::CallState::Ended && !handedOverAndGone(call);
    });
}

bool CallViewModel::hasCall() const noexcept
{
    return foreground() != nullptr;
}

bool CallViewModel::ended() const noexcept
{
    const auto *call = foreground();
    return call != nullptr && call->state == core::CallState::Ended;
}

CallViewModel::Ending CallViewModel::ending() const noexcept
{
    const auto *call = foreground();
    if (call == nullptr || call->state != core::CallState::Ended) {
        return Ending::None;
    }
    return toViewEnding(call->end.reason);
}

const core::CallInfo *CallViewModel::offered() const noexcept
{
    for (const auto &call : calls_mirror_) {
        if (call.state == core::CallState::RingingIn) {
            return &call;
        }
    }
    return nullptr;
}

CallViewModel::State CallViewModel::state() const noexcept
{
    const auto *call = foreground();
    return call != nullptr ? toViewState(call->state) : State::Idle;
}

CallViewModel::Direction CallViewModel::direction() const noexcept
{
    const auto *call = foreground();
    if (call != nullptr && call->direction == core::CallDirection::Incoming) {
        return Direction::Incoming;
    }
    return Direction::Outgoing;
}

QString CallViewModel::remoteLabel() const
{
    const auto *call = foreground();
    return call != nullptr ? QString::fromStdString(call->remoteLabel()) : QString{};
}

QString CallViewModel::currentCallId() const
{
    const auto *call = foreground();
    return call != nullptr ? QString::fromStdString(call->id) : QString{};
}

QString CallViewModel::accountId() const
{
    const auto *call = foreground();
    return call != nullptr ? QString::fromStdString(call->accountId) : QString{};
}

QString CallViewModel::remoteDetail() const
{
    const auto *call = foreground();
    return call != nullptr ? QString::fromStdString(call->remoteDetail()) : QString{};
}

bool CallViewModel::muted() const noexcept
{
    const auto *call = foreground();
    return call != nullptr && call->muted;
}

bool CallViewModel::incoming() const noexcept
{
    const auto *call = foreground();
    return call != nullptr && call->state == core::CallState::RingingIn;
}

bool CallViewModel::talking() const noexcept
{
    const auto *call = foreground();
    return call != nullptr && call->state == core::CallState::Active;
}

bool CallViewModel::canSendDtmf() const noexcept
{
    const auto *call = foreground();
    return call != nullptr && call->canSendDtmf();
}

bool CallViewModel::held() const noexcept
{
    const auto *call = foreground();
    return call != nullptr && call->heldByUs;
}

bool CallViewModel::heldByThem() const noexcept
{
    const auto *call = foreground();
    return call != nullptr && call->heldByThem;
}

bool CallViewModel::offeringWhileBusy() const noexcept
{
    const auto *ringing = offered();
    if (ringing == nullptr) {
        return false;
    }

    for (const auto &call : calls_mirror_) {
        if (call.id != ringing->id && call.state == core::CallState::Active) {
            return true;
        }
    }
    return false;
}

QString CallViewModel::offeredLabel() const
{
    const auto *ringing = offered();
    return ringing != nullptr ? QString::fromStdString(ringing->remoteLabel()) : QString{};
}

QString CallViewModel::offeredDetail() const
{
    const auto *ringing = offered();
    return ringing != nullptr ? QString::fromStdString(ringing->remoteDetail()) : QString{};
}

qint64 CallViewModel::connectedAtMs() const
{
    const auto *call = foreground();
    if (call == nullptr || !call->everConnected()) {
        return 0;
    }
    return msSinceEpoch(*call->connectedAt);
}

qint64 CallViewModel::endedAtMs() const
{
    const auto *call = foreground();
    if (call == nullptr) {
        return 0;
    }
    const auto at = endedAt_.find(call->id);
    return at == endedAt_.end() ? qint64{0} : msSinceEpoch(at->second);
}

qint64 CallViewModel::startedAtMs() const
{
    const auto *call = foreground();
    return call != nullptr && call->startedAt.has_value() ? msSinceEpoch(*call->startedAt)
                                                          : qint64{0};
}

qint64 CallViewModel::nowMs() const
{
    return msSinceEpoch(clock_());
}

namespace {

[[nodiscard]] CallViewModel::Transfer toViewTransfer(core::TransferState state)
{
    switch (state) {
    case core::TransferState::None:
        return CallViewModel::Transfer::NoTransfer;
    case core::TransferState::Requested:
        return CallViewModel::Transfer::TransferRequested;
    case core::TransferState::Ringing:
        return CallViewModel::Transfer::TransferRinging;
    case core::TransferState::Succeeded:
        return CallViewModel::Transfer::TransferSucceeded;
    case core::TransferState::Failed:
        break;
    }
    return CallViewModel::Transfer::TransferFailed;
}

}

CallViewModel::Transfer CallViewModel::transfer() const
{
    const auto *call = foreground();
    if (call == nullptr || routedByTheSwitch_.contains(call->id)) {

        return Transfer::NoTransfer;
    }
    return toViewTransfer(call->transfer.state);
}

QString CallViewModel::transferTarget() const
{
    const auto *call = foreground();
    if (call == nullptr) {
        return {};
    }

    return QString::fromStdString(core::sipUserPart(call->transfer.target));
}

int CallViewModel::transferStatus() const
{
    const auto *call = foreground();
    return call == nullptr ? 0 : call->transfer.sipStatus;
}

bool CallViewModel::transferUnanswered() const
{
    const auto *call = foreground();
    return call != nullptr && call->transfer.switchNeverAnswered();
}

bool CallViewModel::canTransfer() const
{
    const auto *call = foreground();
    return call != nullptr && call->canTransfer();
}

QVariantList CallViewModel::transferTargets() const
{
    const auto *call = foreground();
    if (call == nullptr) {
        return {};
    }

    QVariantList rows;
    for (const auto &target : core::transferCandidates(calls_mirror_, call->id)) {
        QVariantMap row;
        row["callId"] = QString::fromStdString(target.id);
        row["who"] = QString::fromStdString(target.remoteLabel());
        row["number"] = QString::fromStdString(target.remoteDetail());
        rows.append(row);
    }

    return rows;
}

QString CallViewModel::referredBy() const
{
    const auto *call = foreground();
    if (call == nullptr || !call->transfer.askedByThem ||
        !core::somebodyElseTransferred(call->transfer.referredBy, lineOf(*call))) {
        return {};
    }

    if (const auto since = explainedAt_.find(call->id);
        since != explainedAt_.end() && clock_() - since->second.at >= endedLinger_) {
        return {};
    }

    return QString::fromStdString(core::sipUserPart(call->transfer.referredBy));
}

bool CallViewModel::inConference() const noexcept
{
    const auto *call = foreground();

    return call != nullptr && call->inConference && call->state == core::CallState::Active;
}

QVariantList CallViewModel::conferenceList() const
{
    QVariantList rows;
    for (const auto &call : calls_mirror_) {
        if (!call.inConference || call.state != core::CallState::Active) {
            continue;
        }

        QVariantMap row;
        row["callId"] = QString::fromStdString(call.id);
        row["who"] = QString::fromStdString(call.remoteLabel());
        row["number"] = QString::fromStdString(call.remoteDetail());
        row["connectedAtMs"] = call.everConnected() ? msSinceEpoch(*call.connectedAt) : qint64{0};

        row["heldByThem"] = call.heldByThem;
        rows.append(row);
    }
    return rows;
}

bool CallViewModel::canMerge() const
{
    return !core::conferenceCandidates(calls_mirror_).empty();
}

QString CallViewModel::mergePartner() const
{
    const auto *call = foreground();
    if (call == nullptr) {
        return {};
    }

    const auto joining = core::conferenceCandidates(calls_mirror_);
    if (joining.size() != 2) {
        return {};
    }

    if (joining.front().id != call->id && joining.back().id != call->id) {
        return {};
    }

    const auto &other = joining.front().id == call->id ? joining.back() : joining.front();
    return QString::fromStdString(other.remoteLabel());
}

QVariantList CallViewModel::callList() const
{
    const auto *shown = foreground();

    QVariantList rows;
    for (const auto &call : calls_mirror_) {
        if (handedOverAndGone(call)) {
            continue;
        }

        QVariantMap row;
        row["callId"] = QString::fromStdString(call.id);

        row["accountId"] = QString::fromStdString(call.accountId);
        row["who"] = QString::fromStdString(call.remoteLabel());
        row["number"] = QString::fromStdString(call.remoteDetail());

        row["state"] = static_cast<int>(toViewState(call.state));
        row["ending"] = static_cast<int>(
            call.state == core::CallState::Ended ? toViewEnding(call.end.reason) : Ending::None);
        row["held"] = call.heldByUs;
        row["heldByThem"] = call.heldByThem;
        row["current"] = shown != nullptr && shown->id == call.id;
        row["live"] = call.holdsTheAudio();
        row["inConference"] = call.inConference;

        row["recording"] = recordingCovers(call.id);
        row["connectedAtMs"] = call.everConnected() ? msSinceEpoch(*call.connectedAt) : qint64{0};

        const auto endedAt = endedAt_.find(call.id);
        row["endedAtMs"] = endedAt == endedAt_.end() ? qint64{0} : msSinceEpoch(endedAt->second);
        rows.append(row);
    }
    return rows;
}

bool CallViewModel::moreThanTheCard() const
{

    const auto *card = foreground();
    if (card == nullptr) {
        return false;
    }
    const bool room = card->inConference && card->state == core::CallState::Active;

    for (const auto &call : calls_mirror_) {
        if (handedOverAndGone(call) || call.id == card->id) {
            continue;
        }
        if (call.state == core::CallState::Ended) {
            continue;
        }
        if (room && call.inConference && call.state == core::CallState::Active) {
            continue;
        }
        return true;
    }
    return false;
}

bool CallViewModel::canDial() const
{
    return !core::dialTarget(dialText_.toStdString(), account_).empty();
}

void CallViewModel::setDialText(const QString &value)
{
    if (dialText_ == value) {
        return;
    }
    dialText_ = value;
    Q_EMIT dialTextChanged();
}

QString CallViewModel::dialable(const QString &typed) const
{
    return QString::fromStdString(core::dialable(typed.toStdString()));
}

void CallViewModel::setLines(std::vector<core::AccountConfig> lines)
{
    lines_ = std::move(lines);
    announceCallChanged();
}

bool CallViewModel::carriedOnHere(const core::CallInfo &call) const noexcept
{

    for (const auto &other : calls_mirror_) {
        if (other.id != call.id && other.transfer.continuation &&
            other.transfer.otherCallId == call.id && other.state != core::CallState::Ended) {
            return true;
        }
    }
    return false;
}

const core::AccountConfig &CallViewModel::lineOf(const core::CallInfo &call) const noexcept
{
    for (const auto &line : lines_) {
        if (line.id == call.accountId) {
            return line;
        }
    }
    return account_;
}

void CallViewModel::setAccount(const core::AccountConfig &account)
{
    account_ = account;
    Q_EMIT dialTextChanged();
}

void CallViewModel::apply(const core::CallInfo &call)
{

    noticeTheLastNumber();

    const bool wasRinging = ringing_;

    const auto at = std::find_if(calls_mirror_.begin(), calls_mirror_.end(),
                                 [&call](const auto &known) { return known.id == call.id; });

    if (at == calls_mirror_.end() && call.state == core::CallState::Ended) {
        return;
    }

    if (!call.transfer.otherCallId.empty() && call.transfer.askedByThem &&
        core::talkingToOurselves(call, lineOf(call).username)) {
        routedByTheSwitch_.insert(call.transfer.otherCallId);
    }

    if (call.state == core::CallState::Ended &&
        (call.end.reason == core::CallEndReason::Replaced || carriedOnHere(call))) {
        forget(call.id);

        ringing_ = offered() != nullptr;
        announceCallChanged();
        if (!ringing_ && wasRinging) {
            Q_EMIT ringingStopped();
        }
        return;
    }

    if (call.state == core::CallState::Ended) {
        forgetTheQuestion(QString::fromStdString(call.id));
    }

    if (call.state == core::CallState::Ended && call.id == selected_) {

        selected_.clear();
    }

    saidBySwitch_[call.id] = call.remoteDisplayName;
    auto shown = core::asKnown(call, contacts_);

    if (at != calls_mirror_.end()) {
        *at = std::move(shown);
    } else {
        calls_mirror_.push_back(std::move(shown));
    }

    if (call.state == core::CallState::Ended) {

        const auto endedAt = clock_();
        endedAt_[call.id] = endedAt;

        if (!call.transfer.otherCallId.empty()) {
            if (const auto partner = endedAt_.find(call.transfer.otherCallId);
                partner != endedAt_.end()) {
                partner->second = endedAt;
            }
        }
    }

    if (call.state == core::CallState::Active && !call.transfer.referredBy.empty() &&
        !explainedAt_.contains(call.id)) {
        explainedAt_[call.id] = Explanation{clock_(), false};
    }

    const auto *arriving = offered();
    ringing_ = arriving != nullptr && core::ringsHere(*arriving);

    announceCallChanged();

    if (ringing_ && !wasRinging) {

        core::Logger::instance().log(core::LogLevel::Debug, "ui",
                                     "the phone started ringing: " + call.id);
        Q_EMIT ringingStarted();
    } else if (!ringing_ && wasRinging) {
        Q_EMIT ringingStopped();
    }

    announceTone(call);
}

void CallViewModel::announceTone(const core::CallInfo &call)
{
    const auto tone = core::continuousTone(calls_mirror_);

    if (tone != tone_) {
        tone_ = tone;
        Q_EMIT toneWanted(toViewTone(tone));

        if (tone != core::CallTone::None) {
            return;
        }
    }

    if (tone == core::CallTone::None && core::endingIsAudible(call)) {
        Q_EMIT toneWanted(Tone::Disconnect);
        return;
    }

    if (tone == core::CallTone::None && core::failureIsAudible(call)) {
        Q_EMIT toneWanted(Tone::Engaged);
        return;
    }

    if (tone == core::CallTone::None && core::answerIsAudible(call) &&
        announcedAnswerOf_.insert(call.id).second) {
        Q_EMIT toneWanted(Tone::AutoAnswered);
    }
}

void CallViewModel::dial()
{
    if (!canDial()) {
        return;
    }

    const auto typed = dialText_.toStdString();
    const auto target = core::dialTarget(typed, account_);
    const auto accountId = account_.id;

    dialText_.clear();
    Q_EMIT dialTextChanged();

    dispatch_([this, accountId, target, typed]() {
        static_cast<void>(calls_.place(accountId, target, typed));
    });
}

void CallViewModel::toggleDoNotDisturb()
{

    calls_.setDoNotDisturb(!calls_.doNotDisturb());
    Q_EMIT doNotDisturbChanged();
}

void CallViewModel::offerLastDialled()
{
    const auto again = lastDialled();
    if (again.isEmpty() || !dialText_.isEmpty()) {
        return;
    }
    dialText_ = again;
    Q_EMIT dialTextChanged();
}

void CallViewModel::noticeTheLastNumber()
{

    if (const auto now = lastDialled(); now != lastDialledWas_) {
        lastDialledWas_ = now;
        Q_EMIT lastDialledChanged();
    }
}

void CallViewModel::answer()
{
    const auto *ringing = offered();
    if (ringing == nullptr) {
        return;
    }

    const auto id = ringing->id;
    dispatch_([this, id]() { calls_.answer(id, core::AnswerMode::HoldOthers); });
}

void CallViewModel::answerEndingOthers()
{
    const auto *ringing = offered();
    if (ringing == nullptr) {
        return;
    }

    const auto id = ringing->id;
    dispatch_([this, id]() { calls_.answer(id, core::AnswerMode::EndOthers); });
}

void CallViewModel::reject()
{
    const auto *ringing = offered();
    if (ringing == nullptr) {
        return;
    }

    const auto id = ringing->id;
    dispatch_([this, id]() { calls_.reject(id); });
}

void CallViewModel::hangUp()
{
    const auto *call = foreground();
    if (call == nullptr) {
        return;
    }

    const auto id = call->id;
    dispatch_([this, id]() { calls_.hangUp(id); });
}

void CallViewModel::toggleMute()
{
    const auto *call = foreground();
    if (call == nullptr || call->state != core::CallState::Active) {
        return;
    }

    const auto id = call->id;
    const auto muted = !call->muted;
    dispatch_([this, id, muted]() { calls_.setMuted(id, muted); });
}

void CallViewModel::toggleHold()
{
    const auto *call = foreground();
    if (call == nullptr || call->state != core::CallState::Active) {
        return;
    }

    const auto id = call->id;
    const auto parked = call->heldByUs;
    dispatch_([this, id, parked]() {
        if (parked) {
            calls_.activate(id);
        } else {
            calls_.hold(id);
        }
    });
}

void CallViewModel::select(const QString &callId)
{
    const auto id = callId.toStdString();
    if (selected_ == id) {
        return;
    }

    selected_ = id;
    announceCallChanged();
}

QString CallViewModel::redialNumber() const
{

    return QString::fromStdString(campaign_.dialed.empty() ? campaign_.target : campaign_.dialed);
}

qint64 CallViewModel::redialNextAtMs() const
{
    return campaign_.nextAttemptAt.has_value() ? msSinceEpoch(*campaign_.nextAttemptAt) : qint64{0};
}

qint64 CallViewModel::redialGiveUpAtMs() const
{
    return campaign_.running ? msSinceEpoch(campaign_.giveUpAt) : qint64{0};
}

CallViewModel::Redial CallViewModel::redialEnded() const noexcept
{
    if (!campaign_.finished.has_value()) {
        return Redial::Running;
    }

    switch (*campaign_.finished) {
    case core::RedialOutcome::GotThrough:
        return Redial::Answered;
    case core::RedialOutcome::Cancelled:
        return Redial::Stopped;
    case core::RedialOutcome::Refused:
        return Redial::Pointless;
    case core::RedialOutcome::GaveUp:
        return Redial::Exhausted;
    }
    return Redial::Running;
}

bool CallViewModel::canRedial() const
{
    const auto *call = foreground();
    if (call == nullptr || call->state != core::CallState::Ended) {
        return false;
    }

    if (call->direction != core::CallDirection::Outgoing) {
        return false;
    }

    return core::worthRedialling(call->end.reason) && !call->remoteUri.empty();
}

void CallViewModel::startRedial()
{
    const auto *call = foreground();
    if (call == nullptr || !canRedial()) {
        return;
    }

    const auto accountId = call->accountId;
    const auto target = call->remoteUri;
    const auto dialed = call->dialedNumber.empty() ? target : call->dialedNumber;

    startedRedialFrom_ = call->id;

    dispatch_([this, accountId, target, dialed, from = startedRedialFrom_]() {
        redial_.start(accountId, target, dialed, from);
    });
}

void CallViewModel::cancelRedial()
{
    dispatch_([this]() { redial_.cancel(); });
}

void CallViewModel::applyRedial(const core::RedialController::State &state)
{

    const auto drop = [this](const std::string &callId) {
        if (callId.empty()) {
            return;
        }
        endedAt_.erase(callId);
        explainedAt_.erase(callId);
        forget(callId);
        announceCallChanged();
    };

    if (!campaign_.lastAttemptId.empty() && state.lastAttemptId != campaign_.lastAttemptId) {
        drop(campaign_.lastAttemptId);
    }

    if (!state.lastAttemptId.empty() && state.lastAttemptId != startedRedialFrom_) {
        drop(std::exchange(startedRedialFrom_, std::string{}));
    }

    campaign_ = state;
    Q_EMIT redialChanged();

    if (state.finished.has_value() && *state.finished == core::RedialOutcome::GotThrough) {
        Q_EMIT redialGotThrough();
    }
}

void CallViewModel::sendDtmf(const QString &digit)
{
    const auto *call = foreground();
    if (call == nullptr || digit.isEmpty()) {
        return;
    }

    const auto id = call->id;
    const auto key = digit.at(0).toLatin1();
    dispatch_([this, id, key]() { calls_.sendDtmf(id, key); });

    if (tone_ == core::CallTone::None) {
        Q_EMIT dtmfPressed(QString{QChar::fromLatin1(key)});
    }
}

void CallViewModel::merge()
{
    std::vector<std::string> joining;
    for (const auto &call : core::conferenceCandidates(calls_mirror_)) {
        joining.push_back(call.id);
    }

    dispatch_([this, joining]() { calls_.merge(joining); });
}

void CallViewModel::split()
{
    const auto *call = foreground();
    if (call == nullptr || !call->inConference) {
        return;
    }

    const auto id = call->id;
    dispatch_([this, id]() { calls_.split(id); });
}

void CallViewModel::toggleHoldCall(const QString &callId)
{
    const auto wanted = callId.toStdString();
    const auto found = std::ranges::find(calls_mirror_, wanted, &core::CallInfo::id);

    if (found == calls_mirror_.end() || found->state != core::CallState::Active) {
        return;
    }

    const auto parked = found->heldByUs;
    dispatch_([this, wanted, parked]() {
        if (parked) {
            calls_.activate(wanted);
        } else {
            calls_.hold(wanted);
        }
    });
}

void CallViewModel::rejectCall(const QString &callId)
{
    const auto id = callId.toStdString();
    dispatch_([this, id]() { calls_.reject(id); });
}

void CallViewModel::hangUpCall(const QString &callId)
{
    const auto id = callId.toStdString();
    dispatch_([this, id]() { calls_.hangUp(id); });
}

void CallViewModel::hangUpConference()
{
    if (!inConference()) {
        return;
    }

    dispatch_([this]() { calls_.endConference(); });
}

void CallViewModel::beginTransfer()
{
    const auto *call = foreground();
    if (call == nullptr) {
        return;
    }

    const auto id = call->id;
    dispatch_([this, id]() { calls_.beginTransfer(id); });
}

void CallViewModel::cancelTransfer()
{
    const auto *call = foreground();
    if (call == nullptr) {
        return;
    }

    const auto id = call->id;
    dispatch_([this, id]() { calls_.cancelTransfer(id); });
}

void CallViewModel::transferToCall(const QString &callId)
{
    const auto *call = foreground();
    if (call == nullptr || callId.isEmpty()) {
        return;
    }

    const auto self = call->id;
    const auto other = callId.toStdString();
    dispatch_([this, self, other]() { calls_.transferAttended(self, other); });
}

void CallViewModel::transferNow(const QString &target)
{
    const auto *call = foreground();
    if (call == nullptr || target.trimmed().isEmpty()) {
        return;
    }

    const auto self = call->id;

    const auto where = core::dialTarget(target.trimmed().toStdString(), lineOf(*call));
    dispatch_([this, self, where]() { calls_.transferBlind(self, where); });
}

void CallViewModel::askFirst(const QString &target)
{
    if (target.trimmed().isEmpty()) {
        return;
    }

    const auto *about = foreground();
    const auto &line = about != nullptr ? lineOf(*about) : account_;
    const auto accountId = line.id;
    const auto typed = target.trimmed().toStdString();
    const auto where = core::dialTarget(typed, line);

    const auto seance = about != nullptr ? about->seanceId : std::string{};
    dispatch_([this, accountId, where, typed, seance]() {
        static_cast<void>(calls_.place(accountId, where, typed, 0, {}, seance));
    });
}

void CallViewModel::setContacts(std::vector<core::Contact> contacts)
{
    contacts_.reset(std::move(contacts));

    for (auto &call : calls_mirror_) {
        const auto said = saidBySwitch_.find(call.id);
        if (said != saidBySwitch_.end()) {
            call.remoteDisplayName = said->second;
        }
        call = core::asKnown(call, contacts_);
    }

    announceCallChanged();
}

void CallViewModel::tick()
{

    const bool dropped = expireEnded();
    if (expireExplanations() || dropped) {
        announceCallChanged();
    }
    Q_EMIT nowMsChanged();
}

void CallViewModel::announceCallChanged()
{
    Q_EMIT callChanged();

    if (const auto now = hasCall(); now != hasCallAnnounced_) {
        hasCallAnnounced_ = now;
        Q_EMIT hasCallChanged();
    }
}

bool CallViewModel::expireExplanations()
{
    const auto now = clock_();
    bool expired = false;

    for (auto at = explainedAt_.begin(); at != explainedAt_.end();) {
        const bool known = std::any_of(calls_mirror_.begin(), calls_mirror_.end(),
                                       [&at](const auto &call) { return call.id == at->first; });
        if (!known) {

            at = explainedAt_.erase(at);
            continue;
        }

        if (!at->second.faded && now - at->second.at >= endedLinger_) {
            at->second.faded = true;
            expired = true;
        }
        ++at;
    }

    return expired;
}

void CallViewModel::forget(const std::string &callId)
{
    routedByTheSwitch_.erase(callId);

    if (callId == selected_) {

        selected_.clear();
    }

    const auto gone = std::remove_if(calls_mirror_.begin(), calls_mirror_.end(),
                                     [&callId](const auto &call) { return call.id == callId; });
    calls_mirror_.erase(gone, calls_mirror_.end());

    endedAt_.erase(callId);
    explainedAt_.erase(callId);
    saidBySwitch_.erase(callId);
    announcedAnswerOf_.erase(callId);
}

bool CallViewModel::expireEnded()
{
    if (endedAt_.empty()) {
        return false;
    }

    const auto now = clock_();
    bool removed = false;

    for (auto at = endedAt_.begin(); at != endedAt_.end();) {
        if (now - at->second < endedLinger_) {
            ++at;
            continue;
        }

        if (campaign_.running && at->first == campaign_.lastAttemptId) {
            ++at;
            continue;
        }

        const auto id = at->first;
        at = endedAt_.erase(at);

        const auto before = calls_mirror_.size();
        forget(id);
        removed = removed || calls_mirror_.size() != before;
    }

    return removed;
}

bool CallViewModel::recordingCovers(const std::string &callId) const noexcept
{
    return std::any_of(recordings_.begin(), recordings_.end(),
                       [&](const core::RecordingController::State &state) {
                           return state.running && std::find(state.calls.begin(), state.calls.end(),
                                                             callId) != state.calls.end();
                       });
}

const core::RecordingController::State *CallViewModel::recordingOfTheCard() const noexcept
{
    const auto *call = foreground();
    if (call == nullptr) {
        return nullptr;
    }

    const core::RecordingController::State *own = nullptr;
    for (const auto &state : recordings_) {
        if (!state.running) {
            continue;
        }
        if (std::find(state.calls.begin(), state.calls.end(), call->id) == state.calls.end()) {
            continue;
        }
        if (state.conference) {
            return &state;
        }
        if (own == nullptr) {
            own = &state;
        }
    }
    return own;
}

bool CallViewModel::canRecord() const noexcept
{

    if (!startRecording_) {
        return false;
    }

    const auto *call = foreground();
    return call != nullptr && call->state == core::CallState::Active;
}

bool CallViewModel::canStartRecording() const noexcept
{

    const auto *call = foreground();
    if (call == nullptr) {
        return false;
    }
    return !call->heldByUs || recording();
}

void CallViewModel::setRecordingStates(std::vector<core::RecordingController::State> states)
{

    const auto same = [](const core::RecordingController::State &one,
                         const core::RecordingController::State &other) {
        return one.running == other.running && one.paused == other.paused &&
               one.keeping == other.keeping && one.recordingId == other.recordingId &&
               one.calls == other.calls;
    };

    if (states.size() == recordings_.size() &&
        std::equal(states.begin(), states.end(), recordings_.begin(), same)) {
        return;
    }

    recordings_ = std::move(states);

    const auto stale = std::remove_if(asked_.begin(), asked_.end(), [this](const QString &callId) {
        return recordingCovers(callId.toStdString());
    });
    asked_.erase(stale, asked_.end());

    Q_EMIT recordingChanged();

    announceCallChanged();
}

bool CallViewModel::askingToRecord() const noexcept
{

    const auto *call = foreground();
    if (call == nullptr) {
        return false;
    }

    const auto shown = QString::fromStdString(call->id);
    return std::find(asked_.begin(), asked_.end(), shown) != asked_.end();
}

void CallViewModel::askAboutRecording(const QString &callId)
{
    if (callId.isEmpty() || std::find(asked_.begin(), asked_.end(), callId) != asked_.end()) {
        return;
    }

    if (recordingCovers(callId.toStdString())) {
        return;
    }

    asked_.push_back(callId);
    Q_EMIT recordingChanged();
}

void CallViewModel::forgetTheQuestion(const QString &callId)
{
    const auto at = std::find(asked_.begin(), asked_.end(), callId);
    if (at == asked_.end()) {
        return;
    }
    asked_.erase(at);
    Q_EMIT recordingChanged();
}

void CallViewModel::answerRecording(bool yes)
{

    const auto *call = foreground();
    if (call == nullptr) {
        return;
    }

    const auto callId = QString::fromStdString(call->id);
    const auto at = std::find(asked_.begin(), asked_.end(), callId);
    if (at == asked_.end()) {
        return;
    }

    asked_.erase(at);
    Q_EMIT recordingChanged();

    if (yes && startRecording_) {
        startRecording_(callId);
    }
}

void CallViewModel::setRecordingActions(std::function<void(const QString &)> start,
                                        std::function<void(const QString &, bool)> pause)
{
    startRecording_ = std::move(start);
    pauseRecording_ = std::move(pause);
    Q_EMIT recordingChanged();
}

void CallViewModel::refuseRecording(const QString &callId, core::EncoderStatus why)
{

    const auto *shown = foreground();
    if (shown == nullptr || QString::fromStdString(shown->id) != callId) {
        return;
    }

    Q_EMIT recordingRefused(why == core::EncoderStatus::NoRoom ? Refusal::NoRoom
                                                               : Refusal::Unknown);
}

void CallViewModel::toggleRecording()
{
    const auto *call = foreground();
    if (call == nullptr) {
        return;
    }

    if (const auto *mine = recordingOfTheCard(); mine != nullptr) {
        if (pauseRecording_) {
            pauseRecording_(QString::fromStdString(call->id), !mine->paused);
        }
        return;
    }

    if (!startRecording_) {
        return;
    }
    startRecording_(QString::fromStdString(call->id));
}

}
