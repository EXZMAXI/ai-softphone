// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/anthropic_chat.hpp>

#include <softphone/core/llm_dialect.hpp>
#include <softphone/core/provider_knobs.hpp>

#include "json_params.hpp"
#include "streaming_json.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::http {
namespace {

constexpr std::string_view kApiVersion = "2023-06-01";

[[nodiscard]] const std::vector<core::ProviderKnob> &knobs()
{
    static const auto declared = core::knobsFor(core::ProviderKind::Llm, "anthropic");
    return declared;
}

[[nodiscard]] bool ours(std::string_view name)
{

    return core::isKnob(knobs(), name) || core::isDialectName(name);
}

[[nodiscard]] std::string paramOf(const core::LlmEndpoint &where, std::string_view name)
{
    return core::knobValue(where.params, knobs(), name);
}

[[nodiscard]] int numberOf(const core::LlmEndpoint &where, std::string_view name, int fallback)
{
    const auto text = paramOf(where, name);
    if (text.empty()) {
        return fallback;
    }
    try {
        return std::stoi(text);
    } catch (const std::exception &) {
        return fallback;
    }
}

[[nodiscard]] std::string join(std::string_view base, std::string_view leaf)
{
    std::string address{base};
    while (!address.empty() && address.back() == '/') {
        address.pop_back();
    }
    return address + '/' + std::string{leaf};
}

[[nodiscard]] bool startsWith(std::string_view text, std::string_view what)
{
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) {
        text.remove_prefix(1);
    }
    return text.starts_with(what);
}

[[nodiscard]] core::LlmAnswer refuse(core::LlmStatus status, std::string detail)
{
    core::LlmAnswer answer;
    answer.status = status;
    answer.detail = std::move(detail);
    return answer;
}

[[nodiscard]] core::LlmStatus statusOf(const core::HttpResponse &answer)
{
    if (!answer.answered()) {
        return core::LlmStatus::Unreachable;
    }
    if (answer.ok()) {
        return core::LlmStatus::Ok;
    }
    switch (answer.status) {
    case 401:
    case 403:
        return core::LlmStatus::Unauthorised;
    case 404:
        return core::LlmStatus::Refused;
    case 413:
        return core::LlmStatus::TooLarge;
    case 429:
    case 529:
        return core::LlmStatus::RateLimited;
    default:
        break;
    }
    if (answer.status == 400) {
        const auto mentionsContext = answer.body.find("context") != std::string::npos ||
                                     answer.body.find("too long") != std::string::npos;
        return mentionsContext ? core::LlmStatus::TooLarge : core::LlmStatus::Refused;
    }
    return answer.status >= 500 ? core::LlmStatus::Unreachable : core::LlmStatus::Failed;
}

[[nodiscard]] std::string complaintIn(const std::string &body)
{
    const auto parsed = nlohmann::json::parse(body, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return {};
    }
    const auto error = parsed.find("error");
    if (error == parsed.end() || !error->is_object()) {
        return {};
    }
    if (const auto message = error->find("message");
        message != error->end() && message->is_string()) {
        return message->get<std::string>();
    }
    return {};
}

}

AnthropicChatProvider::AnthropicChatProvider(core::IHttpClient &client) : client_{client} {}

core::LlmCapabilities AnthropicChatProvider::capabilities(const core::LlmEndpoint &where) const
{
    core::LlmCapabilities what;

    what.contextTokens = numberOf(where, "context_tokens", 200'000);
    what.maxOutputTokens = numberOf(where, "max_output", 8192);

    what.jsonMode = false;
    what.prefill =
        core::dialectFor("anthropic", where.model, where.params).json == core::JsonAsk::Prefill;

    what.maxConcurrent = std::max(1, numberOf(where, "max_concurrent", 2));
    return what;
}

core::LlmAnswer AnthropicChatProvider::askOnce(const core::LlmEndpoint &where,
                                               const std::vector<core::LlmMessage> &messages,
                                               const core::LlmOptions &options,
                                               const core::LlmCancel &cancelled)
{
    if (where.url.empty()) {
        return refuse(core::LlmStatus::Unsupported, "no address is configured");
    }

    const auto what = capabilities(where);
    const auto wants = core::dialectFor("anthropic", where.model, where.params);

    std::string system;
    auto turns = nlohmann::ordered_json::array();
    for (const auto &message : messages) {
        if (message.role == core::LlmRole::System) {
            if (!system.empty()) {
                system += "\n\n";
            }
            system += message.text;
            continue;
        }
        turns.push_back(
            {{"role", std::string{core::toString(message.role)}}, {"content", message.text}});
    }

    std::string prefill = options.prefill;
    if (options.wantsJson && prefill.empty() && wants.json == core::JsonAsk::Prefill) {
        prefill = "{";
    }
    if (!prefill.empty()) {
        turns.push_back({{"role", "assistant"}, {"content", prefill}});
    }

    if (turns.empty()) {
        return refuse(core::LlmStatus::Unsupported, "nothing to ask");
    }

    nlohmann::ordered_json body;

    if (!where.model.empty()) {
        body["model"] = where.model;
    }
    body["messages"] = std::move(turns);

    if (const auto heat = paramOf(where, "temperature"); !heat.empty() && wants.temperature) {
        body["temperature"] = jsonValueOf(heat);
    }

    if (!wants.answerLimit.empty()) {
        body[wants.answerLimit] = options.maxOutputTokens > 0 ? options.maxOutputTokens
                                                              : std::max(1, what.maxOutputTokens);
    }
    if (!system.empty()) {
        body["system"] = system;
    }

    switch (wants.thinking) {
    case core::Thinking::AsTheModelLikes:
        break;
    case core::Thinking::Off:
        body["thinking"] = {{"type", "disabled"}};
        break;
    case core::Thinking::Low:
        body["output_config"] = {{"effort", "low"}};
        break;
    }

    for (const auto &[name, value] : where.params) {
        if (!ours(name)) {
            body[name] = jsonValueOf(value);
        }
    }

    core::HttpRequest request;
    request.verb = core::HttpVerb::Post;
    request.url = join(where.url, "messages");
    request.body = body.dump();
    request.contentType = "application/json";
    request.timeout = where.timeout;
    request.headers.emplace_back("anthropic-version", std::string{kApiVersion});
    if (!where.token.empty()) {

        request.headers.emplace_back("x-api-key", where.token);
    }

    const auto sent = [&request](core::LlmAnswer answer) {
        answer.request = request.body;
        return answer;
    };

    const auto response = client_.send(request, cancelled);
    const auto status = statusOf(response);
    if (status != core::LlmStatus::Ok) {
        auto detail = complaintIn(response.body);
        if (detail.empty()) {
            detail = response.answered()
                         ? "the provider answered " + std::to_string(response.status)
                         : response.error;
        }
        auto refusal = refuse(status, std::move(detail));
        refusal.raw = response.body;
        return sent(std::move(refusal));
    }

    const auto parsed = nlohmann::json::parse(response.body, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        auto refusal = refuse(core::LlmStatus::Malformed, "the answer was not JSON");
        refusal.raw = response.body;
        return sent(std::move(refusal));
    }

    core::LlmAnswer answer;
    answer.status = core::LlmStatus::Ok;
    answer.raw = response.body;
    answer.model = streaming::readOr(parsed, "model", where.model);

    if (const auto content = parsed.find("content");
        content != parsed.end() && content->is_array()) {
        for (const auto &block : *content) {
            if (streaming::readOr(block, "type", std::string{}) == "text") {
                answer.text += streaming::readOr(block, "text", std::string{});
            }
        }
    }

    if (!prefill.empty() && !answer.text.empty() && !startsWith(answer.text, prefill)) {
        answer.text = prefill + answer.text;
    }

    if (const auto usage = parsed.find("usage"); usage != parsed.end() && usage->is_object()) {
        answer.usage.promptTokens = streaming::readOr(*usage, "input_tokens", 0);
        answer.usage.answerTokens = streaming::readOr(*usage, "output_tokens", 0);
    }

    if (streaming::readOr(parsed, "stop_reason", std::string{}) == "max_tokens") {
        answer.status = core::LlmStatus::TooLarge;

        answer.detail = answer.text.empty()
                            ? "the model spent its whole limit thinking and wrote no answer"
                            : "the model ran out of room before it finished";
    } else if (answer.text.empty()) {
        auto refusal = refuse(core::LlmStatus::Malformed, "the model answered with nothing");
        refusal.raw = response.body;
        return sent(std::move(refusal));
    }
    return sent(std::move(answer));
}

core::LlmAnswer AnthropicChatProvider::ask(const core::LlmEndpoint &where,
                                           const std::vector<core::LlmMessage> &messages,
                                           const core::LlmOptions &options,
                                           const core::LlmCancel &cancelled)
{
    return core::askLearning(
        [this, &messages, &options, &cancelled](const core::LlmEndpoint &corrected) {
            return askOnce(corrected, messages, options, cancelled);
        },
        "anthropic", where);
}

core::LlmProbe AnthropicChatProvider::probe(const core::LlmEndpoint &where)
{
    core::LlmProbe found;
    if (where.url.empty()) {
        found.status = core::LlmStatus::Unsupported;
        found.detail = "no address is configured";
        return found;
    }

    core::HttpRequest listing;
    listing.verb = core::HttpVerb::Get;
    listing.url = join(where.url, "models");
    listing.timeout = std::chrono::seconds{20};
    listing.headers.emplace_back("anthropic-version", std::string{kApiVersion});
    if (!where.token.empty()) {
        listing.headers.emplace_back("x-api-key", where.token);
    }

    const auto listed = client_.send(listing, {});
    if (listed.ok()) {
        const auto parsed = nlohmann::json::parse(listed.body, nullptr, false);
        if (!parsed.is_discarded() && parsed.is_object()) {
            if (const auto data = parsed.find("data"); data != parsed.end() && data->is_array()) {
                for (const auto &entry : *data) {
                    if (auto id = streaming::readOr(entry, "id", std::string{}); !id.empty()) {
                        found.models.push_back(std::move(id));
                    }
                }
            }
        }
    }

    if (where.model.empty()) {
        found.reachable = listed.ok() && !found.models.empty();
        found.status = found.reachable ? core::LlmStatus::Ok : statusOf(listed);
        if (!found.reachable) {
            found.detail = complaintIn(listed.body);
            if (found.detail.empty()) {
                found.detail = listed.answered() ? "no model is chosen" : listed.error;
            }
        }
        return found;
    }

    core::LlmOptions options;
    options.maxOutputTokens = 256;

    options.wantsJson =
        core::dialectFor("anthropic", where.model, where.params).json == core::JsonAsk::Prefill;

    std::vector<core::LlmMessage> messages;
    messages.push_back({core::LlmRole::User, options.wantsJson
                                                 ? "Answer with the JSON object {\"ready\": true}"
                                                 : "Reply with the single word: ready"});

    auto trial = where;
    trial.timeout = std::chrono::seconds{30};

    const auto answer = ask(trial, messages, options, {});
    found.status = answer.status;
    found.detail = answer.detail;

    found.learnt = answer.learnt;

    found.reachable =
        answer.status == core::LlmStatus::Ok || answer.status == core::LlmStatus::TooLarge;
    if (found.reachable) {
        found.status = core::LlmStatus::Ok;
        found.detail.clear();
    }
    return found;
}

}
