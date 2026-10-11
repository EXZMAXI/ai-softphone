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

#include <softphone/secrets/credential_store.hpp>

#include <optional>
#include <string>

namespace softphone::secrets {

class SecretServiceStore final : public ICredentialStore {
public:
    SecretServiceStore();
    ~SecretServiceStore() override;

    [[nodiscard]] bool available() const override;

    [[nodiscard]] SecretStatus store(const std::string &accountId,
                                     const std::string &password) override;

    [[nodiscard]] SecretStatus erase(const std::string &accountId) override;

    [[nodiscard]] SecretStatus fetch(const std::string &accountId,
                                     std::string &password) const override;

    [[nodiscard]] std::optional<std::string> password(const std::string &accountId) const override;
};

}
