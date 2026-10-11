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

#include <softphone/core/call.hpp>
#include <softphone/core/call_controller.hpp>
#include <softphone/core/scheduler.hpp>

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace softphone::core {

[[nodiscard]] bool worthRedialling(CallEndReason reason) noexcept;

struct RedialPolicy {

    std::chrono::milliseconds interval{std::chrono::seconds{15}};

    std::chrono::milliseconds giveUpAfter{std::chrono::minutes{30}};
};

enum class RedialOutcome {

    GotThrough,

    Cancelled,

    Refused,

    GaveUp,
};

[[nodiscard]] std::string_view toString(RedialOutcome outcome) noexcept;

class RedialController {
public:

    struct State {
        bool running{false};

        std::string accountId;

        std::string target;
        std::string dialed;

        int attempts{0};

        bool attemptRang{false};

        std::string callId;

        std::string continuesCallId;

        std::string lastAttemptId;

        std::optional<std::chrono::steady_clock::time_point> nextAttemptAt;

        bool waitingForTheLine{false};

        std::chrono::steady_clock::time_point giveUpAt{};

        std::optional<RedialOutcome> finished;
    };

    using StateCallback = std::function<void(const State &state)>;

    RedialController(CallController &calls, IScheduler &scheduler, RedialPolicy policy,
                     StateCallback onChanged);

    RedialController(const RedialController &) = delete;
    RedialController &operator=(const RedialController &) = delete;
    RedialController(RedialController &&) = delete;
    RedialController &operator=(RedialController &&) = delete;
    ~RedialController();

    void setPolicy(RedialPolicy policy);

    [[nodiscard]] const RedialPolicy &policy() const noexcept
    {
        return policy_;
    }

    void start(std::string accountId, std::string target, std::string dialed = {},
               std::string continues = {});

    void cancel();

    [[nodiscard]] const State &state() const noexcept
    {
        return state_;
    }

    void onCallChanged(const CallInfo &call);

private:
    void attempt();
    void armFor(std::chrono::milliseconds delay);
    void disarm();
    void finish(RedialOutcome outcome);
    void announce();
    [[nodiscard]] bool lineIsBusy() const;

    CallController &calls_;
    IScheduler &scheduler_;
    RedialPolicy policy_;
    StateCallback onChanged_;

    State state_;
    IScheduler::TimerId timer_{IScheduler::kNoTimer};
};

}
