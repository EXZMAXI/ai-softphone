// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/mediastorage/plugin.hpp>

#include <softphone/container/loading.hpp>
#include <softphone/plugins/storage/storage_access.hpp>

#include <softphone/core/version.hpp>

namespace softphone::plugins::mediastorage {

bool MediaStoragePlugin::initialize(container::PluginContext &context, std::string &problem)
{
    auto *const database = context.find<storage::IStorageAccess>(storage::kStorageAccess);
    if (database == nullptr) {
        problem = "the storage service is not there";
        return false;
    }

    database->declareCollections(std::string{kOwner}, kCollectionsVersion,
                                 std::string{collectionsDdl()});

    library_ = database->library();
    if (library_ == nullptr) {
        problem = "the store offers no recording repository";
        return false;
    }

    if (!context.offer<core::IRecordingStore>(std::string{kRecordingStore},
                                              container::Version{1, 0, 0}, library_)) {
        problem = "something else already offers " + std::string{kRecordingStore};
        return false;
    }
    return true;
}

container::Manifest mediaStorageManifest()
{
    container::Manifest manifest;
    manifest.id = std::string{kOwner};
    manifest.version =
        container::parseVersion(core::versionString()).value_or(container::Version{});

    manifest.name = "Mediastorage";
    manifest.summary =
        "The library of conversations, whatever recorded them, and how long they stay";

    manifest.requirements.push_back({"storage", {}});

    manifest.suggestions.push_back({"taxonomy", {}});

    manifest.required = false;
    manifest.ui = true;
    return manifest;
}

}

SOFTPHONE_PLUGIN(::softphone::plugins::mediastorage::MediaStoragePlugin,
                 ::softphone::plugins::mediastorage::mediaStorageManifest)
