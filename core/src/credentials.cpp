// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/credentials.hpp>

#include <utility>

namespace softphone::core {

void InMemoryCredentialProvider::set(std::string accountId, std::string password)
{
    passwords_[std::move(accountId)] = std::move(password);
}

void InMemoryCredentialProvider::erase(const std::string &accountId)
{
    passwords_.erase(accountId);
}

std::optional<std::string> InMemoryCredentialProvider::password(const std::string &accountId) const
{
    const auto it = passwords_.find(accountId);
    if (it == passwords_.end()) {
        return std::nullopt;
    }
    return it->second;
}

}
