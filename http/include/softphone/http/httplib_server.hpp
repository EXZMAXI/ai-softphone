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

#include <softphone/core/api.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace softphone::http {

class HttplibServer {
public:

    using Answer = std::function<core::ApiResponse(const core::ApiRequest &)>;

    explicit HttplibServer(Answer answer);
    HttplibServer(const HttplibServer &) = delete;
    HttplibServer &operator=(const HttplibServer &) = delete;
    HttplibServer(HttplibServer &&) = delete;
    HttplibServer &operator=(HttplibServer &&) = delete;
    ~HttplibServer();

    [[nodiscard]] bool start(std::uint16_t port);

    void stop();

    [[nodiscard]] bool running() const noexcept;

    [[nodiscard]] std::uint16_t port() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
