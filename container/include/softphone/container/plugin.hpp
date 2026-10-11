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

#include <softphone/container/bus.hpp>
#include <softphone/container/manifest.hpp>
#include <softphone/container/services.hpp>
#include <softphone/container/settings.hpp>

#include <optional>
#include <utility>

#include <string>

namespace softphone::container {

class PluginContext {
public:

    PluginContext(Manifest manifest, EventBus &bus, ServiceRegistry &services, Settings *settings)
        : manifest_{std::move(manifest)}, bus_{bus}, services_{services}, settings_{settings}
    {}

    [[nodiscard]] const Manifest &manifest() const noexcept
    {
        return manifest_;
    }

    [[nodiscard]] const std::string &id() const noexcept
    {
        return manifest_.id;
    }

    [[nodiscard]] EventBus &bus() noexcept
    {
        return bus_;
    }

    [[nodiscard]] ServiceRegistry &services() noexcept
    {
        return services_;
    }

    template<Event T>
    void publish(const T &payload)
    {
        bus_.publish(payload, manifest_.id);
    }

    template<typename T>
    [[nodiscard]] bool offer(std::string name, Version version, T *service)
    {
        return services_.offer<T>(std::move(name), version, manifest_.id, service);
    }

    template<typename T>
    [[nodiscard]] T *find(std::string_view name, const VersionRange &wanted = {}) const
    {
        return services_.find<T>(name, wanted);
    }

    [[nodiscard]] std::optional<SettingsSection> settings() const
    {
        if (settings_ == nullptr) {
            return std::nullopt;
        }
        return SettingsSection{*settings_, manifest_.id};
    }

private:
    Manifest manifest_;
    EventBus &bus_;
    ServiceRegistry &services_;
    Settings *settings_{nullptr};
};

class IPlugin {
public:
    IPlugin() = default;
    IPlugin(const IPlugin &) = delete;
    IPlugin &operator=(const IPlugin &) = delete;
    IPlugin(IPlugin &&) = delete;
    IPlugin &operator=(IPlugin &&) = delete;
    virtual ~IPlugin() = default;

    [[nodiscard]] virtual bool initialize(PluginContext &context, std::string &problem) = 0;

    [[nodiscard]] virtual bool ready(PluginContext &context, std::string &problem)
    {
        static_cast<void>(context);
        static_cast<void>(problem);
        return true;
    }

    virtual void leaving() noexcept {}
};

}
