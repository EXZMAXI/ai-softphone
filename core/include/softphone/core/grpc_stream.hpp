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
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::core {

class IGrpcStreamSink {
public:
    IGrpcStreamSink() = default;
    IGrpcStreamSink(const IGrpcStreamSink &) = delete;
    IGrpcStreamSink &operator=(const IGrpcStreamSink &) = delete;
    IGrpcStreamSink(IGrpcStreamSink &&) = delete;
    IGrpcStreamSink &operator=(IGrpcStreamSink &&) = delete;
    virtual ~IGrpcStreamSink() = default;

    virtual void opened() = 0;

    virtual void message(std::string_view payload) = 0;

    virtual void closed(int status, std::string_view said) = 0;
};

class IGrpcStream {
public:
    IGrpcStream() = default;
    IGrpcStream(const IGrpcStream &) = delete;
    IGrpcStream &operator=(const IGrpcStream &) = delete;
    IGrpcStream(IGrpcStream &&) = delete;
    IGrpcStream &operator=(IGrpcStream &&) = delete;

    virtual ~IGrpcStream() = default;

    [[nodiscard]] virtual bool send(std::string_view payload) = 0;

    virtual void finishSending() = 0;

    [[nodiscard]] virtual bool open() const = 0;
};

class IGrpcTransport {
public:
    IGrpcTransport() = default;
    IGrpcTransport(const IGrpcTransport &) = delete;
    IGrpcTransport &operator=(const IGrpcTransport &) = delete;
    IGrpcTransport(IGrpcTransport &&) = delete;
    IGrpcTransport &operator=(IGrpcTransport &&) = delete;
    virtual ~IGrpcTransport() = default;

    struct Call {

        std::string host;

        std::string method;

        std::vector<std::pair<std::string, std::string>> headers;

        std::chrono::seconds connectWithin{20};
    };

    [[nodiscard]] virtual std::unique_ptr<IGrpcStream> open(const Call &call,
                                                            IGrpcStreamSink &sink) = 0;

    [[nodiscard]] virtual std::string libraryVersion() const = 0;
};

}
