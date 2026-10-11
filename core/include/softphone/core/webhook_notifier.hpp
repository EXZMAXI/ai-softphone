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

#include <softphone/core/call_historian.hpp>
#include <softphone/core/scheduler.hpp>
#include <softphone/core/webhook_events.hpp>

#include <functional>
#include <map>
#include <string>

namespace softphone::core {

class WebhookNotifier {
public:
    using AddressLookup = CallHistorian::AddressLookup;
    using NameLookup = CallHistorian::NameLookup;

    using Sender = std::function<void(WebhookRequest)>;

    using SecretLookup = std::function<std::string()>;

    WebhookNotifier(IScheduler &scheduler, Sender send, WallClock clock = defaultWallClock());

    void setSettings(WebhookSettings settings);
    void setAddressLookup(AddressLookup lookup);
    void setNameLookup(NameLookup lookup);
    void setSecretLookup(SecretLookup lookup);

    void observe(const CallInfo &call);

    void sendTestEvent();

    [[nodiscard]] std::size_t watching() const noexcept
    {
        return seen_.size();
    }

private:
    struct Sighting {
        std::chrono::system_clock::time_point startedAt;

        CallInfo standing;
        bool ended{false};
    };

    void raise(const CallInfo &call, WebhookEvent event, std::chrono::system_clock::time_point at,
               std::chrono::seconds duration);

    void forgetTheOldest();

    IScheduler &scheduler_;
    Sender send_;
    WallClock clock_;

    WebhookSettings settings_;
    AddressLookup address_;
    NameLookup name_;
    SecretLookup secret_;

    static constexpr std::size_t kRememberedCalls = 256;
    std::map<std::string, Sighting> seen_;
};

}
