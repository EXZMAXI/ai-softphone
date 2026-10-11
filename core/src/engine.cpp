// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/engine.hpp>

#include <softphone/core/logging.hpp>

#include <utility>

namespace softphone::core {

Engine::Engine(ISipBackend &backend, IScheduler &scheduler, RegistrationPolicy policy,
               JitterSource jitter)
    : backend_{backend}, scheduler_{scheduler}, policy_{policy}, jitter_{std::move(jitter)}
{
    backend_.setObserver(this);
}

Engine::~Engine()
{

    for (const auto &[accountId, controller] : accounts_) {
        backend_.forgetAccount(accountId);
    }

    accounts_.clear();
    backend_.setObserver(nullptr);
}

void Engine::setListener(EngineListener *listener) noexcept
{
    listener_ = listener;
}

ConfigProblem Engine::addAccount(AccountConfig config)
{
    if (const auto problem = validate(config); problem != ConfigProblem::None) {
        return problem;
    }

    auto accountId = config.id;

    auto controller = std::make_unique<RegistrationController>(
        std::move(config), backend_, scheduler_, policy_, jitter_,
        [this](const std::string &id, RegistrationState state, const RegistrationError &error) {
            if (listener_ != nullptr) {
                listener_->onRegistrationStateChanged(id, state, error);
            }
        });

    controller->setSelfRefresh(selfRefresh_);

    const LogAttribute fields[] = {{"line", accountId}};
    Logger::instance().log(LogLevel::Info, "registration", "line added", fields);

    accounts_[std::move(accountId)] = std::move(controller);
    return ConfigProblem::None;
}

void Engine::setSelfRefresh(bool enabled)
{
    selfRefresh_ = enabled;
    for (auto &[id, controller] : accounts_) {
        controller->setSelfRefresh(enabled);
    }
}

bool Engine::removeAccount(const std::string &accountId)
{
    const auto it = accounts_.find(accountId);
    if (it == accounts_.end()) {
        return false;
    }

    it->second->stop();
    accounts_.erase(it);
    backend_.forgetAccount(accountId);

    const LogAttribute fields[] = {{"line", accountId}};
    Logger::instance().log(LogLevel::Info, "registration", "line removed", fields);
    return true;
}

bool Engine::hasAccount(const std::string &accountId) const
{
    return accounts_.contains(accountId);
}

std::vector<std::string> Engine::accountIds() const
{
    std::vector<std::string> ids;
    ids.reserve(accounts_.size());
    for (const auto &[id, controller] : accounts_) {
        ids.push_back(id);
    }
    return ids;
}

bool Engine::startRegistration(const std::string &accountId)
{
    auto *controller = find(accountId);
    if (controller == nullptr) {
        return false;
    }
    controller->start();
    return true;
}

bool Engine::stopRegistration(const std::string &accountId)
{
    auto *controller = find(accountId);
    if (controller == nullptr) {
        return false;
    }
    controller->stop();
    return true;
}

void Engine::stopAll()
{
    for (auto &[id, controller] : accounts_) {
        controller->stop();
    }
}

RegistrationState Engine::registrationState(const std::string &accountId) const
{
    const auto *controller = find(accountId);
    return controller == nullptr ? RegistrationState::Offline : controller->state();
}

RegistrationError Engine::lastError(const std::string &accountId) const
{
    const auto *controller = find(accountId);
    return controller == nullptr ? RegistrationError{} : controller->lastError();
}

bool Engine::retryPending(const std::string &accountId) const
{
    const auto *controller = find(accountId);
    return controller != nullptr && controller->retryPending();
}

void Engine::setServerListener(ServerSeen told)
{
    serverSeen_ = std::move(told);
}

void Engine::onRegistrationOutcome(const std::string &accountId, const RegistrationOutcome &outcome)
{

    if (serverSeen_ && outcome.registered && !outcome.server.empty()) {
        serverSeen_(accountId, outcome.server);
    }

    if (auto *controller = find(accountId); controller != nullptr) {
        controller->onOutcome(outcome);
    }
}

RegistrationController *Engine::find(const std::string &accountId)
{
    const auto it = accounts_.find(accountId);
    return it == accounts_.end() ? nullptr : it->second.get();
}

const RegistrationController *Engine::find(const std::string &accountId) const
{
    const auto it = accounts_.find(accountId);
    return it == accounts_.end() ? nullptr : it->second.get();
}

}
