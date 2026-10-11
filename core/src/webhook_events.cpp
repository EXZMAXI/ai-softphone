// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/webhook_events.hpp>

#include <softphone/core/time_text.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <ctime>

namespace softphone::core {
namespace {

constexpr std::array kPlaceholders{
    std::string_view{"event"},        std::string_view{"id"},
    std::string_view{"direction"},    std::string_view{"state"},
    std::string_view{"number"},       std::string_view{"name"},
    std::string_view{"uri"},          std::string_view{"dialed"},
    std::string_view{"account"},      std::string_view{"account_id"},
    std::string_view{"event_ts"},     std::string_view{"duration_s"},
    std::string_view{"reason"},       std::string_view{"answered_by"},
    std::string_view{"seance_id"},    std::string_view{"callstart_ts"},
    std::string_view{"callstate_ts"},
};

constexpr std::array kRenamed{
    std::pair{std::string_view{"accountId"}, std::string_view{"account_id"}},
    std::pair{std::string_view{"at"}, std::string_view{"event_ts"}},
    std::pair{std::string_view{"duration"}, std::string_view{"duration_s"}},
    std::pair{std::string_view{"answeredBy"}, std::string_view{"answered_by"}},
    std::pair{std::string_view{"seanceId"}, std::string_view{"seance_id"}},
};

[[nodiscard]] bool unreserved(char character) noexcept
{
    return (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z') ||
           (character >= '0' && character <= '9') || character == '-' || character == '_' ||
           character == '.' || character == '~';
}

[[nodiscard]] std::string percentEncoded(std::string_view value)
{
    static constexpr char kHex[] = "0123456789ABCDEF";

    std::string encoded;
    encoded.reserve(value.size());
    for (const char character : value) {
        if (unreserved(character)) {
            encoded.push_back(character);
            continue;
        }
        encoded.push_back('%');
        encoded.push_back(kHex[(static_cast<unsigned char>(character) >> 4U) & 0x0FU]);
        encoded.push_back(kHex[static_cast<unsigned char>(character) & 0x0FU]);
    }
    return encoded;
}

[[nodiscard]] const std::string *valueOf(const WebhookFields &fields, std::string_view name)
{
    for (const auto &[key, value] : fields) {
        if (key == name) {
            return &value;
        }
    }
    return nullptr;
}

}

std::span<const std::string_view> webhookPlaceholders() noexcept
{
    return kPlaceholders;
}

WebhookFields webhookFields(const CallInfo &call, WebhookEvent event, const WebhookContext &context)
{
    WebhookFields fields;
    fields.reserve(kPlaceholders.size());

    fields.emplace_back("event", event);

    fields.emplace_back("id", call.uuid);
    fields.emplace_back("direction", call.direction == CallDirection::Incoming ? "in" : "out");

    fields.emplace_back("state", std::string{standingOf(call)});
    fields.emplace_back("number", sipUserPart(call.remoteUri));

    fields.emplace_back("name", context.name.empty() ? call.remoteDisplayName : context.name);

    fields.emplace_back("uri", call.remoteUri);

    fields.emplace_back("dialed", call.dialedNumber);

    fields.emplace_back("account", context.accountAddress);
    fields.emplace_back("account_id", context.accountId);

    fields.emplace_back("event_ts", unixMilliseconds(context.at));
    fields.emplace_back("duration_s", std::to_string(context.duration.count()));
    fields.emplace_back("reason", std::string{toString(call.end.reason)});

    fields.emplace_back("answered_by", std::string{toString(call.autoAnswer)});

    fields.emplace_back("seance_id", call.seanceId);

    fields.emplace_back("callstart_ts", unixMilliseconds(call.wallStartedAt));
    fields.emplace_back("callstate_ts", unixMilliseconds(call.wallStateSince));

    return fields;
}

std::string webhookJson(const WebhookFields &fields)
{
    nlohmann::json json = nlohmann::json::object();
    for (const auto &[name, value] : fields) {
        json[name] = value;
    }
    return json.dump();
}

std::string webhookQuery(const WebhookFields &fields)
{
    std::string query;
    for (const auto &[name, value] : fields) {
        if (!query.empty()) {
            query.push_back('&');
        }
        query += percentEncoded(name);
        query.push_back('=');
        query += percentEncoded(value);
    }
    return query;
}

std::string fillTemplate(std::string_view pattern, const WebhookFields &fields)
{
    std::string filled;
    filled.reserve(pattern.size());

    for (std::size_t at = 0; at < pattern.size();) {
        if (pattern[at] != '[') {
            filled.push_back(pattern[at]);
            ++at;
            continue;
        }

        const auto close = pattern.find(']', at);
        if (close == std::string_view::npos) {

            filled.append(pattern.substr(at));
            break;
        }

        const auto name = pattern.substr(at + 1, close - at - 1);
        if (const auto *value = valueOf(fields, name); value != nullptr) {

            filled += percentEncoded(*value);
        } else {

            filled.append(pattern.substr(at, close - at + 1));
        }
        at = close + 1;
    }

    return filled;
}

std::vector<std::string> unknownPlaceholders(std::string_view pattern)
{
    std::vector<std::string> unknown;

    for (std::size_t at = pattern.find('['); at != std::string_view::npos;
         at = pattern.find('[', at + 1)) {
        const auto close = pattern.find(']', at);
        if (close == std::string_view::npos) {
            break;
        }

        const auto name = pattern.substr(at + 1, close - at - 1);
        const auto known = std::ranges::any_of(
            kPlaceholders, [&name](const auto &candidate) { return candidate == name; });
        if (!known) {
            unknown.emplace_back(name);
        }
        at = close;
    }

    return unknown;
}

std::string renamedPlaceholders(std::string_view pattern)
{
    std::string renamed{pattern};
    for (const auto &[before, after] : kRenamed) {
        const auto token = '[' + std::string{before} + ']';
        const auto replacement = '[' + std::string{after} + ']';
        for (auto at = renamed.find(token); at != std::string::npos;
             at = renamed.find(token, at + replacement.size())) {
            renamed.replace(at, token.size(), replacement);
        }
    }
    return renamed;
}

std::optional<WebhookRequest> webhookFor(const WebhookSettings &settings,
                                         const WebhookFields &fields, WebhookEvent event,
                                         const std::string &headerValue)
{
    if (!settings.enabled || settings.url.empty() || !settings.wants(event)) {
        return std::nullopt;
    }

    WebhookRequest request;
    request.method = settings.method;

    if (!headerValue.empty() && !settings.headerName.empty()) {

        request.headers.emplace_back(settings.headerName, headerValue);
    }

    if (settings.method == HttpMethod::Post) {
        request.url = settings.url;
        request.body = webhookJson(fields);
        request.contentType = "application/json";
        return request;
    }

    if (!settings.urlTemplate.empty()) {
        request.url = fillTemplate(settings.urlTemplate, fields);
        return request;
    }

    request.url = settings.url;
    request.url += settings.url.find('?') == std::string::npos ? '?' : '&';
    request.url += webhookQuery(fields);
    return request;
}

}
