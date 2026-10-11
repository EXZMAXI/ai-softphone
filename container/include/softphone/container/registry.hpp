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

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::container {

enum class LoadState {

    Waiting,

    Ready,

    Disabled,

    Blocked,

    Cycle,

    Failed,
};

[[nodiscard]] std::string_view toString(LoadState state) noexcept;

struct Entry {
    Manifest manifest;

    IPlugin *plugin{nullptr};

    LoadState state{LoadState::Waiting};

    std::string problem;

    std::vector<std::string> blockedBy;
};

enum class AddProblem {
    None,

    BadManifest,

    DuplicateId,

    AlreadyStarted,
};

[[nodiscard]] std::string_view toString(AddProblem problem) noexcept;

class PluginRegistry {
public:
    PluginRegistry() = default;
    PluginRegistry(const PluginRegistry &) = delete;
    PluginRegistry &operator=(const PluginRegistry &) = delete;
    PluginRegistry(PluginRegistry &&) = delete;
    PluginRegistry &operator=(PluginRegistry &&) = delete;
    ~PluginRegistry() = default;

    [[nodiscard]] AddProblem add(Manifest manifest, IPlugin *plugin);

    [[nodiscard]] bool disable(std::string_view id);

    void resolve();

    [[nodiscard]] std::span<const std::string> order() const noexcept
    {
        return order_;
    }

    [[nodiscard]] std::span<const Entry> entries() const noexcept
    {
        return entries_;
    }

    [[nodiscard]] const Entry *find(std::string_view id) const;
    [[nodiscard]] Entry *find(std::string_view id);

    [[nodiscard]] bool attach(std::string_view id, IPlugin *plugin);

    [[nodiscard]] bool enable(std::string_view id);

    [[nodiscard]] bool requirementsReady(std::string_view id) const;

    [[nodiscard]] std::vector<std::string> requirementsOf(std::string_view id) const;

    [[nodiscard]] std::vector<std::string> dependentsOf(std::string_view id) const;

    [[nodiscard]] std::vector<std::string> suggestersOf(std::string_view id) const;

    void markFailed(std::string_view id, std::string problem);

private:
    void blockDependentsOf(std::string_view id);

    std::vector<Entry> entries_;
    std::vector<std::string> order_;
};

}
