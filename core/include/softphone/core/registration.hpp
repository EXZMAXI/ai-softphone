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

#include <string>
#include <string_view>

namespace softphone::core {

enum class RegistrationState {
    Offline,
    Registering,
    Registered,
    Failed,
    Unregistering,
};

enum class TransportFailure {
    None,
    Timeout,
    DnsFailure,
    ConnectionFailed,

    CertificateRejected,
};

struct RegistrationOutcome {
    bool registered{false};
    int sipStatus{0};
    TransportFailure transportFailure{TransportFailure::None};
    std::string reason;

    std::string server{};
};

enum class RegistrationErrorCategory {
    None,
    InvalidConfiguration,

    PasswordUnavailable,
    AuthenticationRejected,
    Forbidden,
    AccountNotFound,
    RegistrarUnreachable,
    DomainNotResolved,
    ServerUnavailable,

    CertificateNotTrusted,
    Unknown,
};

struct RegistrationError {
    RegistrationErrorCategory category{RegistrationErrorCategory::None};
    int sipStatus{0};
    std::string detail;

    [[nodiscard]] bool isTransient() const noexcept;

    friend bool operator==(const RegistrationError &, const RegistrationError &) = default;
};

[[nodiscard]] RegistrationError classify(const RegistrationOutcome &outcome);

[[nodiscard]] std::string_view toString(RegistrationState state) noexcept;
[[nodiscard]] std::string_view toString(RegistrationErrorCategory category) noexcept;

}
