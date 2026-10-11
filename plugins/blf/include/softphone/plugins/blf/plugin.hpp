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

namespace softphone::plugins::blf {

class BlfPlugin final : public container::IPlugin {
public:
    bool initialize(container::PluginContext &context, std::string &problem) override;
};

[[nodiscard]] container::Manifest blfManifest();

inline constexpr std::string_view kOwner = "blf";

}
