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

#include <softphone/core/backoff.hpp>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace softphone::core {

namespace secretKeys {

inline constexpr std::string_view kWebhookHeader{"integration:webhook"};
inline constexpr std::string_view kApiToken{"integration:api"};

}

enum class HttpMethod {
    Get,
    Post,
};

[[nodiscard]] std::string_view toString(HttpMethod method) noexcept;
[[nodiscard]] std::optional<HttpMethod> parseHttpMethod(std::string_view text) noexcept;

struct WebhookRequest {
    HttpMethod method{HttpMethod::Post};
    std::string url;

    std::vector<std::pair<std::string, std::string>> headers;

    std::string body;
    std::string contentType{"application/json"};
};

struct WebhookResult {

    bool delivered{false};

    int status{0};

    std::string error;

    [[nodiscard]] bool worthRetrying() const noexcept
    {
        if (delivered) {
            return false;
        }

        return status == 0 || status == 408 || status == 429 || status >= 500;
    }
};

class IWebhookTransport {
public:
    IWebhookTransport() = default;
    IWebhookTransport(const IWebhookTransport &) = delete;
    IWebhookTransport &operator=(const IWebhookTransport &) = delete;
    IWebhookTransport(IWebhookTransport &&) = delete;
    IWebhookTransport &operator=(IWebhookTransport &&) = delete;
    virtual ~IWebhookTransport() = default;

    virtual WebhookResult send(const WebhookRequest &request) = 0;
};

struct WebhookPolicy {

    std::size_t queueDepth{64};

    unsigned attempts{3};

    BackoffPolicy backoff{std::chrono::milliseconds{500}, std::chrono::seconds{10}, 2.0, 0.2};
};

class WebhookDispatcher {
public:
    WebhookDispatcher(IWebhookTransport &transport, WebhookPolicy policy = {},
                      JitterSource jitter = {});
    WebhookDispatcher(const WebhookDispatcher &) = delete;
    WebhookDispatcher &operator=(const WebhookDispatcher &) = delete;
    WebhookDispatcher(WebhookDispatcher &&) = delete;
    WebhookDispatcher &operator=(WebhookDispatcher &&) = delete;
    ~WebhookDispatcher();

    bool post(WebhookRequest request);

    [[nodiscard]] bool drain(std::chrono::milliseconds limit);

    struct Tally {
        std::size_t queued{0};
        std::size_t delivered{0};
        std::size_t failed{0};
        std::size_t dropped{0};
        std::size_t waiting{0};
    };

    [[nodiscard]] Tally tally() const;

private:
    void run();

    IWebhookTransport &transport_;
    WebhookPolicy policy_;
    JitterSource jitter_;

    mutable std::mutex mutex_;
    std::condition_variable work_;
    std::condition_variable idle_;
    std::deque<WebhookRequest> queue_;
    bool stopping_{false};
    bool busy_{false};
    Tally tally_;

    std::thread thread_;
};

}
