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

#include <softphone/container/manifest.hpp>
#include <softphone/container/plugin.hpp>
#include <softphone/core/settings.hpp>
#include <softphone/core/webhooks.hpp>
#include <softphone/http/httplib_transport.hpp>
#include <softphone/plugins/integration/api_host.hpp>
#include <softphone/plugins/integration/events.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::plugins::integration {

class IntegrationPlugin final : public container::IPlugin {
public:

    explicit IntegrationPlugin(ApiHost::SecretLookup apiToken);

    ~IntegrationPlugin() override;

    bool initialize(container::PluginContext &context, std::string &problem) override;

    void leaving() noexcept override;

    void applyApi(const core::ApiSettings &api);
    void applyWebhooks(core::WebhookSettings webhooks);

    [[nodiscard]] core::ApiRouter &router() noexcept
    {
        return server_.router();
    }

    [[nodiscard]] IOutboundEvents &events() noexcept
    {
        return events_;
    }

private:

    class Outbound final : public IOutboundEvents {
    public:
        explicit Outbound(core::IWebhookTransport &transport)
            : transport_{transport},
              dispatcher_{std::make_unique<core::WebhookDispatcher>(transport)}
        {}

        void start();

        void declare(std::string event) override;
        [[nodiscard]] std::vector<std::string> declared() const override;
        [[nodiscard]] bool wanted(std::string_view event) const override;
        void post(core::WebhookRequest request) override;

        void setSettings(core::WebhookSettings settings);
        void stop() noexcept;

    private:

        core::IWebhookTransport &transport_;
        std::unique_ptr<core::WebhookDispatcher> dispatcher_;
        core::WebhookSettings settings_;
        std::vector<std::string> declared_;
    };

    ApiHost server_;

    http::HttplibTransport transport_;

    Outbound events_;
};

[[nodiscard]] container::Manifest integrationManifest();

inline constexpr std::string_view kOwner = "integration";

}
