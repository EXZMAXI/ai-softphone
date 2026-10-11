// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/openai_chat.hpp>

#include <softphone/core/llm_dialect.hpp>
#include <softphone/core/provider_knobs.hpp>

#include "json_params.hpp"
#include "streaming_json.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::http {
namespace {

[[nodiscard]] const std::vector<core::ProviderKnob> &knobsOf(std::string_view type)
{
    static const auto openAi = core::knobsFor(core::ProviderKind::Llm, "openai");
    static const auto xai = core::knobsFor(core::ProviderKind::Llm, "xai");
    static const auto yandex = core::knobsFor(core::ProviderKind::Llm, "yandex");
    if (type == "xai") {
        return xai;
    }
    return type == "yandex" ? yandex : openAi;
}

[[nodiscard]] bool ours(const std::vector<core::ProviderKnob> &declared, std::string_view name)
{

    return core::isKnob(declared, name) || core::isDialectName(name);
}

[[nodiscard]] std::string paramOf(const std::vector<core::ProviderKnob> &declared,
                                  const core::LlmEndpoint &where, std::string_view name)
{
    return core::knobValue(where.params, declared, name);
}

[[nodiscard]] bool flagOf(const std::vector<core::ProviderKnob> &declared,
                          const core::LlmEndpoint &where, std::string_view name)
{
    const auto text = paramOf(declared, where, name);
    return text == "true" || text == "1" || text == "yes";
}

[[nodiscard]] int numberOf(const std::vector<core::ProviderKnob> &declared,
                           const core::LlmEndpoint &where, std::string_view name, int fallback)
{
    const auto text = paramOf(declared, where, name);
    if (text.empty()) {
        return fallback;
    }
    try {
        return std::stoi(text);
    } catch (const std::exception &) {
        return fallback;
    }
}

void authorise(core::HttpRequest &request, std::string_view type,
               const std::vector<core::ProviderKnob> &declared, const core::LlmEndpoint &where)
{
    if (type != "yandex") {
        if (!where.token.empty()) {
            request.headers.emplace_back("Authorization", "Bearer " + where.token);
        }
        return;
    }
    if (!where.token.empty()) {
        const bool iam = where.token.rfind("t1.", 0) == 0;
        request.headers.emplace_back("Authorization", (iam ? "Bearer " : "Api-Key ") + where.token);
    }
    if (const auto folder = paramOf(declared, where, "folder_id"); !folder.empty()) {
        request.headers.emplace_back("x-folder-id", folder);
    }
    request.headers.emplace_back("x-data-logging-enabled", "false");
}

[[nodiscard]] std::string modelFor(std::string_view type,
                                   const std::vector<core::ProviderKnob> &declared,
                                   const core::LlmEndpoint &where)
{
    if (type != "yandex" || where.model.empty() || where.model.find("://") != std::string::npos) {
        return where.model;
    }
    const auto folder = paramOf(declared, where, "folder_id");
    return folder.empty() ? where.model : "gpt://" + folder + '/' + where.model;
}

[[nodiscard]] std::string join(std::string_view base, std::string_view leaf)
{
    std::string address{base};
    while (!address.empty() && address.back() == '/') {
        address.pop_back();
    }
    return address + '/' + std::string{leaf};
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
        return core::LlmStatus::RateLimited;
    default:
        break;
    }

    if (answer.status == 400 || answer.status == 422) {
        const auto &body = answer.body;
        const auto mentionsContext = body.find("context") != std::string::npos ||
                                     body.find("too long") != std::string::npos ||
                                     body.find("too large") != std::string::npos;
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
    if (const auto detail = parsed.find("detail"); detail != parsed.end() && detail->is_string()) {
        return detail->get<std::string>();
    }
    const auto error = parsed.find("error");
    if (error == parsed.end()) {
        return {};
    }
    if (error->is_string()) {
        return error->get<std::string>();
    }
    if (error->is_object()) {
        if (const auto message = error->find("message");
            message != error->end() && message->is_string()) {
            return message->get<std::string>();
        }
    }
    return {};
}

}

OpenAiChatProvider::OpenAiChatProvider(core::IHttpClient &client, std::string type)
    : client_{client}, type_{std::move(type)}
{}

core::LlmCapabilities OpenAiChatProvider::capabilities(const core::LlmEndpoint &where) const
{
    const auto &declared = knobsOf(type_);
    core::LlmCapabilities what;

    what.contextTokens = numberOf(declared, where, "context_tokens", 8192);

    what.maxOutputTokens = numberOf(declared, where, "max_output", 0);

    const auto wants = core::dialectFor("openai", where.model, where.params);
    what.jsonMode =
        flagOf(declared, where, "json_mode") && wants.json == core::JsonAsk::ResponseFormat;

    what.prefill = false;

    what.maxConcurrent = std::max(1, numberOf(declared, where, "max_concurrent", 2));
    return what;
}

core::LlmAnswer OpenAiChatProvider::askOnce(const core::LlmEndpoint &where,
                                            const std::vector<core::LlmMessage> &messages,
                                            const core::LlmOptions &options,
                                            const core::LlmCancel &cancelled)
{
    const auto &declared = knobsOf(type_);
    if (where.url.empty()) {
        return refuse(core::LlmStatus::Unsupported, "no address is configured");
    }
    if (messages.empty()) {
        return refuse(core::LlmStatus::Unsupported, "nothing to ask");
    }

    const auto what = capabilities(where);

    nlohmann::ordered_json body;

    if (const auto named = modelFor(type_, declared, where); !named.empty()) {
        body["model"] = named;
    }

    const auto wants = core::dialectFor("openai", where.model, where.params);
    if (const auto heat = paramOf(declared, where, "temperature");
        !heat.empty() && wants.temperature) {
        body["temperature"] = jsonValueOf(heat);
    }

    auto turns = nlohmann::ordered_json::array();
    for (const auto &message : messages) {
        turns.push_back(
            {{"role", std::string{core::toString(message.role)}}, {"content", message.text}});
    }
    body["messages"] = std::move(turns);

    const auto asked = paramOf(declared, where, "max_output");
    if (const auto limit = asked.empty()                 ? 0
                           : options.maxOutputTokens > 0 ? options.maxOutputTokens
                                                         : what.maxOutputTokens;
        limit > 0 && !wants.answerLimit.empty()) {

        body[wants.answerLimit] = limit;
    }

    if (options.wantsJson && what.jsonMode) {
        body["response_format"] = {{"type", "json_object"}};
    }

    for (const auto &[name, value] : where.params) {
        if (!ours(declared, name)) {
            body[name] = jsonValueOf(value);
        }
    }

    core::HttpRequest request;
    request.verb = core::HttpVerb::Post;
    request.url = join(where.url, "chat/completions");
    request.body = body.dump();
    request.contentType = "application/json";
    request.timeout = where.timeout;
    authorise(request, type_, declared, where);

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

    const auto choices = parsed.find("choices");
    if (choices == parsed.end() || !choices->is_array() || choices->empty()) {
        auto refusal = refuse(core::LlmStatus::Malformed, "the answer carried no reply");
        refusal.raw = response.body;
        return sent(std::move(refusal));
    }

    core::LlmAnswer answer;
    answer.status = core::LlmStatus::Ok;
    answer.raw = response.body;
    answer.model = streaming::readOr(parsed, "model", where.model);

    const auto &first = choices->front();
    if (const auto message = first.find("message"); message != first.end()) {
        const auto content = message->find("content");
        if (content != message->end() && content->is_string()) {
            answer.text = content->get<std::string>();
        }
    }

    if (const auto reason = first.find("finish_reason");
        reason != first.end() && reason->is_string() && reason->get<std::string>() == "length") {
        answer.status = core::LlmStatus::TooLarge;
        answer.detail = "the model ran out of room before it finished";
    }

    if (const auto usage = parsed.find("usage"); usage != parsed.end() && usage->is_object()) {
        answer.usage.promptTokens = streaming::readOr(*usage, "prompt_tokens", 0);
        answer.usage.answerTokens = streaming::readOr(*usage, "completion_tokens", 0);
    }

    if (answer.text.empty() && answer.status == core::LlmStatus::Ok) {
        auto refusal = refuse(core::LlmStatus::Malformed, "the model answered with nothing");
        refusal.raw = response.body;
        return sent(std::move(refusal));
    }
    return sent(std::move(answer));
}

core::LlmAnswer OpenAiChatProvider::ask(const core::LlmEndpoint &where,
                                        const std::vector<core::LlmMessage> &messages,
                                        const core::LlmOptions &options,
                                        const core::LlmCancel &cancelled)
{
    return core::askLearning(
        [this, &messages, &options, &cancelled](const core::LlmEndpoint &corrected) {
            return askOnce(corrected, messages, options, cancelled);
        },
        "openai", where);
}

core::LlmProbe OpenAiChatProvider::probe(const core::LlmEndpoint &where)
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
    authorise(listing, type_, knobsOf(type_), where);

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

    std::vector<core::LlmMessage> messages;
    messages.push_back({core::LlmRole::User, "Reply with the single word: ready"});

    auto trial = where;
    trial.timeout = std::chrono::seconds{30};

    const auto answer = ask(trial, messages, options, {});

    found.learnt = answer.learnt;

    found.status = answer.status;
    found.detail = answer.detail;

    found.reachable =
        answer.status == core::LlmStatus::Ok || answer.status == core::LlmStatus::TooLarge;
    if (found.reachable) {
        found.status = core::LlmStatus::Ok;
        found.detail.clear();
    }
    return found;
}

}
