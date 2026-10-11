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

#include <map>
#include <optional>
#include <string>

namespace softphone::core {

class ICredentialProvider {
public:
    ICredentialProvider() = default;
    ICredentialProvider(const ICredentialProvider &) = delete;
    ICredentialProvider &operator=(const ICredentialProvider &) = delete;
    ICredentialProvider(ICredentialProvider &&) = delete;
    ICredentialProvider &operator=(ICredentialProvider &&) = delete;
    virtual ~ICredentialProvider() = default;

    [[nodiscard]] virtual std::optional<std::string>
    password(const std::string &accountId) const = 0;
};

class InMemoryCredentialProvider final : public ICredentialProvider {
public:
    void set(std::string accountId, std::string password);
    void erase(const std::string &accountId);

    [[nodiscard]] std::optional<std::string> password(const std::string &accountId) const override;

private:
    std::map<std::string, std::string> passwords_;
};

}
