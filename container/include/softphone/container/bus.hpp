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

#include <atomic>
#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <vector>

namespace softphone::container {

using Executor = std::function<void(std::function<void()>)>;

[[nodiscard]] Executor inlineExecutor();

template<typename T>
concept Event = std::copy_constructible<T> && requires {
    { T::kName } -> std::convertible_to<std::string_view>;
    { T::kVersion } -> std::convertible_to<int>;
};

struct Trace {
    std::string_view name;
    int version{0};

    std::string_view publisher;

    std::uint64_t correlation{0};

    std::size_t subscribers{0};
};

class EventBus {
public:
    using Token = std::uint64_t;
    static constexpr Token kNoToken = 0;

    EventBus() = default;
    EventBus(const EventBus &) = delete;
    EventBus &operator=(const EventBus &) = delete;
    EventBus(EventBus &&) = delete;
    EventBus &operator=(EventBus &&) = delete;
    ~EventBus() = default;

    template<Event T>
    [[nodiscard]] Token subscribe(Executor executor, std::function<void(const T &)> callback)
    {
        if (!executor || !callback) {
            return kNoToken;
        }

        auto deliver = [callback = std::move(callback)](const void *payload) {
            callback(*static_cast<const T *>(payload));
        };
        auto copy = [](const void *payload) -> std::shared_ptr<void> {
            return std::make_shared<T>(*static_cast<const T *>(payload));
        };
        return add(T::kName, std::type_index(typeid(T)), std::move(executor), std::move(deliver),
                   std::move(copy));
    }

    template<Event T>
    void publish(const T &payload, std::string_view publisher = "container")
    {
        dispatch(Trace{T::kName, T::kVersion, publisher, 0, 0}, std::type_index(typeid(T)),
                 &payload);
    }

    void unsubscribe(Token token);

    using Watcher = std::function<void(const Trace &)>;
    [[nodiscard]] Token watch(Watcher watcher);
    void unwatch(Token token);

    template<Event T>
    [[nodiscard]] std::size_t subscribers() const
    {
        return countFor(T::kName, std::type_index(typeid(T)));
    }

private:
    struct Subscription {

        Subscription(std::string_view name_, std::type_index type_, Executor executor_,
                     std::function<void(const void *)> deliver_,
                     std::function<std::shared_ptr<void>(const void *)> copy_)
            : name{name_}, type{type_}, executor{std::move(executor_)},
              deliver{std::move(deliver_)}, copy{std::move(copy_)}
        {}

        Token token{kNoToken};
        std::string_view name;
        std::type_index type;
        Executor executor;
        std::function<void(const void *)> deliver;
        std::function<std::shared_ptr<void>(const void *)> copy;

        std::atomic<bool> alive{true};
    };

    [[nodiscard]] Token add(std::string_view name, std::type_index type, Executor executor,
                            std::function<void(const void *)> deliver,
                            std::function<std::shared_ptr<void>(const void *)> copy);
    void dispatch(Trace trace, std::type_index type, const void *payload);
    [[nodiscard]] std::size_t countFor(std::string_view name, std::type_index type) const;

    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<Subscription>> subscriptions_;
    std::vector<std::pair<Token, Watcher>> watchers_;
    Token nextToken_{1};
    std::atomic<std::uint64_t> correlation_{0};
};

}
