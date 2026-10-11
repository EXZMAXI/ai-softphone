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

#include <openssl/types.h>

#include <string>

namespace httplib {
class Client;
}

namespace softphone::http {

void trustTheMachine(httplib::Client &client, const std::string &host);

[[nodiscard]] std::string trustTheMachine(SSL_CTX *context, const std::string &host);

[[nodiscard]] std::string trustStoreForWebSockets();

}
