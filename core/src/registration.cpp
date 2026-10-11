// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/registration.hpp>

namespace softphone::core {
namespace {

[[nodiscard]] RegistrationErrorCategory categoryForTransport(TransportFailure failure) noexcept
{
    switch (failure) {
    case TransportFailure::DnsFailure:
        return RegistrationErrorCategory::DomainNotResolved;
    case TransportFailure::Timeout:
    case TransportFailure::ConnectionFailed:
        return RegistrationErrorCategory::RegistrarUnreachable;
    case TransportFailure::CertificateRejected:
        return RegistrationErrorCategory::CertificateNotTrusted;
    case TransportFailure::None:
        break;
    }
    return RegistrationErrorCategory::None;
}

[[nodiscard]] RegistrationErrorCategory categoryForStatus(int status) noexcept
{
    switch (status) {
    case 401:
    case 407:
        return RegistrationErrorCategory::AuthenticationRejected;
    case 403:
        return RegistrationErrorCategory::Forbidden;
    case 404:
        return RegistrationErrorCategory::AccountNotFound;
    case 408:
        return RegistrationErrorCategory::RegistrarUnreachable;
    default:
        break;
    }

    if (status >= 500 && status <= 599) {
        return RegistrationErrorCategory::ServerUnavailable;
    }

    return RegistrationErrorCategory::Unknown;
}

}

bool RegistrationError::isTransient() const noexcept
{
    switch (category) {
    case RegistrationErrorCategory::RegistrarUnreachable:
    case RegistrationErrorCategory::DomainNotResolved:
    case RegistrationErrorCategory::ServerUnavailable:

    case RegistrationErrorCategory::Unknown:
        return true;
    case RegistrationErrorCategory::None:
    case RegistrationErrorCategory::InvalidConfiguration:

    case RegistrationErrorCategory::PasswordUnavailable:
    case RegistrationErrorCategory::AuthenticationRejected:
    case RegistrationErrorCategory::Forbidden:
    case RegistrationErrorCategory::AccountNotFound:

    case RegistrationErrorCategory::CertificateNotTrusted:
        break;
    }
    return false;
}

RegistrationError classify(const RegistrationOutcome &outcome)
{
    if (outcome.registered) {
        return RegistrationError{};
    }

    RegistrationError error;
    error.sipStatus = outcome.sipStatus;
    error.detail = outcome.reason;

    if (outcome.transportFailure != TransportFailure::None) {
        error.category = categoryForTransport(outcome.transportFailure);
        return error;
    }

    error.category = categoryForStatus(outcome.sipStatus);
    return error;
}

std::string_view toString(RegistrationState state) noexcept
{
    switch (state) {
    case RegistrationState::Offline:
        return "offline";
    case RegistrationState::Registering:
        return "registering";
    case RegistrationState::Registered:
        return "registered";
    case RegistrationState::Failed:
        return "failed";
    case RegistrationState::Unregistering:
        return "unregistering";
    }
    return "unknown";
}

std::string_view toString(RegistrationErrorCategory category) noexcept
{
    switch (category) {
    case RegistrationErrorCategory::None:
        return "none";
    case RegistrationErrorCategory::InvalidConfiguration:
        return "invalid-configuration";
    case RegistrationErrorCategory::PasswordUnavailable:
        return "password-unavailable";
    case RegistrationErrorCategory::AuthenticationRejected:
        return "authentication-rejected";
    case RegistrationErrorCategory::Forbidden:
        return "forbidden";
    case RegistrationErrorCategory::AccountNotFound:
        return "account-not-found";
    case RegistrationErrorCategory::RegistrarUnreachable:
        return "registrar-unreachable";
    case RegistrationErrorCategory::DomainNotResolved:
        return "domain-not-resolved";
    case RegistrationErrorCategory::ServerUnavailable:
        return "server-unavailable";
    case RegistrationErrorCategory::CertificateNotTrusted:
        return "certificate-not-trusted";
    case RegistrationErrorCategory::Unknown:
        return "unknown";
    }
    return "unknown";
}

}
