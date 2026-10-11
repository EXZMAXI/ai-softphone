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

#include <string>
#include <string_view>
#include <vector>

namespace softphone::plugins::integration {

class IOutboundEvents {
public:
    IOutboundEvents() = default;
    IOutboundEvents(const IOutboundEvents &) = delete;
    IOutboundEvents &operator=(const IOutboundEvents &) = delete;
    IOutboundEvents(IOutboundEvents &&) = delete;
    IOutboundEvents &operator=(IOutboundEvents &&) = delete;
    virtual ~IOutboundEvents() = default;

    virtual void declare(std::string event) = 0;

    [[nodiscard]] virtual std::vector<std::string> declared() const = 0;

    [[nodiscard]] virtual bool wanted(std::string_view event) const = 0;

    virtual void post(core::WebhookRequest request) = 0;
};

inline constexpr auto kOutboundEvents = "integration.events";

inline constexpr auto kRouteRegistry = "integration.routes";

}
