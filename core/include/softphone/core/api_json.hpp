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

#include <softphone/core/account_config.hpp>
#include <softphone/core/call.hpp>
#include <softphone/core/call_log.hpp>
#include <softphone/core/contacts.hpp>
#include <softphone/core/settings.hpp>
#include <softphone/core/taxonomy.hpp>
#include <softphone/core/webhook_events.hpp>

#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace softphone::core {

struct ApiAccountView {
    AccountConfig config;

    std::string state;
};

struct ApiCallView {
    CallInfo call;

    std::string accountId;
};

[[nodiscard]] std::string queryValue(std::string_view query, std::string_view name);

[[nodiscard]] std::string toJson(std::span<const Contact> contacts);
[[nodiscard]] std::string toJson(const Contact &contact);
[[nodiscard]] std::string toJson(std::span<const CallRecord> calls);

[[nodiscard]] std::string toJson(std::span<const ApiCallView> calls,
                                 std::chrono::system_clock::time_point now);
[[nodiscard]] std::string toJson(std::span<const ApiAccountView> accounts);

[[nodiscard]] std::string toJson(std::span<const TaxonomyEntry> entries);

[[nodiscard]] std::optional<Contact> contactFromJson(std::string_view body, std::string &problem);

[[nodiscard]] std::optional<AccountConfig>
accountFromJson(std::string_view body, const AccountConfig &current, std::string &problem);

[[nodiscard]] std::string stringFromJson(std::string_view body, std::string_view name);

}
