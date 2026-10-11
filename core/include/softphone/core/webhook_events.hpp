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

#include <softphone/core/call.hpp>
#include <softphone/core/settings.hpp>
#include <softphone/core/webhooks.hpp>

#include <chrono>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::core {

using WebhookEvent = std::string;

namespace webhookEvent {

inline constexpr auto kStarted = "call-started";

inline constexpr auto kEnded = "call-ended";

inline constexpr auto kStateChanged = "call-state-changed";

}

struct WebhookContext {

    std::string accountId;
    std::string accountAddress;

    std::string name;

    std::chrono::system_clock::time_point at{};

    std::chrono::seconds duration{0};
};

using WebhookFields = std::vector<std::pair<std::string, std::string>>;

[[nodiscard]] WebhookFields webhookFields(const CallInfo &call, WebhookEvent event,
                                          const WebhookContext &context);

[[nodiscard]] std::span<const std::string_view> webhookPlaceholders() noexcept;

[[nodiscard]] std::string webhookJson(const WebhookFields &fields);

[[nodiscard]] std::string webhookQuery(const WebhookFields &fields);

[[nodiscard]] std::string fillTemplate(std::string_view pattern, const WebhookFields &fields);

[[nodiscard]] std::vector<std::string> unknownPlaceholders(std::string_view pattern);

[[nodiscard]] std::string renamedPlaceholders(std::string_view pattern);

[[nodiscard]] std::optional<WebhookRequest> webhookFor(const WebhookSettings &settings,
                                                       const WebhookFields &fields,
                                                       WebhookEvent event,
                                                       const std::string &headerValue);

}
