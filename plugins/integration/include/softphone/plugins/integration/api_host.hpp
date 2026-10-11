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

#include <softphone/core/api.hpp>
#include <softphone/core/settings.hpp>
#include <softphone/http/httplib_server.hpp>

#include <functional>
#include <string>

namespace softphone::plugins::integration {

class ApiHost {
public:

    using SecretLookup = std::function<std::string()>;

    explicit ApiHost(SecretLookup secret);

    [[nodiscard]] core::ApiRouter &router() noexcept
    {
        return router_;
    }

    void apply(const core::ApiSettings &api);

    [[nodiscard]] bool running() const noexcept
    {
        return server_.running();
    }

private:
    [[nodiscard]] core::ApiResponse answer(const core::ApiRequest &request) const;
    [[nodiscard]] core::ApiPermissions permissionsFor(const core::ApiRequest &request) const;

    SecretLookup secret_;
    core::ApiRouter router_;
    core::ApiSettings settings_;
    http::HttplibServer server_;
};

}
