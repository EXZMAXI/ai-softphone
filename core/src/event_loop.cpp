// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/event_loop.hpp>

#include <algorithm>
#include <utility>

namespace softphone::core {

EventLoop::EventLoop() = default;

EventLoop::~EventLoop() = default;

EventLoop::Clock::time_point EventLoop::now() const
{
    return Clock::now();
}

IScheduler::TimerId EventLoop::scheduleAfter(std::chrono::milliseconds delay,
                                             std::function<void()> callback)
{
    const std::lock_guard lock{mutex_};

    const auto id = ++nextTimerId_;
    timers_.push_back(Timer{id, Clock::now() + delay, std::move(callback)});

    wakeup_.notify_all();
    return id;
}

void EventLoop::cancel(TimerId id)
{
    const std::lock_guard lock{mutex_};

    const auto it = std::find_if(timers_.begin(), timers_.end(),
                                 [id](const Timer &timer) { return timer.id == id; });
    if (it != timers_.end()) {
        timers_.erase(it);
    }
}

void EventLoop::post(std::function<void()> work)
{
    const std::lock_guard lock{mutex_};

    queue_.push_back(std::move(work));
    wakeup_.notify_all();
}

void EventLoop::stop()
{
    const std::lock_guard lock{mutex_};

    stopped_ = true;
    wakeup_.notify_all();
}

void EventLoop::restart()
{
    const std::lock_guard lock{mutex_};

    stopped_ = false;
}

bool EventLoop::stopped() const
{
    const std::lock_guard lock{mutex_};
    return stopped_;
}

std::vector<std::function<void()>> EventLoop::collectRunnable()
{
    std::vector<std::function<void()>> runnable;

    const std::lock_guard lock{mutex_};

    runnable.swap(queue_);

    const auto now = Clock::now();
    for (auto it = timers_.begin(); it != timers_.end();) {
        if (it->due <= now) {
            runnable.push_back(std::move(it->callback));
            it = timers_.erase(it);
        } else {
            ++it;
        }
    }

    return runnable;
}

void EventLoop::runPending()
{
    for (auto &work : collectRunnable()) {
        work();
    }
}

void EventLoop::runFor(std::chrono::milliseconds duration)
{
    {
        std::unique_lock lock{mutex_};

        if (!stopped_ && queue_.empty()) {
            auto wakeAt = Clock::now() + duration;

            const auto earliest =
                std::min_element(timers_.begin(), timers_.end(),
                                 [](const Timer &a, const Timer &b) { return a.due < b.due; });
            if (earliest != timers_.end() && earliest->due < wakeAt) {
                wakeAt = earliest->due;
            }

            wakeup_.wait_until(lock, wakeAt, [this]() { return stopped_ || !queue_.empty(); });
        }
    }

    runPending();
}

void EventLoop::run()
{
    while (true) {
        {
            std::unique_lock lock{mutex_};

            if (stopped_) {
                return;
            }

            if (queue_.empty()) {
                const auto earliest =
                    std::min_element(timers_.begin(), timers_.end(),
                                     [](const Timer &a, const Timer &b) { return a.due < b.due; });

                if (earliest == timers_.end()) {
                    wakeup_.wait(lock, [this]() { return stopped_ || !queue_.empty(); });
                } else {
                    wakeup_.wait_until(lock, earliest->due,
                                       [this]() { return stopped_ || !queue_.empty(); });
                }

                if (stopped_) {
                    return;
                }
            }
        }

        runPending();
    }
}

}
