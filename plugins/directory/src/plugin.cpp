// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/directory/plugin.hpp>

#include <softphone/container/loading.hpp>

#include <softphone/core/version.hpp>

#include <softphone/plugins/storage/storage_access.hpp>

#include <string>

namespace softphone::plugins::directory {

bool DirectoryPlugin::initialize(container::PluginContext &context, std::string &problem)
{
    auto *const database = context.find<storage::IStorageAccess>(storage::kStorageAccess);
    if (database == nullptr) {

        problem = "the storage service is not there";
        return false;
    }

    database->declareCollections(std::string{kOwner}, kCollectionsVersion,
                                 std::string{collectionsDdl()});

    book_ = database->contacts();
    if (book_ == nullptr) {
        problem = "the store offers no address book";
        return false;
    }

    if (!context.offer<core::IContactStore>(std::string{kContactStore}, container::Version{1, 0, 0},
                                            book_)) {
        problem = "something else already offers " + std::string{kContactStore};
        return false;
    }

    return true;
}

container::Manifest directoryManifest()
{
    container::Manifest manifest;
    manifest.id = std::string{kOwner};
    manifest.name = "Directory";
    manifest.summary = "The address book, and the window it is read in";

    manifest.version =
        container::parseVersion(core::versionString()).value_or(container::Version{});
    manifest.requirements.push_back({"storage", {}});
    manifest.ui = true;
    return manifest;
}

}

SOFTPHONE_PLUGIN(::softphone::plugins::directory::DirectoryPlugin,
                 ::softphone::plugins::directory::directoryManifest)
