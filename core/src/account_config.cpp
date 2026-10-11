// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/account_config.hpp>

#include <algorithm>
#include <cctype>

namespace softphone::core {
namespace {

[[nodiscard]] bool equalFolded(std::string_view left, std::string_view right)
{
    return std::ranges::equal(left, right, [](char one, char other) {
        return std::tolower(static_cast<unsigned char>(one)) ==
               std::tolower(static_cast<unsigned char>(other));
    });
}

}

std::string AccountConfig::effectiveAuthUser() const
{
    return authUser.empty() ? username : authUser;
}

std::string AccountConfig::effectiveRegistrar() const
{
    return registrar.empty() ? domain : registrar;
}

std::string AccountConfig::effectiveOutboundProxy() const
{
    auto written = outboundProxy;

    if (written.empty() && port != 0 && port != defaultPortFor(transport)) {
        written = effectiveRegistrar() + ':' + std::to_string(port);
    }

    if (written.empty()) {
        return {};
    }

    auto uri =
        written.starts_with("sip:") || written.starts_with("sips:") ? written : "sip:" + written;

    return sayingTheTransport(std::move(uri), transport);
}

std::string sayingTheTransport(std::string uri, SipTransport transport)
{
    if (transport == SipTransport::Udp || !uri.starts_with("sip:") ||
        uri.find("transport=") != std::string::npos) {
        return uri;
    }

    const auto headers = uri.find('?');
    uri.insert(headers == std::string::npos ? uri.size() : headers,
               std::string{";transport="} + std::string{toString(transport)});
    return uri;
}

std::string AccountConfig::sipUri() const
{
    return "sip:" + username + '@' + domain;
}

AutoAnswerPolicy autoAnswerPolicy(const AccountConfig &account) noexcept
{
    return AutoAnswerPolicy{account.autoAnswer, account.autoAnswerAfter};
}

std::string switchHost(const AccountConfig &account)
{
    auto host = account.effectiveOutboundProxy();
    if (host.empty()) {
        host = account.effectiveRegistrar();
    }

    for (const auto *const scheme : {"sips:", "sip:"}) {
        if (host.starts_with(scheme)) {
            host.erase(0, std::string_view{scheme}.size());
            break;
        }
    }

    if (const auto end = host.find_first_of(";?"); end != std::string::npos) {
        host.erase(end);
    }
    if (const auto at = host.rfind('@'); at != std::string::npos) {
        host.erase(0, at + 1);
    }

    if (host.starts_with('[')) {
        if (const auto close = host.find(']'); close != std::string::npos) {
            return host.substr(1, close - 1);
        }
        return host;
    }
    if (const auto colon = host.rfind(':'); colon != std::string::npos) {
        host.erase(colon);
    }
    return host;
}

std::string accountLabel(const AccountConfig &account)
{
    if (!account.displayName.empty()) {
        return account.displayName;
    }
    return account.username + '@' + account.domain;
}

std::string shortAccountLabel(const AccountConfig &account)
{
    if (!account.displayName.empty()) {
        return account.displayName;
    }
    return account.username;
}

std::string accountAddress(const AccountConfig &account)
{
    if (account.username.empty() || account.domain.empty()) {
        return {};
    }
    return account.username + "@" + account.domain;
}

std::string credentialName(const AccountConfig &account)
{

    return accountAddress(account);
}

AccountChoice chooseAccount(std::span<const AccountConfig> accounts, std::string_view uuid,
                            std::string_view address, std::string_view fallbackAccountId)
{
    AccountChoice choice;
    choice.accountId = std::string{fallbackAccountId};
    choice.asked = !uuid.empty() || !address.empty();

    if (!choice.asked) {
        return choice;
    }

    if (!uuid.empty()) {
        for (const auto &account : accounts) {
            if (account.enabled && account.uuid == uuid) {
                choice.accountId = account.id;
                choice.honoured = true;
                return choice;
            }
        }
    }

    if (!address.empty()) {

        for (const auto &account : accounts) {
            if (account.enabled && equalFolded(accountAddress(account), address)) {
                choice.accountId = account.id;
                choice.honoured = true;
                return choice;
            }
        }
    }

    return choice;
}

ConfigProblem validate(const AccountConfig &config) noexcept
{
    if (config.id.empty()) {
        return ConfigProblem::MissingId;
    }
    if (config.username.empty()) {
        return ConfigProblem::MissingUsername;
    }
    if (config.domain.empty()) {
        return ConfigProblem::MissingDomain;
    }
    if (config.port == 0) {
        return ConfigProblem::InvalidPort;
    }
    if (config.expiry < kMinExpiry || config.expiry > kMaxExpiry) {
        return ConfigProblem::InvalidExpiry;
    }
    return ConfigProblem::None;
}

std::string_view toString(ConfigProblem problem) noexcept
{
    switch (problem) {
    case ConfigProblem::None:
        return "none";
    case ConfigProblem::MissingId:
        return "missing-id";
    case ConfigProblem::MissingUsername:
        return "missing-username";
    case ConfigProblem::MissingDomain:
        return "missing-domain";
    case ConfigProblem::InvalidPort:
        return "invalid-port";
    case ConfigProblem::InvalidExpiry:
        return "invalid-expiry";
    }
    return "unknown";
}

std::string_view toString(SipTransport transport) noexcept
{
    switch (transport) {
    case SipTransport::Udp:
        return "udp";
    case SipTransport::Tcp:
        return "tcp";
    case SipTransport::Tls:
        return "tls";
    }
    return "unknown";
}

std::optional<SipTransport> parseTransport(std::string_view text) noexcept
{
    if (text == "udp") {
        return SipTransport::Udp;
    }
    if (text == "tcp") {
        return SipTransport::Tcp;
    }
    if (text == "tls") {
        return SipTransport::Tls;
    }
    return std::nullopt;
}

std::string_view toString(MediaEncryption encryption) noexcept
{
    switch (encryption) {
    case MediaEncryption::Off:
        return "off";
    case MediaEncryption::Offered:
        return "offered";
    case MediaEncryption::Required:
        return "required";
    }
    return "unknown";
}

std::optional<MediaEncryption> parseMediaEncryption(std::string_view text) noexcept
{
    if (text == "off") {
        return MediaEncryption::Off;
    }
    if (text == "offered") {
        return MediaEncryption::Offered;
    }
    if (text == "required") {
        return MediaEncryption::Required;
    }
    return std::nullopt;
}

std::string_view toString(DtmfTransport transport) noexcept
{
    switch (transport) {
    case DtmfTransport::Rfc2833:
        return "rfc2833";
    case DtmfTransport::SipInfo:
        return "sip-info";
    }
    return "unknown";
}

std::optional<DtmfTransport> parseDtmfTransport(std::string_view text) noexcept
{
    if (text == "rfc2833") {
        return DtmfTransport::Rfc2833;
    }
    if (text == "sip-info") {
        return DtmfTransport::SipInfo;
    }
    return std::nullopt;
}

}
