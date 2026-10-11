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

#include <softphone/core/metrics.hpp>

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::core {

using ApiResource = std::string;

enum class ApiAccess {

    Read,

    Control,

    Write,
};

[[nodiscard]] std::string_view toString(ApiAccess access) noexcept;

struct ApiRoute {
    std::string method;

    std::string path;

    ApiAccess access{ApiAccess::Read};

    ApiResource resource;

    std::string summary;
};

struct ApiRequest {
    std::string method;
    std::string path;
    std::string query;
    std::string body;

    std::vector<std::pair<std::string, std::string>> headers;

    [[nodiscard]] std::string header(std::string_view name) const;

    std::string id;
};

struct ApiResponse {
    int status{200};
    std::string contentType{"application/json"};
    std::string body;

    std::vector<std::pair<std::string, std::string>> headers;

    std::function<bool(std::uint64_t offset, std::size_t length, std::string &out)> reader;

    std::uint64_t size{0};

    [[nodiscard]] std::string headerNamed(std::string_view name) const;
};

[[nodiscard]] ApiResponse apiError(int status, std::string_view message);

[[nodiscard]] std::string metricsJson(std::span<const MetricReading> readings);

[[nodiscard]] std::string prometheusText(std::span<const MetricReading> readings);

[[nodiscard]] bool wantsPlainText(std::string_view accept) noexcept;

[[nodiscard]] bool sameSecret(std::string_view left, std::string_view right) noexcept;

[[nodiscard]] bool fromThisMachine(const ApiRequest &request) noexcept;

struct ApiPermissions {

    bool tokenConfigured{false};

    bool authorised{false};

    std::function<bool(std::string_view)> enabled;
};

class ApiRouter {
public:
    using Handler = std::function<ApiResponse(const ApiRequest &)>;

    void add(ApiRoute route, Handler handler);

    void removeRoutesOf(const ApiResource &resource);

    [[nodiscard]] std::vector<ApiRoute> routes() const;

    void handleRoot(Handler handler);

    [[nodiscard]] ApiResponse route(const ApiRequest &request,
                                    const ApiPermissions &permissions) const;

    [[nodiscard]] std::vector<ApiRoute> visible(const ApiPermissions &permissions) const;

private:
    struct Registered {
        ApiRoute route;
        Handler handler;
    };

    mutable std::mutex mutex_;

    std::vector<Registered> handlers_;

    std::vector<ApiRoute> routes_;

    Handler root_;
};

[[nodiscard]] std::string apiHelpPage(std::span<const ApiRoute> routes, bool authorised);

[[nodiscard]] std::optional<std::string> matchPath(std::string_view pattern, std::string_view path);

}
