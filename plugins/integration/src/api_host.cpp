// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/integration/api_host.hpp>

#include <utility>

namespace softphone::plugins::integration {

ApiHost::ApiHost(SecretLookup secret)
    : secret_{std::move(secret)},
      server_{[this](const core::ApiRequest &request) { return answer(request); }}
{

    router_.handleRoot([this](const core::ApiRequest &request) {
        core::ApiResponse response;
        response.contentType = "text/html; charset=utf-8";

        const auto permissions = permissionsFor(request);
        const auto visible = router_.visible(permissions);
        response.body = core::apiHelpPage(visible, permissions.authorised);
        return response;
    });
}

void ApiHost::apply(const core::ApiSettings &api)
{
    const auto wasRunning = server_.running();
    const auto portChanged = api.port != settings_.port;
    settings_ = api;

    if (!api.enabled) {
        server_.stop();
        return;
    }

    if (!wasRunning || portChanged) {
        static_cast<void>(server_.start(api.port));
    }
}

core::ApiPermissions ApiHost::permissionsFor(const core::ApiRequest &request) const
{
    const auto token = secret_ ? secret_() : std::string{};

    core::ApiPermissions permissions;
    permissions.tokenConfigured = !token.empty();

    const auto offered = request.header("Authorization");
    constexpr std::string_view kBearer{"Bearer "};
    const auto bare = offered.starts_with(kBearer) ? offered.substr(kBearer.size()) : offered;

    permissions.authorised = core::sameSecret(token, bare);

    permissions.enabled = [settings = settings_](std::string_view resource) {
        return settings.serves(resource);
    };

    return permissions;
}

core::ApiResponse ApiHost::answer(const core::ApiRequest &request) const
{
    const auto answered = router_.route(request, permissionsFor(request));

    core::metrics::apiRequests().add();
    if (answered.status >= 400) {
        core::metrics::apiRequestsRefused().add();
    }

    return answered;
}

}
