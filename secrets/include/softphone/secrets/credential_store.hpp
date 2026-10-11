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

#include <softphone/core/credentials.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace softphone::secrets {

enum class SecretStatus {
    Ok,

    NotFound,

    ServiceUnavailable,

    Denied,
    Failed,
};

[[nodiscard]] std::string_view toString(SecretStatus status) noexcept;

class ICredentialStore : public core::ICredentialProvider {
public:

    [[nodiscard]] virtual bool available() const = 0;

    [[nodiscard]] virtual SecretStatus store(const std::string &accountId,
                                             const std::string &password) = 0;

    [[nodiscard]] virtual SecretStatus erase(const std::string &accountId) = 0;

    [[nodiscard]] virtual SecretStatus fetch(const std::string &accountId,
                                             std::string &password) const = 0;
};

enum class StoreKind {

    System,

    Sealed,

    None,
};

[[nodiscard]] std::unique_ptr<ICredentialStore> makePlatformCredentialStore();

struct ChosenStore {
    std::unique_ptr<ICredentialStore> store;
    StoreKind kind{StoreKind::None};
};

[[nodiscard]] ChosenStore makeCredentialStore();

}
