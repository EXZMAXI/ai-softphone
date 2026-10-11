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
#include <softphone/core/backoff.hpp>
#include <softphone/core/registration.hpp>
#include <softphone/core/scheduler.hpp>
#include <softphone/core/sip_backend.hpp>

#include <functional>
#include <string>

namespace softphone::core {

struct RegistrationPolicy {
    BackoffPolicy backoff{};

    std::chrono::milliseconds attemptTimeout{std::chrono::seconds{15}};

    std::chrono::milliseconds stackTransaction{std::chrono::seconds{32}};
};

class RegistrationController {
public:
    using StateCallback = std::function<void(const std::string &accountId, RegistrationState state,
                                             const RegistrationError &error)>;

    RegistrationController(AccountConfig config, ISipBackend &backend, IScheduler &scheduler,
                           RegistrationPolicy policy, JitterSource jitter,
                           StateCallback onStateChanged);

    RegistrationController(const RegistrationController &) = delete;
    RegistrationController &operator=(const RegistrationController &) = delete;
    RegistrationController(RegistrationController &&) = delete;
    RegistrationController &operator=(RegistrationController &&) = delete;
    ~RegistrationController();

    void setSelfRefresh(bool enabled);

    [[nodiscard]] bool selfRefresh() const noexcept
    {
        return selfRefresh_;
    }

    void start();

    void stop();

    void onOutcome(const RegistrationOutcome &outcome);

    [[nodiscard]] RegistrationState state() const noexcept
    {
        return state_;
    }

    [[nodiscard]] const RegistrationError &lastError() const noexcept
    {
        return lastError_;
    }

    [[nodiscard]] const AccountConfig &config() const noexcept
    {
        return config_;
    }

    [[nodiscard]] bool retryPending() const noexcept
    {
        return retryTimer_ != IScheduler::kNoTimer;
    }

    [[nodiscard]] unsigned attempts() const noexcept
    {
        return attempt_;
    }

    [[nodiscard]] bool everRegistered() const noexcept
    {
        return everRegistered_;
    }

private:

    [[nodiscard]] bool worthRetrying(const RegistrationError &error) const;

    void beginRegistration();
    void scheduleAttemptTimeout();
    void cancelAttemptTimeout();
    void scheduleRetry(bool afterOurOwnTimeout);
    void scheduleRefresh();
    void cancelRetry();
    void cancelRefresh();
    void setState(RegistrationState state, RegistrationError error);

    AccountConfig config_;
    ISipBackend &backend_;
    IScheduler &scheduler_;
    RegistrationPolicy policy_;
    JitterSource jitter_;
    StateCallback onStateChanged_;

    RegistrationState state_{RegistrationState::Offline};
    RegistrationError lastError_{};
    bool selfRefresh_{true};

    bool everRegistered_{false};

    bool ourOwnTimeout_{false};

    unsigned attempt_{0};
    IScheduler::TimerId retryTimer_{IScheduler::kNoTimer};
    IScheduler::TimerId refreshTimer_{IScheduler::kNoTimer};
    IScheduler::TimerId attemptTimer_{IScheduler::kNoTimer};
};

}
