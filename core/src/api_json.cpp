// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/api_json.hpp>

#include <softphone/core/time_text.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <ctime>

namespace softphone::core {
namespace {

[[nodiscard]] std::string percentDecoded(std::string_view text)
{
    std::string decoded;
    decoded.reserve(text.size());

    for (std::size_t at = 0; at < text.size(); ++at) {
        if (text[at] == '+') {
            decoded.push_back(' ');
            continue;
        }
        if (text[at] != '%' || at + 2 >= text.size()) {
            decoded.push_back(text[at]);
            continue;
        }

        const auto hex = text.substr(at + 1, 2);
        char *end = nullptr;
        const std::string digits{hex};
        const auto value = std::strtol(digits.c_str(), &end, 16);
        if (end != digits.c_str() + digits.size()) {
            decoded.push_back(text[at]);
            continue;
        }
        decoded.push_back(static_cast<char>(value));
        at += 2;
    }
    return decoded;
}

[[nodiscard]] nlohmann::json asJson(const Contact &contact)
{
    return nlohmann::json{{"id", contact.id},
                          {"name", contact.name},
                          {"number", contact.number},
                          {"account_id", contact.accountId},
                          {"account", contact.accountAddress}};
}

[[nodiscard]] nlohmann::json stampOf(std::chrono::system_clock::time_point when)
{
    if (when == std::chrono::system_clock::time_point{}) {
        return nullptr;
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(when.time_since_epoch()).count();
}

}

std::string queryValue(std::string_view query, std::string_view name)
{
    std::size_t at = 0;
    while (at < query.size()) {
        const auto end = std::min(query.find('&', at), query.size());
        const auto pair = query.substr(at, end - at);
        const auto equals = pair.find('=');
        if (equals != std::string_view::npos && pair.substr(0, equals) == name) {
            return percentDecoded(pair.substr(equals + 1));
        }
        at = end + 1;
    }
    return {};
}

std::string toJson(const Contact &contact)
{
    return asJson(contact).dump();
}

std::string toJson(std::span<const Contact> contacts)
{
    auto json = nlohmann::json::array();
    for (const auto &contact : contacts) {
        json.push_back(asJson(contact));
    }
    return json.dump();
}

std::string toJson(std::span<const TaxonomyEntry> entries)
{
    auto json = nlohmann::json::array();
    for (const auto &entry : entries) {
        nlohmann::json one{
            {"kind", std::string{toString(entry.kind)}},
            {"code", entry.code},
            {"title", entry.title},
            {"description", entry.description},
            {"retired", entry.retired()},
        };
        if (entry.severity.has_value()) {
            one["severity"] = std::string{toString(*entry.severity)};
        }
        json.push_back(std::move(one));
    }
    return json.dump();
}

std::string toJson(std::span<const CallRecord> calls)
{
    auto json = nlohmann::json::array();
    for (const auto &call : calls) {
        json.push_back(
            nlohmann::json{{"id", call.id},
                           {"seance_id", call.seanceId},
                           {"callstart_ts", stampOf(call.startedAt)},
                           {"direction", call.direction == CallDirection::Incoming ? "in" : "out"},
                           {"outcome", std::string{toString(outcome(call))}},
                           {"answered", call.answered},
                           {"duration_s", call.duration.count()},
                           {"number", call.number},
                           {"name", call.name},
                           {"uri", call.remoteUri},
                           {"dialed", call.dialedNumber},
                           {"account", call.accountAddress},
                           {"account_id", call.accountId},
                           {"reason", std::string{toString(call.endReason)}},
                           {"answered_by", std::string{toString(call.autoAnswer)}}});
    }
    return json.dump();
}

std::string toJson(std::span<const ApiCallView> calls, std::chrono::system_clock::time_point now)
{
    auto json = nlohmann::json::array();
    for (const auto &view : calls) {
        const auto &call = view.call;
        json.push_back(
            nlohmann::json{{"id", call.uuid},
                           {"seance_id", call.seanceId},
                           {"account_id", view.accountId},
                           {"direction", call.direction == CallDirection::Incoming ? "in" : "out"},
                           {"state", std::string{standingOf(call)}},
                           {"number", sipUserPart(call.remoteUri)},
                           {"name", call.remoteDisplayName},
                           {"uri", call.remoteUri},
                           {"dialed", call.dialedNumber},
                           {"muted", call.muted},

                           {"event_ts", stampOf(now)},
                           {"callstart_ts", stampOf(call.wallStartedAt)},
                           {"callstate_ts", stampOf(call.wallStateSince)}});
    }
    return json.dump();
}

std::string toJson(std::span<const ApiAccountView> accounts)
{
    auto json = nlohmann::json::array();
    for (const auto &account : accounts) {

        json.push_back(nlohmann::json{

            {"id", account.config.uuid},
            {"display_name", account.config.displayName},
            {"username", account.config.username},
            {"auth_user", account.config.authUser},
            {"domain", account.config.domain},
            {"registrar", account.config.registrar},
            {"outbound_proxy", account.config.outboundProxy},
            {"port", account.config.port},
            {"transport", toString(account.config.transport)},
            {"verify_certificate", account.config.verifyCertificate},
            {"media_encryption", toString(account.config.mediaEncryption)},
            {"expiry_s", account.config.expiry.count()},
            {"enabled", account.config.enabled},
            {"dtmf", account.config.dtmfTransport == DtmfTransport::SipInfo ? "info" : "rfc2833"},
            {"auto_answer", account.config.autoAnswer},
            {"auto_answer_after_s", account.config.autoAnswerAfter.count()},
            {"address", accountAddress(account.config)},
            {"state", account.state}});
    }
    return json.dump();
}

std::string stringFromJson(std::string_view body, std::string_view name)
{
    const auto json = nlohmann::json::parse(body, nullptr, false);
    if (json.is_discarded() || !json.is_object()) {
        return {};
    }
    const auto at = json.find(std::string{name});
    return at != json.end() && at->is_string() ? at->get<std::string>() : std::string{};
}

std::optional<Contact> contactFromJson(std::string_view body, std::string &problem)
{
    const auto json = nlohmann::json::parse(body, nullptr, false);
    if (json.is_discarded() || !json.is_object()) {
        problem = "the body is not a JSON object";
        return std::nullopt;
    }

    Contact contact;
    contact.name = json.value("name", std::string{});
    contact.number = json.value("number", std::string{});
    contact.accountId = json.value("account_id", std::string{});
    contact.accountAddress = json.value("account", std::string{});

    if (const auto fault = validate(contact); fault != ContactProblem::None) {
        problem = std::string{toString(fault)};
        return std::nullopt;
    }
    return contact;
}

std::optional<AccountConfig> accountFromJson(std::string_view body, const AccountConfig &current,
                                             std::string &problem)
{
    const auto json = nlohmann::json::parse(body, nullptr, false);
    if (json.is_discarded() || !json.is_object()) {
        problem = "the body is not a JSON object";
        return std::nullopt;
    }

    auto account = current;
    account.displayName = json.value("display_name", account.displayName);
    account.username = json.value("username", account.username);
    account.authUser = json.value("auth_user", account.authUser);
    account.domain = json.value("domain", account.domain);
    account.registrar = json.value("registrar", account.registrar);
    account.outboundProxy = json.value("outbound_proxy", account.outboundProxy);
    account.port = json.value("port", account.port);
    account.enabled = json.value("enabled", account.enabled);

    if (const auto transport = json.find("transport");
        transport != json.end() && transport->is_string()) {
        account.transport =
            parseTransport(transport->get<std::string>()).value_or(SipTransport::Udp);
    }
    account.verifyCertificate = json.value("verify_certificate", account.verifyCertificate);
    if (const auto encryption = json.find("media_encryption");
        encryption != json.end() && encryption->is_string()) {
        account.mediaEncryption =
            parseMediaEncryption(encryption->get<std::string>()).value_or(account.mediaEncryption);
    }
    if (const auto expiry = json.find("expiry_s");
        expiry != json.end() && expiry->is_number_integer()) {
        account.expiry = std::chrono::seconds{expiry->get<int>()};
    }
    if (const auto dtmf = json.find("dtmf"); dtmf != json.end() && dtmf->is_string()) {
        account.dtmfTransport =
            dtmf->get<std::string>() == "info" ? DtmfTransport::SipInfo : DtmfTransport::Rfc2833;
    }
    if (const auto answer = json.find("auto_answer");
        answer != json.end() && answer->is_boolean()) {
        account.autoAnswer = answer->get<bool>();
    }
    if (const auto after = json.find("auto_answer_after_s");
        after != json.end() && after->is_number_integer()) {
        const auto seconds = after->get<int>();
        if (seconds < 0 || std::chrono::seconds{seconds} > kMaxAutoAnswerSetting) {
            problem = "invalid-auto-answer-delay";
            return std::nullopt;
        }
        account.autoAnswerAfter = std::chrono::seconds{seconds};
    }

    if (const auto fault = validate(account); fault != ConfigProblem::None) {
        problem = std::string{toString(fault)};
        return std::nullopt;
    }
    return account;
}

}
