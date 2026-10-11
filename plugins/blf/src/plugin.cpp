// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/blf/plugin.hpp>

#include <softphone/container/loading.hpp>

#include <softphone/core/version.hpp>

namespace softphone::plugins::blf {

bool BlfPlugin::initialize(container::PluginContext &context, std::string &problem)
{
    static_cast<void>(context);
    static_cast<void>(problem);

    return true;
}

container::Manifest blfManifest()
{
    container::Manifest manifest;
    manifest.id = std::string{kOwner};
    manifest.version =
        container::parseVersion(core::versionString()).value_or(container::Version{});

    manifest.name = "Buttons";
    manifest.summary = "The buttons beside the dialler, and the lamps on them";

    manifest.requirements.push_back({"softphone", {}});

    manifest.required = false;
    manifest.ui = true;
    return manifest;
}

}

SOFTPHONE_PLUGIN(::softphone::plugins::blf::BlfPlugin, ::softphone::plugins::blf::blfManifest)
