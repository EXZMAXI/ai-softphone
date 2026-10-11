// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/container/bus.hpp>

#include <algorithm>
#include <utility>

namespace softphone::container {

Executor inlineExecutor()
{
    return [](std::function<void()> work) { work(); };
}

EventBus::Token EventBus::add(std::string_view name, std::type_index type, Executor executor,
                              std::function<void(const void *)> deliver,
                              std::function<std::shared_ptr<void>(const void *)> copy)
{
    auto subscription = std::make_shared<Subscription>(name, type, std::move(executor),
                                                       std::move(deliver), std::move(copy));

    const std::lock_guard lock{mutex_};
    subscription->token = nextToken_++;
    const auto token = subscription->token;
    subscriptions_.push_back(std::move(subscription));
    return token;
}

void EventBus::unsubscribe(Token token)
{
    if (token == kNoToken) {
        return;
    }

    const std::lock_guard lock{mutex_};
    for (auto &subscription : subscriptions_) {
        if (subscription->token == token) {
            subscription->alive.store(false);
        }
    }
    std::erase_if(subscriptions_,
                  [token](const auto &subscription) { return subscription->token == token; });
}

EventBus::Token EventBus::watch(Watcher watcher)
{
    if (!watcher) {
        return kNoToken;
    }

    const std::lock_guard lock{mutex_};
    const auto token = nextToken_++;
    watchers_.emplace_back(token, std::move(watcher));
    return token;
}

void EventBus::unwatch(Token token)
{
    const std::lock_guard lock{mutex_};
    std::erase_if(watchers_, [token](const auto &entry) { return entry.first == token; });
}

void EventBus::dispatch(Trace trace, std::type_index type, const void *payload)
{
    trace.correlation = correlation_.fetch_add(1) + 1;

    std::vector<std::shared_ptr<Subscription>> listening;
    std::vector<Watcher> watching;
    {
        const std::lock_guard lock{mutex_};
        for (const auto &subscription : subscriptions_) {
            if (subscription->type == type && subscription->name == trace.name) {
                listening.push_back(subscription);
            }
        }
        watching.reserve(watchers_.size());
        for (const auto &entry : watchers_) {
            watching.push_back(entry.second);
        }
    }

    trace.subscribers = listening.size();
    for (const auto &watcher : watching) {
        watcher(trace);
    }

    for (const auto &subscription : listening) {
        if (!subscription->alive.load()) {
            continue;
        }

        auto kept = subscription->copy(payload);
        subscription->executor([subscription, kept = std::move(kept)]() {
            if (!subscription->alive.load()) {
                return;
            }
            subscription->deliver(kept.get());
        });
    }
}

std::size_t EventBus::countFor(std::string_view name, std::type_index type) const
{
    const std::lock_guard lock{mutex_};
    return static_cast<std::size_t>(
        std::ranges::count_if(subscriptions_, [name, type](const auto &subscription) {
            return subscription->type == type && subscription->name == name;
        }));
}

}
