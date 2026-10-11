// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/webhooks.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/metrics.hpp>

#include <string>
#include <utility>

namespace softphone::core {

std::string_view toString(HttpMethod method) noexcept
{
    switch (method) {
    case HttpMethod::Get:
        return "GET";
    case HttpMethod::Post:
        return "POST";
    }
    return "POST";
}

std::optional<HttpMethod> parseHttpMethod(std::string_view text) noexcept
{
    if (text == "GET" || text == "get") {
        return HttpMethod::Get;
    }
    if (text == "POST" || text == "post") {
        return HttpMethod::Post;
    }
    return std::nullopt;
}

WebhookDispatcher::WebhookDispatcher(IWebhookTransport &transport, WebhookPolicy policy,
                                     JitterSource jitter)
    : transport_{transport}, policy_{policy},
      jitter_{jitter ? std::move(jitter) : defaultJitterSource()}
{
    thread_ = std::thread{[this]() { run(); }};
}

WebhookDispatcher::~WebhookDispatcher()
{
    {
        const std::lock_guard lock{mutex_};
        stopping_ = true;
    }

    work_.notify_all();
    idle_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

bool WebhookDispatcher::post(WebhookRequest request)
{
    bool kept = true;
    {
        const std::lock_guard lock{mutex_};

        while (queue_.size() >= policy_.queueDepth) {
            queue_.pop_front();
            ++tally_.dropped;
            kept = false;
            metrics::webhooksDropped().add();
        }

        queue_.push_back(std::move(request));
        ++tally_.queued;
    }

    work_.notify_one();
    return kept;
}

bool WebhookDispatcher::drain(std::chrono::milliseconds limit)
{
    std::unique_lock lock{mutex_};
    return idle_.wait_for(lock, limit, [this]() { return queue_.empty() && !busy_; });
}

WebhookDispatcher::Tally WebhookDispatcher::tally() const
{
    const std::lock_guard lock{mutex_};
    auto tally = tally_;
    tally.waiting = queue_.size();
    return tally;
}

void WebhookDispatcher::run()
{
    std::unique_lock lock{mutex_};

    while (true) {
        work_.wait(lock, [this]() { return stopping_ || !queue_.empty(); });

        if (stopping_) {
            if (!queue_.empty()) {
                const auto abandoned = std::to_string(queue_.size());
                const LogAttribute fields[] = {{"events", abandoned}};
                Logger::instance().log(LogLevel::Info, "webhook",
                                       "events still queued when the phone closed were not sent",
                                       fields);
                queue_.clear();
            }
            return;
        }

        auto request = std::move(queue_.front());
        queue_.pop_front();
        busy_ = true;

        lock.unlock();

        WebhookResult result;
        for (unsigned attempt = 0; attempt < policy_.attempts; ++attempt) {
            result = transport_.send(request);
            if (result.delivered || !result.worthRetrying()) {
                break;
            }

            const auto delay = backoffDelay(policy_.backoff, attempt, jitter_());

            std::unique_lock retryLock{mutex_};
            if (idle_.wait_for(retryLock, delay, [this]() { return stopping_; })) {
                break;
            }
        }

        lock.lock();
        if (result.delivered) {
            ++tally_.delivered;
            metrics::webhooksDelivered().add();
        } else {
            ++tally_.failed;
            metrics::webhooksFailed().add();

            Logger::instance().log(
                LogLevel::Warning, "webhook",
                "not delivered to " + request.url + ": " +
                    (result.error.empty() ? std::to_string(result.status) : result.error));
        }

        busy_ = false;
        idle_.notify_all();
    }
}

}
