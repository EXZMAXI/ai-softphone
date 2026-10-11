// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/registration_controller.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/metrics.hpp>

#include <algorithm>
#include <utility>

namespace softphone::core {
namespace {

constexpr double kRefreshFraction = 0.9;

}

RegistrationController::RegistrationController(AccountConfig config, ISipBackend &backend,
                                               IScheduler &scheduler, RegistrationPolicy policy,
                                               JitterSource jitter, StateCallback onStateChanged)
    : config_{std::move(config)}, backend_{backend}, scheduler_{scheduler}, policy_{policy},
      jitter_{std::move(jitter)}, onStateChanged_{std::move(onStateChanged)}
{}

RegistrationController::~RegistrationController()
{

    cancelRetry();
    cancelRefresh();
    cancelAttemptTimeout();
}

void RegistrationController::setSelfRefresh(bool enabled)
{
    selfRefresh_ = enabled;
    if (!selfRefresh_) {
        cancelRefresh();
    }
}

void RegistrationController::start()
{
    if (state_ == RegistrationState::Registering || state_ == RegistrationState::Registered) {
        return;
    }

    const LogAttribute fields[] = {{"line", config_.id}};
    Logger::instance().log(LogLevel::Info, "registration",
                           "switched on, registering as " + config_.sipUri(), fields);

    cancelRetry();
    cancelRefresh();
    cancelAttemptTimeout();
    attempt_ = 0;
    beginRegistration();
}

void RegistrationController::stop()
{
    const LogAttribute fields[] = {{"line", config_.id}};
    Logger::instance().log(LogLevel::Info, "registration", "switched off", fields);

    cancelRetry();
    cancelRefresh();
    cancelAttemptTimeout();
    attempt_ = 0;

    if (state_ == RegistrationState::Registered) {
        setState(RegistrationState::Unregistering, RegistrationError{});
        backend_.requestUnregistration(config_.id);
        return;
    }

    if (state_ != RegistrationState::Offline) {
        setState(RegistrationState::Offline, RegistrationError{});
    }
}

void RegistrationController::onOutcome(const RegistrationOutcome &outcome)
{
    if (state_ == RegistrationState::Unregistering) {

        setState(RegistrationState::Offline, RegistrationError{});
        return;
    }

    if (state_ == RegistrationState::Offline) {

        return;
    }

    cancelRetry();
    cancelRefresh();
    cancelAttemptTimeout();

    if (outcome.registered) {
        attempt_ = 0;
        everRegistered_ = true;
        metrics::registrationsSucceeded().add();
        setState(RegistrationState::Registered, RegistrationError{});
        scheduleRefresh();
        return;
    }

    metrics::registrationsFailed().add();

    auto error = classify(outcome);

    const auto mine = std::exchange(ourOwnTimeout_, false);

    if (worthRetrying(error)) {
        scheduleRetry(mine);
    }

    setState(RegistrationState::Failed, std::move(error));
}

bool RegistrationController::worthRetrying(const RegistrationError &error) const
{
    if (error.isTransient()) {
        return true;
    }

    if (!everRegistered_) {
        return false;
    }

    switch (error.category) {
    case RegistrationErrorCategory::PasswordUnavailable:
    case RegistrationErrorCategory::AuthenticationRejected:
        return false;
    case RegistrationErrorCategory::None:
    case RegistrationErrorCategory::InvalidConfiguration:
    case RegistrationErrorCategory::Forbidden:
    case RegistrationErrorCategory::AccountNotFound:
    case RegistrationErrorCategory::RegistrarUnreachable:
    case RegistrationErrorCategory::DomainNotResolved:
    case RegistrationErrorCategory::ServerUnavailable:
    case RegistrationErrorCategory::CertificateNotTrusted:
    case RegistrationErrorCategory::Unknown:
        break;
    }
    return true;
}

void RegistrationController::beginRegistration()
{
    setState(RegistrationState::Registering, RegistrationError{});

    scheduleAttemptTimeout();
    backend_.requestRegistration(config_);
}

void RegistrationController::scheduleAttemptTimeout()
{
    cancelAttemptTimeout();

    if (policy_.attemptTimeout <= std::chrono::milliseconds::zero()) {
        return;
    }

    attemptTimer_ = scheduler_.scheduleAfter(policy_.attemptTimeout, [this]() {
        attemptTimer_ = IScheduler::kNoTimer;

        ourOwnTimeout_ = true;
        onOutcome(RegistrationOutcome{false, 0, TransportFailure::Timeout,
                                      "the SIP stack did not answer in time"});
    });
}

void RegistrationController::cancelAttemptTimeout()
{
    if (attemptTimer_ != IScheduler::kNoTimer) {
        scheduler_.cancel(attemptTimer_);
        attemptTimer_ = IScheduler::kNoTimer;
    }
}

void RegistrationController::scheduleRetry(bool afterOurOwnTimeout)
{
    auto delay = backoffDelay(policy_.backoff, attempt_, jitter_ ? jitter_() : 0.5);
    ++attempt_;

    if (afterOurOwnTimeout && policy_.stackTransaction > policy_.attemptTimeout) {
        delay = std::max(delay, policy_.stackTransaction - policy_.attemptTimeout);
    }

    retryTimer_ = scheduler_.scheduleAfter(delay, [this]() {
        retryTimer_ = IScheduler::kNoTimer;
        beginRegistration();
    });
}

void RegistrationController::scheduleRefresh()
{
    if (!selfRefresh_) {
        return;
    }

    const auto expiryMs = std::chrono::duration_cast<std::chrono::milliseconds>(config_.expiry);
    const auto delay = std::chrono::milliseconds{static_cast<std::chrono::milliseconds::rep>(
        static_cast<double>(expiryMs.count()) * kRefreshFraction)};

    cancelRefresh();
    refreshTimer_ = scheduler_.scheduleAfter(delay, [this]() {
        refreshTimer_ = IScheduler::kNoTimer;

        scheduleAttemptTimeout();
        backend_.requestRegistration(config_);
    });
}

void RegistrationController::cancelRetry()
{
    if (retryTimer_ != IScheduler::kNoTimer) {
        scheduler_.cancel(retryTimer_);
        retryTimer_ = IScheduler::kNoTimer;
    }
}

void RegistrationController::cancelRefresh()
{
    if (refreshTimer_ != IScheduler::kNoTimer) {
        scheduler_.cancel(refreshTimer_);
        refreshTimer_ = IScheduler::kNoTimer;
    }
}

void RegistrationController::setState(RegistrationState state, RegistrationError error)
{
    const auto was = state_;

    state_ = state;
    lastError_ = std::move(error);

    if (was != state_) {
        const auto sipStatus = std::to_string(lastError_.sipStatus);
        const LogAttribute fields[] = {{"line", config_.id}};

        std::string what = std::string{toString(state_)};
        if (state_ == RegistrationState::Failed) {
            what += ": " + std::string{toString(lastError_.category)};
            if (lastError_.sipStatus != 0) {
                what += " (" + sipStatus + ')';
            }
            if (!lastError_.detail.empty()) {
                what += " — " + lastError_.detail;
            }
            what += retryPending() ? ", trying again" : ", not trying again";
        }

        Logger::instance().log(state_ == RegistrationState::Failed ? LogLevel::Warning
                                                                   : LogLevel::Info,
                               "registration", what, fields);
    }

    if (onStateChanged_) {
        onStateChanged_(config_.id, state_, lastError_);
    }
}

}
