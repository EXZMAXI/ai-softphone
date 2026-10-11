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
#include <softphone/core/call_log.hpp>
#include <softphone/plugins/softphone/engine_host.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace softphone::plugins::softphone {

class SoftphonePlugin final : public container::IPlugin {
public:

    using Power = std::function<void(bool)>;

    explicit SoftphonePlugin(EngineHost *engine = nullptr, Power power = {})
        : engine_{engine}, power_{std::move(power)}
    {}

    bool initialize(container::PluginContext &context, std::string &problem) override;

    void leaving() noexcept override;

private:

    EngineHost *engine_{nullptr};

    Power power_;
    core::ICallLog *log_{nullptr};
};

[[nodiscard]] container::Manifest softphoneManifest();

inline constexpr std::string_view kOwner = "softphone";

inline constexpr std::string_view kCallLog = "softphone.calls";

inline constexpr std::string_view kEngine = "softphone.engine";

[[nodiscard]] std::string_view collectionsDdl();
inline constexpr int kCollectionsVersion = 1;

}
