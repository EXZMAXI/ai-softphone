// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/ix_web_socket.hpp>

#include "machine_trust.hpp"

#include <softphone/core/http.hpp>
#include <softphone/core/logging.hpp>

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketCloseConstants.h>
#include <ixwebsocket/IXWebSocketVersion.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

namespace softphone::http {
namespace {

void startTheNetwork()
{
    static const bool started = ::ix::initNetSystem();
    (void)started;
}

[[nodiscard]] core::WebSocketStatus readTheRefusal(const ::ix::WebSocketErrorInfo &why)
{
    if (why.http_status == 401 || why.http_status == 403) {
        return core::WebSocketStatus::Unauthorised;
    }
    if (why.http_status >= 400) {
        return core::WebSocketStatus::Refused;
    }

    const auto &said = why.reason;
    const auto mentions = [&said](std::string_view word) {
        return said.find(word) != std::string::npos;
    };

    if (mentions("OpenSSL") || mentions("certificate") || mentions("SSL")) {
        return core::WebSocketStatus::Untrusted;
    }
    if (mentions("Unable to connect") || mentions("Could not resolve") ||
        mentions("no address found")) {
        return core::WebSocketStatus::Unreachable;
    }
    if (mentions("imeout") || mentions("imed out")) {
        return core::WebSocketStatus::Timeout;
    }
    return core::WebSocketStatus::Failed;
}

class IxWebSocket final : public core::IWebSocket {
public:
    IxWebSocket(const core::WebSocketEndpoint &where, core::IWebSocketSink &sink)
        : where_{where}, sink_{sink}
    {
        startTheNetwork();

        socket_.setUrl(where_.url);
        socket_.setHandshakeTimeout(static_cast<int>(where_.handshakeTimeout.count()));

        socket_.enablePong();
        if (where_.pingInterval > std::chrono::seconds::zero()) {
            socket_.setPingInterval(static_cast<int>(where_.pingInterval.count()));
        }

        socket_.disablePerMessageDeflate();
        socket_.disableAutomaticReconnection();

        if (!where_.protocol.empty()) {
            socket_.addSubProtocol(where_.protocol);
        }

        ::ix::WebSocketHttpHeaders headers;
        for (const auto &[name, value] : where_.headers) {

            headers[core::headerSafeName(name)] = core::headerSafeValue(value);
        }
        socket_.setExtraHeaders(headers);

        ::ix::SocketTLSOptions tls;
        tls.caFile = trustStoreForWebSockets();
        socket_.setTLSOptions(tls);

        socket_.setOnMessageCallback(
            [this](const ::ix::WebSocketMessagePtr &message) { heard(message); });

        supervisor_ = std::thread{[this] { supervise(); }};
    }

    ~IxWebSocket() override
    {
        close(std::chrono::milliseconds{500});
        if (supervisor_.joinable()) {
            supervisor_.join();
        }
    }

    [[nodiscard]] bool send(std::span<const std::byte> payload) override
    {
        if (!open()) {
            return false;
        }
        const ::ix::IXWebSocketSendData data{reinterpret_cast<const char *>(payload.data()),
                                             payload.size()};
        return socket_.sendBinary(data).success;
    }

    [[nodiscard]] bool sendText(std::string_view payload) override
    {
        if (!open()) {
            return false;
        }
        return socket_.sendUtf8Text(std::string{payload}).success;
    }

    [[nodiscard]] bool open() const override
    {
        return socket_.getReadyState() == ::ix::ReadyState::Open;
    }

    [[nodiscard]] std::size_t waiting() const override
    {
        return socket_.bufferedAmount();
    }

    void close(std::chrono::milliseconds within) override
    {
        {
            const std::lock_guard<std::mutex> held{lock_};
            if (leaving_) {
                return;
            }
            leaving_ = true;
        }

        awake_.notify_all();

        socket_.close(::ix::WebSocketCloseConstants::kNormalClosureCode, "going");

        if (within == std::chrono::milliseconds::zero()) {
            return;
        }

        std::unique_lock<std::mutex> held{lock_};
        if (!awake_.wait_for(held, within, [this] { return finished_; })) {

            core::Logger::instance().log(core::LogLevel::Warning, "websocket",
                                         "the socket to " + where_.redacted() +
                                             " did not close inside its deadline");
        }
    }

private:

    void heard(const ::ix::WebSocketMessagePtr &message)
    {
        if (!message) {
            return;
        }

        switch (message->type) {
        case ::ix::WebSocketMessageType::Open:
            attempt_ = 0;
            sink_.opened();
            return;

        case ::ix::WebSocketMessageType::Message:
            if (message->binary) {
                sink_.binary({reinterpret_cast<const std::byte *>(message->str.data()),
                              message->str.size()});
            } else {
                sink_.text(message->str);
            }
            return;

        case ::ix::WebSocketMessageType::Close:

            ended(message->closeInfo.remote ? core::WebSocketStatus::Dropped
                                            : core::WebSocketStatus::Ok,
                  message->closeInfo.reason);
            return;

        case ::ix::WebSocketMessageType::Error:
            ended(readTheRefusal(message->errorInfo), message->errorInfo.reason);
            return;

        case ::ix::WebSocketMessageType::Ping:
        case ::ix::WebSocketMessageType::Pong:
        case ::ix::WebSocketMessageType::Fragment:
            return;
        }
    }

    void ended(core::WebSocketStatus status, const std::string &why)
    {
        {
            const std::lock_guard<std::mutex> held{lock_};
            status_ = status;
            why_ = why;
            down_ = true;
        }
        awake_.notify_all();
    }

    void supervise()
    {
        socket_.start();

        for (;;) {
            core::WebSocketStatus status{};
            std::string why;
            {
                std::unique_lock<std::mutex> held{lock_};
                awake_.wait(held, [this] { return down_ || leaving_; });
                if (leaving_) {
                    break;
                }
                down_ = false;
                status = status_;
                why = why_;
            }

            if (!core::worthReconnecting(status) || attempt_ >= where_.reconnectAttempts) {
                tellTheSink(status, why);
                return;
            }

            ++attempt_;
            const auto after = core::reconnectWait(where_, attempt_);
            sink_.reconnecting(attempt_, after);

            {
                std::unique_lock<std::mutex> held{lock_};
                if (awake_.wait_for(held, after, [this] { return leaving_; })) {
                    break;
                }
            }

            socket_.stop();
            socket_.start();
        }

        socket_.stop();
        tellTheSink(core::WebSocketStatus::Cancelled, "closed from here");
    }

    void tellTheSink(core::WebSocketStatus status, const std::string &why)
    {
        bool first = false;
        {
            const std::lock_guard<std::mutex> held{lock_};
            first = !finished_;
            finished_ = true;
        }
        if (first) {
            sink_.closed(status, why);
        }
        awake_.notify_all();
    }

    core::WebSocketEndpoint where_;
    core::IWebSocketSink &sink_;

    ::ix::WebSocket socket_;

    std::thread supervisor_;

    mutable std::mutex lock_;
    std::condition_variable awake_;

    bool down_{false};
    bool leaving_{false};
    bool finished_{false};
    core::WebSocketStatus status_{core::WebSocketStatus::Failed};
    std::string why_;

    std::atomic<unsigned> attempt_{0};
};

class RefusedWebSocket final : public core::IWebSocket {
public:
    RefusedWebSocket(core::IWebSocketSink &sink, core::WebSocketStatus status, std::string why)
        : sink_{sink}, status_{status}, why_{std::move(why)}
    {}

    ~RefusedWebSocket() override
    {
        close(std::chrono::milliseconds{500});
    }

    [[nodiscard]] bool send(std::span<const std::byte>) override
    {
        return false;
    }

    [[nodiscard]] bool sendText(std::string_view) override
    {
        return false;
    }

    [[nodiscard]] bool open() const override
    {
        return false;
    }

    [[nodiscard]] std::size_t waiting() const override
    {
        return 0;
    }

    void close(std::chrono::milliseconds) override
    {
        if (!said_) {
            said_ = true;
            sink_.closed(status_, why_);
        }
    }

private:
    core::IWebSocketSink &sink_;
    core::WebSocketStatus status_;
    std::string why_;
    bool said_{false};
};

}

IxWebSocketFactory::IxWebSocketFactory() = default;
IxWebSocketFactory::~IxWebSocketFactory() = default;

std::unique_ptr<core::IWebSocket> IxWebSocketFactory::connect(const core::WebSocketEndpoint &where,
                                                              core::IWebSocketSink &sink)
{

    if (!where.proxy.empty()) {
        return std::make_unique<RefusedWebSocket>(
            sink, core::WebSocketStatus::Refused,
            "realtime recognition cannot go through an HTTP proxy yet");
    }

    if (where.url.empty()) {

        return std::make_unique<RefusedWebSocket>(sink, core::WebSocketStatus::Refused,
                                                  "this recogniser has no address");
    }

    const auto scheme = where.url.substr(0, where.url.find(':'));
    if (scheme != "ws" && scheme != "wss") {
        return std::make_unique<RefusedWebSocket>(sink, core::WebSocketStatus::Refused,
                                                  "the address is not a WebSocket address");
    }

    return std::make_unique<IxWebSocket>(where, sink);
}

std::string webSocketLibraryVersion()
{
    return std::string{"IXWebSocket "} + IX_WEBSOCKET_VERSION;
}

}
