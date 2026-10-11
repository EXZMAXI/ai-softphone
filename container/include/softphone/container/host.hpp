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
#include <softphone/container/registry.hpp>
#include <softphone/container/services.hpp>
#include <softphone/container/settings.hpp>

#include <filesystem>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::container {

enum class Severity {
    Error,
    Warning,
    Info,
    Trace,
};

using LogSink = std::function<void(Severity, std::string_view category, std::string_view message)>;

class Host {
public:
    Host() = default;
    explicit Host(LogSink log);

    Host(LogSink log, std::filesystem::path settingsFile, FileProtector protect,
         std::string adoptOlderDocumentsAs = {});

    Host(const Host &) = delete;
    Host &operator=(const Host &) = delete;
    Host(Host &&) = delete;
    Host &operator=(Host &&) = delete;
    ~Host();

    [[nodiscard]] AddProblem add(Manifest manifest, IPlugin *plugin);

    [[nodiscard]] bool attach(std::string_view id, IPlugin *plugin);

    [[nodiscard]] bool disable(std::string_view id);

    struct StartReport {
        std::size_t started{0};
        std::size_t disabled{0};
        std::size_t blocked{0};
        std::size_t failed{0};
        std::size_t cycled{0};

        [[nodiscard]] bool clean() const noexcept
        {
            return blocked == 0 && failed == 0 && cycled == 0;
        }
    };

    [[nodiscard]] StartReport start();

    struct LiveChange {

        std::vector<std::string> changed;

        std::string problem;

        [[nodiscard]] bool ok() const noexcept
        {
            return problem.empty();
        }
    };

    [[nodiscard]] LiveChange activate(std::string_view id);

    [[nodiscard]] LiveChange deactivate(std::string_view id);

    void leave() noexcept;

    [[nodiscard]] EventBus &bus() noexcept
    {
        return bus_;
    }

    [[nodiscard]] ServiceRegistry &services() noexcept
    {
        return services_;
    }

    [[nodiscard]] const PluginRegistry &plugins() const noexcept
    {
        return registry_;
    }

    [[nodiscard]] Settings *settings() noexcept
    {
        return settings_.get();
    }

    [[nodiscard]] SettingsStatus settingsStatus() const noexcept
    {
        return settingsStatus_;
    }

    [[nodiscard]] std::vector<std::string> describe() const;

private:
    void write(Severity severity, std::string_view category, std::string_view message) const;

    LogSink log_;
    EventBus bus_;
    ServiceRegistry services_;
    PluginRegistry registry_;
    std::unique_ptr<Settings> settings_;
    SettingsStatus settingsStatus_{SettingsStatus::NotFound};

    std::vector<std::unique_ptr<PluginContext>> contexts_;
    std::vector<std::string> started_;

    EventBus::Token watcher_{EventBus::kNoToken};
    bool started_once_{false};
    bool left_{false};
};

}
