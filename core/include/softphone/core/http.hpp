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

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::core {

enum class HttpVerb {
    Get,
    Post,
    Put,
    Remove,
};

[[nodiscard]] std::string_view toString(HttpVerb verb) noexcept;

[[nodiscard]] std::string asOneLine(std::string_view text);

[[nodiscard]] std::string headerSafeValue(std::string_view value);

[[nodiscard]] std::string headerSafeName(std::string_view name);

struct HttpPart {
    std::string name;

    std::string fileName;

    std::string contentType;

    std::string value;

    std::filesystem::path file;
};

struct HttpRequest {
    HttpVerb verb{HttpVerb::Post};
    std::string url;
    std::vector<std::pair<std::string, std::string>> headers;

    std::string body;

    std::filesystem::path bodyFile;

    std::string contentType{"application/json"};

    std::vector<HttpPart> parts;

    std::chrono::seconds timeout{std::chrono::minutes{20}};
};

struct HttpResponse {

    int status{0};

    std::vector<std::pair<std::string, std::string>> headers;
    std::string body;

    std::string error;

    [[nodiscard]] bool answered() const noexcept
    {
        return status != 0;
    }

    [[nodiscard]] bool ok() const noexcept
    {
        return status >= 200 && status < 300;
    }

    [[nodiscard]] std::string header(std::string_view name) const;
};

using HttpCancel = std::function<bool()>;

class IHttpClient {
public:
    IHttpClient() = default;
    IHttpClient(const IHttpClient &) = delete;
    IHttpClient &operator=(const IHttpClient &) = delete;
    IHttpClient(IHttpClient &&) = delete;
    IHttpClient &operator=(IHttpClient &&) = delete;
    virtual ~IHttpClient() = default;

    [[nodiscard]] virtual HttpResponse send(const HttpRequest &request,
                                            const HttpCancel &cancelled) = 0;
};

}
