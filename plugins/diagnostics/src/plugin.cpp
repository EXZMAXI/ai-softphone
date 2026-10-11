// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/diagnostics/plugin.hpp>

#include <softphone/core/api.hpp>
#include <softphone/core/metrics.hpp>
#include <softphone/core/version.hpp>
#include <softphone/plugins/integration/events.hpp>
#include <softphone/plugins/integration/plugin.hpp>

#include <string>

namespace softphone::plugins::diagnostics {

bool DiagnosticsPlugin::initialize(container::PluginContext &context, std::string &problem)
{

    static_cast<void>(context);
    static_cast<void>(problem);
    return true;
}

bool DiagnosticsPlugin::ready(container::PluginContext &context, std::string &problem)
{
    static_cast<void>(problem);

    auto *const routes = context.find<core::ApiRouter>(integration::kRouteRegistry);
    if (routes == nullptr) {

        return true;
    }

    routes_ = routes;

    routes->add(core::ApiRoute{"GET", "/metrics", core::ApiAccess::Read,
                               std::string{kMetricsResource}, "Counters in Prometheus text format"},
                [](const core::ApiRequest &request) {
                    const auto readings = core::MetricRegistry::instance().readings();

                    core::ApiResponse response;
                    if (core::wantsPlainText(request.header("Accept"))) {

                        response.contentType = "text/plain; version=0.0.4; charset=utf-8";
                        response.body = core::prometheusText(readings);
                    } else {
                        response.body = core::metricsJson(readings);
                    }
                    return response;
                });
    return true;
}

void DiagnosticsPlugin::leaving() noexcept
{
    if (routes_ != nullptr) {
        routes_->removeRoutesOf(core::ApiResource{kMetricsResource});
        routes_ = nullptr;
    }
}

container::Manifest diagnosticsManifest()
{
    container::Manifest manifest;
    manifest.id = std::string{kOwner};
    manifest.name = "Diagnostics";
    manifest.summary = "The SIP trace, the media statistics, the counters and the bus";

    manifest.version =
        container::parseVersion(core::versionString()).value_or(container::Version{});

    manifest.suggestions.push_back(container::Dependency{std::string{integration::kOwner}, {}});

    manifest.ui = true;
    return manifest;
}

}
