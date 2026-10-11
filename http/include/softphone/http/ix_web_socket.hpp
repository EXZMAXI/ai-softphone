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

#include <softphone/core/web_socket.hpp>

#include <memory>
#include <string>

namespace softphone::http {

class IxWebSocketFactory final : public core::IWebSocketFactory {
public:
    IxWebSocketFactory();
    ~IxWebSocketFactory() override;

    [[nodiscard]] std::unique_ptr<core::IWebSocket> connect(const core::WebSocketEndpoint &where,
                                                            core::IWebSocketSink &sink) override;
};

[[nodiscard]] std::string webSocketLibraryVersion();

}
