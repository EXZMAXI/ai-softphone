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

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::core {

enum class WebSocketStatus {

    Ok,

    Unauthorised,

    Unreachable,

    Refused,

    Untrusted,

    Timeout,

    Cancelled,

    Dropped,
    Failed,
};

[[nodiscard]] std::string_view toString(WebSocketStatus status) noexcept;

[[nodiscard]] bool worthReconnecting(WebSocketStatus status) noexcept;

struct WebSocketEndpoint {

    std::string url;

    std::vector<std::pair<std::string, std::string>> headers;

    std::string protocol;

    std::string secret;

    std::chrono::seconds handshakeTimeout{10};

    std::chrono::seconds pingInterval{15};

    std::chrono::seconds pingTimeout{30};

    std::string proxy;

    unsigned reconnectAttempts{4};

    std::chrono::milliseconds reconnectFirstWait{250};
    std::chrono::milliseconds reconnectCeiling{4000};

    [[nodiscard]] std::string redacted() const;
};

class IWebSocketSink {
public:
    IWebSocketSink() = default;
    IWebSocketSink(const IWebSocketSink &) = delete;
    IWebSocketSink &operator=(const IWebSocketSink &) = delete;
    IWebSocketSink(IWebSocketSink &&) = delete;
    IWebSocketSink &operator=(IWebSocketSink &&) = delete;
    virtual ~IWebSocketSink() = default;

    virtual void opened() = 0;

    virtual void text(std::string_view payload) = 0;

    virtual void binary(std::span<const std::byte> payload) = 0;

    virtual void reconnecting(unsigned attempt, std::chrono::milliseconds after) = 0;

    virtual void closed(WebSocketStatus status, std::string_view why) = 0;
};

class IWebSocket {
public:
    IWebSocket() = default;
    IWebSocket(const IWebSocket &) = delete;
    IWebSocket &operator=(const IWebSocket &) = delete;
    IWebSocket(IWebSocket &&) = delete;
    IWebSocket &operator=(IWebSocket &&) = delete;

    virtual ~IWebSocket() = default;

    [[nodiscard]] virtual bool send(std::span<const std::byte> payload) = 0;

    [[nodiscard]] virtual bool sendText(std::string_view payload) = 0;

    [[nodiscard]] virtual bool open() const = 0;

    [[nodiscard]] virtual std::size_t waiting() const = 0;

    virtual void close(std::chrono::milliseconds within = std::chrono::milliseconds{500}) = 0;
};

class IWebSocketFactory {
public:
    IWebSocketFactory() = default;
    IWebSocketFactory(const IWebSocketFactory &) = delete;
    IWebSocketFactory &operator=(const IWebSocketFactory &) = delete;
    IWebSocketFactory(IWebSocketFactory &&) = delete;
    IWebSocketFactory &operator=(IWebSocketFactory &&) = delete;
    virtual ~IWebSocketFactory() = default;

    [[nodiscard]] virtual std::unique_ptr<IWebSocket> connect(const WebSocketEndpoint &where,
                                                              IWebSocketSink &sink) = 0;
};

[[nodiscard]] std::chrono::milliseconds reconnectWait(const WebSocketEndpoint &where,
                                                      unsigned attempt) noexcept;

}
