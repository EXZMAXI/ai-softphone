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
#include <softphone/core/registration_controller.hpp>
#include <softphone/core/scheduler.hpp>
#include <softphone/core/sip_backend.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace softphone::core {

class EngineListener {
public:
    EngineListener() = default;
    EngineListener(const EngineListener &) = delete;
    EngineListener &operator=(const EngineListener &) = delete;
    EngineListener(EngineListener &&) = delete;
    EngineListener &operator=(EngineListener &&) = delete;
    virtual ~EngineListener() = default;

    virtual void onRegistrationStateChanged(const std::string &accountId, RegistrationState state,
                                            const RegistrationError &error) = 0;
};

class Engine final : private ISipBackend::Observer {
public:
    Engine(ISipBackend &backend, IScheduler &scheduler, RegistrationPolicy policy = {},
           JitterSource jitter = defaultJitterSource());

    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;
    Engine(Engine &&) = delete;
    Engine &operator=(Engine &&) = delete;
    ~Engine() override;

    void setListener(EngineListener *listener) noexcept;

    void setSelfRefresh(bool enabled);

    ConfigProblem addAccount(AccountConfig config);

    bool removeAccount(const std::string &accountId);

    [[nodiscard]] bool hasAccount(const std::string &accountId) const;
    [[nodiscard]] std::vector<std::string> accountIds() const;

    bool startRegistration(const std::string &accountId);
    bool stopRegistration(const std::string &accountId);
    void stopAll();

    void addressesChanged();

    [[nodiscard]] RegistrationState registrationState(const std::string &accountId) const;
    [[nodiscard]] RegistrationError lastError(const std::string &accountId) const;
    [[nodiscard]] bool retryPending(const std::string &accountId) const;

    using ServerSeen = std::function<void(const std::string &accountId, const std::string &said)>;
    void setServerListener(ServerSeen told);

private:
    void onRegistrationOutcome(const std::string &accountId,
                               const RegistrationOutcome &outcome) override;

    ServerSeen serverSeen_;

    [[nodiscard]] RegistrationController *find(const std::string &accountId);
    [[nodiscard]] const RegistrationController *find(const std::string &accountId) const;

    ISipBackend &backend_;
    IScheduler &scheduler_;
    RegistrationPolicy policy_;
    JitterSource jitter_;
    EngineListener *listener_{nullptr};
    bool selfRefresh_{true};
    std::map<std::string, std::unique_ptr<RegistrationController>> accounts_;
};

}
