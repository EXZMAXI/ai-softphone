// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/api.hpp>

#include <algorithm>
#include <charconv>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>

namespace softphone::core {
namespace {

[[nodiscard]] std::string lowered(std::string_view text)
{
    std::string lower;
    lower.reserve(text.size());
    for (const char character : text) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
    }
    return lower;
}

[[nodiscard]] bool loopbackAuthority(std::string_view authority) noexcept
{
    if (authority.empty()) {
        return false;
    }

    if (authority.front() == '[') {
        const auto close = authority.find(']');
        if (close == std::string_view::npos) {
            return false;
        }
        const auto host = authority.substr(1, close - 1);
        return host == "::1";
    }

    const auto colon = authority.rfind(':');
    const auto host = colon == std::string_view::npos ? authority : authority.substr(0, colon);
    return host == "127.0.0.1" || host == "localhost" || host == "::1";
}

}

std::string_view toString(ApiAccess access) noexcept
{
    switch (access) {
    case ApiAccess::Read:
        return "read";
    case ApiAccess::Control:
        return "control";
    case ApiAccess::Write:
        return "write";
    }
    return "unknown";
}

std::string ApiRequest::header(std::string_view name) const
{
    const auto wanted = lowered(name);
    for (const auto &[key, value] : headers) {
        if (lowered(key) == wanted) {
            return value;
        }
    }
    return {};
}

ApiResponse apiError(int status, std::string_view message)
{
    nlohmann::json json;
    json["error"] = std::string{message};

    ApiResponse response;
    response.status = status;
    response.body = json.dump();
    return response;
}

std::string metricsJson(std::span<const MetricReading> readings)
{
    auto json = nlohmann::json::array();
    for (const auto &reading : readings) {
        json.push_back(nlohmann::json{{"name", reading.name},
                                      {"help", reading.help},
                                      {"type", std::string{toString(reading.kind)}},
                                      {"value", reading.value}});
    }
    return json.dump();
}

bool wantsPlainText(std::string_view accept) noexcept
{

    return accept.find("text/plain") != std::string_view::npos;
}

std::string prometheusText(std::span<const MetricReading> readings)
{
    std::string text;
    for (const auto &reading : readings) {
        text += "# HELP " + reading.name + ' ' + reading.help + '\n';
        text += "# TYPE " + reading.name + ' ' + std::string{toString(reading.kind)} + '\n';

        const auto value = reading.value;
        if (value == static_cast<double>(static_cast<long long>(value))) {
            text += reading.name + ' ' + std::to_string(static_cast<long long>(value)) + '\n';
        } else {
            text += reading.name + ' ' + std::to_string(value) + '\n';
        }
    }
    return text;
}

bool sameSecret(std::string_view left, std::string_view right) noexcept
{

    unsigned char difference = left.size() == right.size() ? 0U : 1U;
    const auto shortest = std::min(left.size(), right.size());
    for (std::size_t at = 0; at < shortest; ++at) {
        difference |= static_cast<unsigned char>(left[at] ^ right[at]);
    }
    return difference == 0U && !left.empty();
}

bool fromThisMachine(const ApiRequest &request) noexcept
{
    if (!loopbackAuthority(request.header("Host"))) {
        return false;
    }

    const auto origin = request.header("Origin");
    if (origin.empty()) {
        return true;
    }

    constexpr std::string_view kHttp{"http://"};
    constexpr std::string_view kHttps{"https://"};
    if (origin.starts_with(kHttp)) {
        return loopbackAuthority(std::string_view{origin}.substr(kHttp.size()));
    }
    if (origin.starts_with(kHttps)) {
        return loopbackAuthority(std::string_view{origin}.substr(kHttps.size()));
    }
    return false;
}

namespace {

[[nodiscard]] std::string escaped(std::string_view text)
{
    std::string safe;
    safe.reserve(text.size());
    for (const char character : text) {
        switch (character) {
        case '&':
            safe += "&amp;";
            break;
        case '<':
            safe += "&lt;";
            break;
        case '>':
            safe += "&gt;";
            break;
        case '"':
            safe += "&quot;";
            break;
        default:
            safe.push_back(character);
        }
    }
    return safe;
}

}

std::string apiHelpPage(std::span<const ApiRoute> routes, bool authorised)
{
    std::string page;
    page += "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n";
    page += "<title>Softphone API</title>\n<style>\n";

    page += ":root{--fg:#222;--bg:#fff;--code:#f2f2f2;--link:#0645ad;--rule:rgba(0,0,0,.15)}\n";
    page += "@media(prefers-color-scheme:dark){:root{--fg:#ddd;--bg:#1b1b1b;--code:#2a2a2a;"
            "--link:#7ab7ff;--rule:rgba(255,255,255,.18)}}\n";
    page += "body{font:14px/1.5 system-ui,sans-serif;margin:2rem auto;max-width:52rem;padding:0 "
            "1rem;color:var(--fg);background:var(--bg)}\n";
    page += "a{color:var(--link)}\n";
    page += "h1{font-size:1.3rem}table{border-collapse:collapse;width:100%}\n";
    page += "td,th{text-align:left;padding:.35rem .6rem;border-bottom:1px solid "
            "var(--rule);vertical-align:top}\n";
    page += "code{font-family:ui-monospace,monospace;background:var(--code);padding:.1rem "
            ".3rem;border-radius:3px}\n";
    page += "p{opacity:.8}\n</style>\n</head>\n<body>\n";

    page += "<h1>Softphone API</h1>\n";
    page += "<p>Everything this telephone can be told to do, from this machine only. Answers are "
            "JSON.</p>\n";

    page += authorised
                ? "<p>You are authorised: everything below is available.</p>\n"
                : "<p>Without the authorization token you may read, and you may control calls. "
                  "Endpoints that change what is stored are not listed here; send the token in "
                  "the <code>Authorization</code> header to see them.</p>\n";

    page += "<table>\n<tr><th>Method</th><th>Endpoint</th><th>What it does</th></tr>\n";

    std::vector<const ApiRoute *> ordered;
    ordered.reserve(routes.size());
    for (const auto &route : routes) {
        if (std::ranges::any_of(
                ordered, [&route](const auto *seen) { return seen->resource == route.resource; })) {
            continue;
        }
        for (const auto &sibling : routes) {
            if (sibling.resource == route.resource) {
                ordered.push_back(&sibling);
            }
        }
    }

    for (const auto *listed : ordered) {
        const auto &route = *listed;
        page += "<tr><td><code>" + escaped(route.method) + "</code></td><td>";

        const auto linkable = route.method == "GET" && route.path.find("{id}") == std::string::npos;
        if (linkable) {
            page += "<a href=\"" + escaped(route.path) + "\"><code>" + escaped(route.path) +
                    "</code></a>";
        } else {
            page += "<code>" + escaped(route.path) + "</code>";
        }

        page += "</td><td>" + escaped(route.summary) + "</td></tr>\n";
    }
    page += "</table>\n";

    page += "<p>Identifiers are the ones the phone gives out: a call's id from "
            "<code>/calls</code>, an account's from <code>/accounts</code>.</p>\n";
    page += "</body>\n</html>\n";
    return page;
}

std::optional<std::string> matchPath(std::string_view pattern, std::string_view path)
{
    std::optional<std::string> captured;

    std::size_t patternAt = 0;
    std::size_t pathAt = 0;

    while (patternAt < pattern.size() || pathAt < path.size()) {
        const auto patternEnd = std::min(pattern.find('/', patternAt), pattern.size());
        const auto pathEnd = std::min(path.find('/', pathAt), path.size());

        const auto patternPart = pattern.substr(patternAt, patternEnd - patternAt);
        const auto pathPart = path.substr(pathAt, pathEnd - pathAt);

        if (patternPart == "{id}") {
            if (pathPart.empty()) {
                return std::nullopt;
            }
            captured = std::string{pathPart};
        } else if (patternPart != pathPart) {
            return std::nullopt;
        }

        if (patternEnd >= pattern.size() && pathEnd >= path.size()) {
            return captured.value_or(std::string{});
        }
        if (patternEnd >= pattern.size() || pathEnd >= path.size()) {
            return std::nullopt;
        }

        patternAt = patternEnd + 1;
        pathAt = pathEnd + 1;
    }

    return captured.value_or(std::string{});
}

void ApiRouter::handleRoot(Handler handler)
{
    root_ = std::move(handler);
}

void ApiRouter::add(ApiRoute route, Handler handler)
{
    const std::lock_guard<std::mutex> held{mutex_};

    for (auto &registered : handlers_) {
        if (registered.route.method != route.method || registered.route.path != route.path) {
            continue;
        }

        const auto at = static_cast<std::size_t>(&registered - handlers_.data());
        registered.handler = std::move(handler);
        registered.route = route;
        routes_[at] = std::move(route);
        return;
    }

    handlers_.push_back(Registered{route, std::move(handler)});
    routes_.push_back(std::move(route));
}

void ApiRouter::removeRoutesOf(const ApiResource &resource)
{
    const std::lock_guard<std::mutex> held{mutex_};

    std::erase_if(handlers_, [&resource](const Registered &registered) {
        return registered.route.resource == resource;
    });
    std::erase_if(routes_,
                  [&resource](const ApiRoute &route) { return route.resource == resource; });
}

std::vector<ApiRoute> ApiRouter::routes() const
{
    const std::lock_guard<std::mutex> held{mutex_};
    return routes_;
}

ApiResponse ApiRouter::route(const ApiRequest &request, const ApiPermissions &permissions) const
{

    if (!fromThisMachine(request)) {
        return apiError(403, "this API answers only to this machine");
    }

    if (request.path == "/" && request.method == "GET") {
        if (!root_) {
            return apiError(501, "this build serves no help page");
        }
        return root_(request);
    }

    bool pathExists = false;

    Handler chosen;
    ApiRequest answered = request;

    {
        const std::lock_guard<std::mutex> held{mutex_};
        for (const auto &registered : handlers_) {
            const auto &route = registered.route;
            auto captured = matchPath(route.path, request.path);
            if (!captured.has_value()) {
                continue;
            }
            pathExists = true;

            if (route.method != request.method) {
                continue;
            }

            if (permissions.enabled && !permissions.enabled(route.resource)) {
                return apiError(404, "no such endpoint");
            }

            if (route.access == ApiAccess::Write && !permissions.authorised) {
                return apiError(401, permissions.tokenConfigured
                                         ? "this endpoint needs the authorization token"
                                         : "writing needs a token, and none is configured");
            }

            if (!registered.handler) {
                return apiError(501, "this build does not serve that yet");
            }

            chosen = registered.handler;
            answered.id = *captured;
            break;
        }
    }

    if (chosen) {
        return chosen(answered);
    }

    return pathExists ? apiError(405, "that endpoint does not take this method")
                      : apiError(404, "no such endpoint");
}

std::vector<ApiRoute> ApiRouter::visible(const ApiPermissions &permissions) const
{
    const std::lock_guard<std::mutex> held{mutex_};

    std::vector<ApiRoute> visible;
    for (const auto &registered : handlers_) {
        const auto &route = registered.route;
        if (permissions.enabled && !permissions.enabled(route.resource)) {
            continue;
        }
        if (route.access == ApiAccess::Write && !permissions.authorised) {
            continue;
        }
        if (!registered.handler) {
            continue;
        }
        visible.push_back(route);
    }
    return visible;
}

std::string ApiResponse::headerNamed(std::string_view name) const
{
    const auto found = std::ranges::find_if(headers, [name](const auto &header) {
        return std::ranges::equal(header.first, name, [](char left, char right) {
            return std::tolower(static_cast<unsigned char>(left)) ==
                   std::tolower(static_cast<unsigned char>(right));
        });
    });
    return found == headers.end() ? std::string{} : found->second;
}

}
