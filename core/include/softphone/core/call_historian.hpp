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

#include <softphone/core/call_log.hpp>
#include <softphone/core/scheduler.hpp>

#include <cstddef>
#include <functional>
#include <map>
#include <string>

namespace softphone::core {

class CallHistorian {
public:

    struct AccountIdentity {
        std::string uuid;
        std::string address;
    };

    using AddressLookup = std::function<AccountIdentity(const std::string &accountId)>;
    using NameLookup = std::function<std::string(const std::string &number)>;

    CallHistorian(ICallLog &log, IScheduler &scheduler, WallClock clock = defaultWallClock());

    void setAddressLookup(AddressLookup lookup);
    void setNameLookup(NameLookup lookup);

    void observe(const CallInfo &call);

    using Recorded = std::function<void(const CallRecord &)>;
    void setOnRecorded(Recorded told);

    [[nodiscard]] std::size_t watching() const noexcept
    {
        return seen_.size();
    }

    [[nodiscard]] StoreStatus lastStatus() const noexcept
    {
        return lastStatus_;
    }

private:
    struct Sighting {
        std::chrono::system_clock::time_point startedAt;

        bool written{false};

        std::string recordId;
    };

    void forgetTheOldest();

    struct Campaign {
        std::string recordId;
        std::chrono::system_clock::time_point startedAt{};
        int attempt{0};

        bool grewOutOfACall{false};
    };

    Campaign campaign_;

    ICallLog &log_;
    IScheduler &scheduler_;
    WallClock clock_;
    AddressLookup address_;
    NameLookup name_;
    Recorded recorded_;
    std::map<std::string, Sighting> seen_;
    StoreStatus lastStatus_{StoreStatus::Ok};
};

inline constexpr std::size_t kRememberedCalls = 64;

}
