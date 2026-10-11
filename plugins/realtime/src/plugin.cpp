// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/realtime/plugin.hpp>

#include <softphone/plugins/storage/storage_access.hpp>
#include <softphone/plugins/transcription/plugin.hpp>

#include <softphone/container/loading.hpp>

#include <softphone/core/version.hpp>

namespace softphone::plugins::realtime {

bool RealtimePlugin::initialize(container::PluginContext &context, std::string &problem)
{
    auto *const database = context.find<storage::IStorageAccess>(storage::kStorageAccess);
    if (database == nullptr) {
        problem = "the storage service is not there";
        return false;
    }

    database->declareCollections(std::string{kOwner}, kCollectionsVersion,
                                 std::string{collectionsDdl()});
    return true;
}

container::Manifest realtimeManifest()
{
    container::Manifest manifest;
    manifest.id = std::string{kOwner};
    manifest.version =
        container::parseVersion(core::versionString()).value_or(container::Version{});
    manifest.name = "Live prompter";
    manifest.summary = "Listening to a conversation in progress, and suggesting";

    manifest.requirements.push_back({std::string{transcription::kOwner}, {}});

    manifest.suggestions.push_back({"processing", {}});

    manifest.required = false;
    manifest.ui = true;
    return manifest;
}

}

SOFTPHONE_PLUGIN(::softphone::plugins::realtime::RealtimePlugin,
                 ::softphone::plugins::realtime::realtimeManifest)
