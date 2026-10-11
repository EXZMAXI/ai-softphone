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

#include <softphone/core/scheduler.hpp>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

namespace softphone::core {

class EventLoop final : public IScheduler {
public:
    using Clock = std::chrono::steady_clock;

    EventLoop();
    EventLoop(const EventLoop &) = delete;
    EventLoop &operator=(const EventLoop &) = delete;
    EventLoop(EventLoop &&) = delete;
    EventLoop &operator=(EventLoop &&) = delete;
    ~EventLoop() override;

    [[nodiscard]] Clock::time_point now() const override;

    [[nodiscard]] TimerId scheduleAfter(std::chrono::milliseconds delay,
                                        std::function<void()> callback) override;
    void cancel(TimerId id) override;

    void post(std::function<void()> work);

    void run();

    void runPending();

    void runFor(std::chrono::milliseconds duration);

    void stop();

    void restart();

    [[nodiscard]] bool stopped() const;

private:
    struct Timer {
        TimerId id{};
        Clock::time_point due;
        std::function<void()> callback;
    };

    [[nodiscard]] std::vector<std::function<void()>> collectRunnable();

    mutable std::mutex mutex_;
    std::condition_variable wakeup_;
    std::vector<std::function<void()>> queue_;
    std::vector<Timer> timers_;
    TimerId nextTimerId_{0};
    bool stopped_{false};
};

}
