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

#include <softphone/core/streaming_asr.hpp>
#include <softphone/core/web_socket.hpp>

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::http {

class StreamingHeard {
public:
    std::vector<core::AsrPartial> partials;
    std::vector<core::AsrTurn> turns;

    std::optional<std::int64_t> costMicros;

    std::optional<core::AsrStatus> fatal;
    std::string said;

    bool ended{false};

    bool understood{false};

    std::string shape;

    bool ready{false};

    bool remark{false};
};

class IStreamingProtocol {
public:
    IStreamingProtocol() = default;
    IStreamingProtocol(const IStreamingProtocol &) = delete;
    IStreamingProtocol &operator=(const IStreamingProtocol &) = delete;
    IStreamingProtocol(IStreamingProtocol &&) = delete;
    IStreamingProtocol &operator=(IStreamingProtocol &&) = delete;
    virtual ~IStreamingProtocol() = default;

    [[nodiscard]] virtual std::string_view type() const noexcept = 0;

    [[nodiscard]] virtual core::StreamingCapabilities
    capabilities(const core::AsrEndpoint &where) const = 0;

    [[nodiscard]] virtual std::string url(const core::AsrEndpoint &where,
                                          const core::StreamingOptions &options) const = 0;

    [[nodiscard]] virtual std::vector<std::pair<std::string, std::string>>
    headers(const core::AsrEndpoint &where) const
    {
        (void)where;
        return {};
    }

    [[nodiscard]] virtual std::vector<std::string> opening(const core::AsrEndpoint &where,
                                                           const core::StreamingOptions &) const
    {
        (void)where;
        return {};
    }

    [[nodiscard]] virtual bool closingIsBinary() const noexcept
    {
        return false;
    }

    [[nodiscard]] virtual std::vector<std::string> closing(std::size_t framesSent) const
    {
        (void)framesSent;
        return {};
    }

    [[nodiscard]] virtual std::vector<std::string> endOfTurn()
    {
        return {};
    }

    [[nodiscard]] virtual bool audioIsBinary() const noexcept
    {
        return true;
    }

    [[nodiscard]] virtual bool waitsToBeTold() const noexcept
    {
        return false;
    }

    [[nodiscard]] virtual std::string wrapAudio(std::span<const std::byte> audio) const
    {
        (void)audio;
        return {};
    }

    virtual void read(std::string_view payload, StreamingHeard &heard) = 0;

    virtual void readBinary(std::span<const std::byte> payload, StreamingHeard &heard)
    {
        (void)payload;
        (void)heard;
    }
};

class StreamingSession final : public core::IStreamingAsrSession, private core::IWebSocketSink {
public:
    StreamingSession(core::IWebSocketFactory &sockets, std::unique_ptr<IStreamingProtocol> protocol,
                     const core::AsrEndpoint &where, core::StreamingOptions options,
                     core::IStreamingAsrSink &sink, core::AsrCancel cancelled);

    ~StreamingSession() override;

    [[nodiscard]] bool send(std::chrono::milliseconds position,
                            std::span<const std::int16_t> samples) override;

    void turnEnded() override;
    void drain() override;

    [[nodiscard]] bool listening() const override;

private:

    void opened() override;
    void text(std::string_view payload) override;
    void binary(std::span<const std::byte> payload) override;
    void reconnecting(unsigned attempt, std::chrono::milliseconds after) override;
    void closed(core::WebSocketStatus status, std::string_view why) override;

    void deliver(StreamingHeard &heard);
    void finish(core::AsrStatus status, std::string why);

    std::unique_ptr<IStreamingProtocol> protocol_;
    core::AsrEndpoint where_;
    core::StreamingOptions options_;
    core::IStreamingAsrSink &sink_;
    core::AsrCancel cancelled_;

    std::unique_ptr<core::IWebSocket> socket_;

    mutable std::mutex lock_;

    bool up_{false};

    bool told_{false};

    bool saidWaiting_{false};
    bool over_{false};

    std::vector<core::AsrTurn> turns_;

    std::string detected_;
    std::int64_t costMicros_{0};
    std::chrono::milliseconds audio_{0};

    std::chrono::milliseconds furthest_{0};

    std::chrono::milliseconds heardFrom_{0};

    std::vector<std::byte> wire_;
    std::string wrapped_;

    std::size_t framesSent_{0};

    std::vector<std::string> unread_;

    std::vector<std::string> remarked_;

    std::vector<std::pair<std::string, std::size_t>> shapesSeen_;
};

class ProtocolRecogniser final : public core::IStreamingAsr {
public:
    using Make = std::unique_ptr<IStreamingProtocol> (*)();

    ProtocolRecogniser(core::IWebSocketFactory &sockets, Make make);

    [[nodiscard]] std::string_view type() const noexcept override;

    [[nodiscard]] core::StreamingCapabilities
    capabilities(const core::AsrEndpoint &where) const override;

    [[nodiscard]] std::unique_ptr<core::IStreamingAsrSession>
    open(const core::AsrEndpoint &where, const core::StreamingOptions &options,
         core::IStreamingAsrSink &sink, const core::AsrCancel &cancelled) override;

    [[nodiscard]] core::AsrProbe probe(const core::AsrEndpoint &where) override;

private:
    core::IWebSocketFactory &sockets_;
    Make make_;
    std::unique_ptr<IStreamingProtocol> asked_;
};

}
