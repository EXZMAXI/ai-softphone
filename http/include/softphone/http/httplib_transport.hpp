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

#include <softphone/core/webhooks.hpp>

#include <chrono>
#include <memory>

namespace softphone::http {

class HttplibTransport final : public core::IWebhookTransport {
public:

    static constexpr auto kDefaultTimeout = std::chrono::seconds{5};

    explicit HttplibTransport(std::chrono::seconds timeout = kDefaultTimeout);
    ~HttplibTransport() override;

    core::WebhookResult send(const core::WebhookRequest &request) override;

    [[nodiscard]] static bool supportsTls() noexcept;

private:
    std::chrono::seconds timeout_;
};

}
