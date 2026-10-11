// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/httplib_transport.hpp>

#include "machine_trust.hpp"
#include "split_url.hpp"

#include <softphone/core/http.hpp>
#include <softphone/core/version.hpp>

#include <httplib.h>

#include <string>
#include <string_view>

namespace softphone::http {
namespace {

[[nodiscard]] core::WebhookResult refuse(std::string reason)
{
    core::WebhookResult result;
    result.delivered = false;
    result.status = 0;
    result.error = std::move(reason);
    return result;
}

}

HttplibTransport::HttplibTransport(std::chrono::seconds timeout) : timeout_{timeout} {}

HttplibTransport::~HttplibTransport() = default;

bool HttplibTransport::supportsTls() noexcept
{
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    return true;
#else
    return false;
#endif
}

core::WebhookResult HttplibTransport::send(const core::WebhookRequest &request)
{
    const auto parts = split(request.url);
    if (!parts.ok) {

        auto refusal = refuse("not a URL this phone can send to: " + request.url);
        refusal.status = 400;
        return refusal;
    }

    if (parts.tls && !supportsTls()) {
        auto refusal = refuse("this build has no TLS, so it cannot reach " + request.url);
        refusal.status = 400;
        return refusal;
    }

    httplib::Client client{parts.origin};

    trustTheMachine(client, parts.host);

    client.set_connection_timeout(timeout_);
    client.set_read_timeout(timeout_);
    client.set_write_timeout(timeout_);

    client.set_follow_location(false);

    client.set_default_headers({{"User-Agent", std::string{core::userAgent()}}});

    httplib::Headers headers;
    for (const auto &[name, value] : request.headers) {
        headers.emplace(core::headerSafeName(name), core::headerSafeValue(value));
    }

    const auto answer = request.method == core::HttpMethod::Get
                            ? client.Get(parts.target, headers)
                            : client.Post(parts.target, headers, request.body, request.contentType);

    if (!answer) {
        return refuse(httplib::to_string(answer.error()));
    }

    core::WebhookResult result;
    result.status = answer->status;
    result.delivered = answer->status >= 200 && answer->status < 300;
    if (!result.delivered) {
        result.error = "the endpoint answered " + std::to_string(answer->status);
    }
    return result;
}

}
