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

#include <softphone/core/auto_answer.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace softphone::core {

enum class SipTransport { Udp, Tcp, Tls };

enum class MediaEncryption {

    Off,

    Offered,

    Required,
};

enum class DtmfTransport {

    Rfc2833,

    SipInfo,
};

struct AccountConfig {

    std::string id;

    std::string uuid;
    std::string displayName;
    std::string username;
    std::string authUser;
    std::string domain;
    std::string registrar;
    std::string outboundProxy;
    std::uint16_t port{5060};
    SipTransport transport{SipTransport::Udp};
    std::chrono::seconds expiry{300};

    bool verifyCertificate{true};

    MediaEncryption mediaEncryption{MediaEncryption::Offered};

    bool enabled{true};

    DtmfTransport dtmfTransport{DtmfTransport::Rfc2833};

    std::string consent;

    bool autoAnswer{false};

    std::chrono::seconds autoAnswerAfter{0};

    [[nodiscard]] std::string effectiveAuthUser() const;

    [[nodiscard]] std::string effectiveRegistrar() const;

    static constexpr std::uint16_t kDefaultSipPort = 5060;

    static constexpr std::uint16_t kDefaultSipsPort = 5061;

    [[nodiscard]] static constexpr std::uint16_t defaultPortFor(SipTransport transport) noexcept
    {
        return transport == SipTransport::Tls ? kDefaultSipsPort : kDefaultSipPort;
    }

    [[nodiscard]] MediaEncryption effectiveMediaEncryption() const noexcept
    {
        return transport == SipTransport::Tls ? mediaEncryption : MediaEncryption::Off;
    }

    [[nodiscard]] std::string effectiveOutboundProxy() const;

    [[nodiscard]] std::string sipUri() const;

    friend bool operator==(const AccountConfig &, const AccountConfig &) = default;
};

[[nodiscard]] std::string accountLabel(const AccountConfig &account);

[[nodiscard]] std::string shortAccountLabel(const AccountConfig &account);

[[nodiscard]] std::string sayingTheTransport(std::string uri, SipTransport transport);

[[nodiscard]] std::string switchHost(const AccountConfig &account);

[[nodiscard]] std::string accountAddress(const AccountConfig &account);

[[nodiscard]] std::string credentialName(const AccountConfig &account);

struct AccountChoice {

    std::string accountId;

    bool asked{false};

    bool honoured{false};
};

[[nodiscard]] AccountChoice chooseAccount(std::span<const AccountConfig> accounts,
                                          std::string_view uuid, std::string_view address,
                                          std::string_view fallbackAccountId);

[[nodiscard]] AutoAnswerPolicy autoAnswerPolicy(const AccountConfig &account) noexcept;

inline constexpr std::chrono::seconds kMinExpiry{30};
inline constexpr std::chrono::seconds kMaxExpiry{86400};

enum class ConfigProblem {
    None,
    MissingId,
    MissingUsername,
    MissingDomain,
    InvalidPort,
    InvalidExpiry,
};

[[nodiscard]] ConfigProblem validate(const AccountConfig &config) noexcept;

[[nodiscard]] std::string_view toString(ConfigProblem problem) noexcept;
[[nodiscard]] std::string_view toString(SipTransport transport) noexcept;
[[nodiscard]] std::string_view toString(DtmfTransport transport) noexcept;

[[nodiscard]] std::optional<DtmfTransport> parseDtmfTransport(std::string_view text) noexcept;

[[nodiscard]] std::optional<SipTransport> parseTransport(std::string_view text) noexcept;

[[nodiscard]] std::string_view toString(MediaEncryption encryption) noexcept;

[[nodiscard]] std::optional<MediaEncryption> parseMediaEncryption(std::string_view text) noexcept;

}
