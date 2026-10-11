// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/integration/plugin.hpp>

#include <softphone/core/version.hpp>

#include <algorithm>
#include <utility>

namespace softphone::plugins::integration {

void IntegrationPlugin::Outbound::declare(std::string event)
{
    if (event.empty() || std::ranges::find(declared_, event) != declared_.end()) {
        return;
    }
    declared_.push_back(std::move(event));
}

std::vector<std::string> IntegrationPlugin::Outbound::declared() const
{
    return declared_;
}

bool IntegrationPlugin::Outbound::wanted(std::string_view event) const
{
    return settings_.enabled && !settings_.url.empty() && settings_.wants(event);
}

void IntegrationPlugin::Outbound::post(core::WebhookRequest request)
{
    if (!dispatcher_) {

        return;
    }

    static_cast<void>(dispatcher_->post(std::move(request)));
}

void IntegrationPlugin::Outbound::setSettings(core::WebhookSettings settings)
{
    settings_ = std::move(settings);
}

void IntegrationPlugin::Outbound::start()
{
    if (!dispatcher_) {
        dispatcher_ = std::make_unique<core::WebhookDispatcher>(transport_);
    }
}

void IntegrationPlugin::Outbound::stop() noexcept
{

    dispatcher_.reset();
}

IntegrationPlugin::IntegrationPlugin(ApiHost::SecretLookup apiToken)
    : server_{std::move(apiToken)}, events_{transport_}
{}

IntegrationPlugin::~IntegrationPlugin() = default;

bool IntegrationPlugin::initialize(container::PluginContext &context, std::string &problem)
{

    events_.start();

    if (!context.offer<core::ApiRouter>(std::string{kRouteRegistry}, container::Version{1, 0, 0},
                                        &server_.router())) {
        problem = "something else already offers " + std::string{kRouteRegistry};
        return false;
    }

    if (!context.offer<IOutboundEvents>(std::string{kOutboundEvents}, container::Version{1, 0, 0},
                                        &events_)) {
        problem = "something else already offers " + std::string{kOutboundEvents};
        return false;
    }

    return true;
}

void IntegrationPlugin::leaving() noexcept
{

    server_.apply(core::ApiSettings{});
    events_.stop();
}

void IntegrationPlugin::applyApi(const core::ApiSettings &api)
{
    server_.apply(api);
}

void IntegrationPlugin::applyWebhooks(core::WebhookSettings webhooks)
{
    events_.setSettings(std::move(webhooks));
}

container::Manifest integrationManifest()
{
    container::Manifest manifest;
    manifest.id = std::string{kOwner};
    manifest.name = "Integration";
    manifest.summary = "The local API, and telling another system about calls";

    manifest.version =
        container::parseVersion(core::versionString()).value_or(container::Version{});

    manifest.ui = true;
    return manifest;
}

}
