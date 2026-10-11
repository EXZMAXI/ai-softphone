// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/redial.hpp>

#include <softphone/core/logging.hpp>

#include <algorithm>
#include <utility>

namespace softphone::core {
namespace {

void note(LogLevel level, std::string_view line, std::string_view callId, const std::string &what)
{
    const LogAttribute fields[] = {{"line", line}, {"call", callId}};
    Logger::instance().log(level, "redial", what, fields);
}

}

bool worthRedialling(CallEndReason reason) noexcept
{
    switch (reason) {
    case CallEndReason::Busy:
    case CallEndReason::Unavailable:
    case CallEndReason::Unreachable:
    case CallEndReason::ServerError:
    case CallEndReason::NoAnswer:
        return true;

    case CallEndReason::None:
    case CallEndReason::LocalHangup:
    case CallEndReason::RemoteHangup:
    case CallEndReason::Replaced:
    case CallEndReason::Transferred:
    case CallEndReason::Declined:
    case CallEndReason::PickedUpElsewhere:

    case CallEndReason::DoNotDisturb:
    case CallEndReason::TooManyCalls:
    case CallEndReason::CallWaitingOff:
    case CallEndReason::Cancelled:
    case CallEndReason::NumberNotFound:
    case CallEndReason::NotAllowed:
    case CallEndReason::MediaMismatch:
    case CallEndReason::NoAudioDevice:
    case CallEndReason::Failed:
        break;
    }
    return false;
}

std::string_view toString(RedialOutcome outcome) noexcept
{
    switch (outcome) {
    case RedialOutcome::GotThrough:
        return "got-through";
    case RedialOutcome::Cancelled:
        return "cancelled";
    case RedialOutcome::Refused:
        return "refused";
    case RedialOutcome::GaveUp:
        return "gave-up";
    }
    return "unknown";
}

RedialController::RedialController(CallController &calls, IScheduler &scheduler,
                                   RedialPolicy policy, StateCallback onChanged)
    : calls_{calls}, scheduler_{scheduler}, policy_{policy}, onChanged_{std::move(onChanged)}
{}

RedialController::~RedialController()
{
    disarm();
}

void RedialController::setPolicy(RedialPolicy policy)
{
    if (state_.running) {

        state_.giveUpAt = state_.giveUpAt - policy_.giveUpAfter + policy.giveUpAfter;
    }

    policy_ = policy;
}

void RedialController::start(std::string accountId, std::string target, std::string dialed,
                             std::string continues)
{
    disarm();

    state_ = State{};
    state_.running = true;
    state_.accountId = std::move(accountId);
    state_.target = std::move(target);
    state_.dialed = dialed.empty() ? state_.target : std::move(dialed);
    state_.continuesCallId = std::move(continues);
    state_.giveUpAt = scheduler_.now() + policy_.giveUpAfter;

    note(LogLevel::Info, state_.accountId, state_.callId,
         "redialling " + state_.target + " until it gets through");

    attempt();
}

void RedialController::cancel()
{
    if (!state_.running) {
        return;
    }
    finish(RedialOutcome::Cancelled);
}

void RedialController::onCallChanged(const CallInfo &call)
{
    if (!state_.running) {
        return;
    }

    if (call.id != state_.callId) {

        if (state_.waitingForTheLine && call.state == CallState::Ended && !lineIsBusy()) {
            attempt();
        }
        return;
    }

    if (call.state == CallState::RingingOut) {
        state_.attemptRang = true;
        announce();
        return;
    }

    if (call.state == CallState::Active) {
        note(LogLevel::Info, state_.accountId, state_.callId,
             "got through to " + state_.target + " on attempt " + std::to_string(state_.attempts));
        finish(RedialOutcome::GotThrough);
        return;
    }

    if (call.state != CallState::Ended) {
        return;
    }

    state_.callId.clear();

    if (call.end.reason == CallEndReason::NoAnswer && state_.attemptRang) {
        note(LogLevel::Info, state_.accountId, state_.callId,
             "stopping: it rang and nobody picked up");
        finish(RedialOutcome::Refused);
        return;
    }

    if (!worthRedialling(call.end.reason)) {
        note(LogLevel::Info, state_.accountId, call.id,
             "stopping: " + std::string{toString(call.end.reason)} +
                 " is not an outcome another call would change");
        finish(RedialOutcome::Refused);
        return;
    }

    armFor(policy_.interval);
    announce();
}

void RedialController::attempt()
{
    disarm();

    if (scheduler_.now() >= state_.giveUpAt) {
        note(LogLevel::Info, state_.accountId, state_.callId,
             "giving up on " + state_.target + " after " + std::to_string(state_.attempts) +
                 " attempts");
        finish(RedialOutcome::GaveUp);
        return;
    }

    if (lineIsBusy()) {

        state_.waitingForTheLine = true;
        state_.nextAttemptAt = scheduler_.now() + policy_.interval;
        armFor(policy_.interval);
        announce();
        return;
    }

    state_.waitingForTheLine = false;
    state_.nextAttemptAt.reset();
    state_.attemptRang = false;
    ++state_.attempts;

    state_.callId = calls_.place(state_.accountId, state_.target, state_.dialed, state_.attempts,
                                 state_.attempts == 1 ? state_.continuesCallId : std::string{});
    if (!state_.callId.empty()) {
        state_.lastAttemptId = state_.callId;
    }

    if (state_.callId.empty()) {

        note(LogLevel::Warning, state_.accountId, state_.callId,
             "attempt " + std::to_string(state_.attempts) +
                 " could not be placed; waiting for the line");
        state_.waitingForTheLine = true;
        state_.nextAttemptAt = scheduler_.now() + policy_.interval;
        armFor(policy_.interval);
    }

    announce();
}

void RedialController::armFor(std::chrono::milliseconds delay)
{
    disarm();

    state_.nextAttemptAt = scheduler_.now() + delay;
    timer_ = scheduler_.scheduleAfter(delay, [this]() {
        timer_ = IScheduler::kNoTimer;
        attempt();
    });
}

void RedialController::disarm()
{
    if (timer_ != IScheduler::kNoTimer) {
        scheduler_.cancel(timer_);
        timer_ = IScheduler::kNoTimer;
    }
}

void RedialController::finish(RedialOutcome outcome)
{
    disarm();

    state_.running = false;
    state_.waitingForTheLine = false;
    state_.nextAttemptAt.reset();
    state_.finished = outcome;

    announce();
}

void RedialController::announce()
{
    if (onChanged_) {
        onChanged_(state_);
    }
}

bool RedialController::lineIsBusy() const
{
    const auto calls = calls_.calls();
    return std::ranges::any_of(calls, [this](const CallInfo &call) {
        return call.id != state_.callId && call.state != CallState::Ended;
    });
}

}
