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
#include <softphone/core/registration.hpp>

#include <string>

namespace softphone::core {

class ISipBackend {
public:
    class Observer {
    public:
        Observer() = default;
        Observer(const Observer &) = delete;
        Observer &operator=(const Observer &) = delete;
        Observer(Observer &&) = delete;
        Observer &operator=(Observer &&) = delete;
        virtual ~Observer() = default;

        virtual void onRegistrationOutcome(const std::string &accountId,
                                           const RegistrationOutcome &outcome) = 0;
    };

    ISipBackend() = default;
    ISipBackend(const ISipBackend &) = delete;
    ISipBackend &operator=(const ISipBackend &) = delete;
    ISipBackend(ISipBackend &&) = delete;
    ISipBackend &operator=(ISipBackend &&) = delete;
    virtual ~ISipBackend() = default;

    virtual void setObserver(Observer *observer) = 0;

    virtual void requestRegistration(const AccountConfig &config) = 0;
    virtual void requestUnregistration(const std::string &accountId) = 0;

    virtual void forgetAccount(const std::string &accountId) = 0;
};

}
