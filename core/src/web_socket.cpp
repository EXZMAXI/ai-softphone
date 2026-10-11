// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/web_socket.hpp>

#include <algorithm>
#include <cstddef>

namespace softphone::core {

std::string_view toString(WebSocketStatus status) noexcept
{
    switch (status) {
    case WebSocketStatus::Ok:
        return "ok";
    case WebSocketStatus::Unauthorised:
        return "unauthorised";
    case WebSocketStatus::Unreachable:
        return "unreachable";
    case WebSocketStatus::Refused:
        return "refused";
    case WebSocketStatus::Untrusted:
        return "untrusted";
    case WebSocketStatus::Timeout:
        return "timeout";
    case WebSocketStatus::Cancelled:
        return "cancelled";
    case WebSocketStatus::Dropped:
        return "dropped";
    case WebSocketStatus::Failed:
        break;
    }
    return "failed";
}

bool worthReconnecting(WebSocketStatus status) noexcept
{
    switch (status) {
    case WebSocketStatus::Unreachable:
    case WebSocketStatus::Timeout:
    case WebSocketStatus::Dropped:
        return true;

    case WebSocketStatus::Ok:
    case WebSocketStatus::Unauthorised:
    case WebSocketStatus::Refused:
    case WebSocketStatus::Untrusted:
    case WebSocketStatus::Cancelled:
    case WebSocketStatus::Failed:
        break;
    }
    return false;
}

std::string WebSocketEndpoint::redacted() const
{

    if (!secret.empty() && secret.size() >= 8) {
        std::string shown = url;
        for (auto at = shown.find(secret); at != std::string::npos;
             at = shown.find(secret, at + 1)) {
            shown.replace(at, secret.size(), "…");
        }
        return shown;
    }

    const auto query = url.find('?');
    if (query == std::string::npos) {
        return url;
    }
    return url.substr(0, query) + "?…";
}

std::chrono::milliseconds reconnectWait(const WebSocketEndpoint &where, unsigned attempt) noexcept
{
    if (attempt == 0) {
        return std::chrono::milliseconds::zero();
    }

    auto wait = where.reconnectFirstWait;
    for (unsigned doubled = 1; doubled < attempt; ++doubled) {
        if (wait >= where.reconnectCeiling) {
            break;
        }
        wait *= 2;
    }
    return std::min(wait, where.reconnectCeiling);
}

}
