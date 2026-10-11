// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/storage/plugin.hpp>

#include <softphone/core/version.hpp>
#include <softphone/storage/sqlite_store.hpp>

#include <utility>

namespace softphone::plugins::storage {

StoragePlugin::StoragePlugin(std::filesystem::path file, std::string contentLanguage,
                             std::function<void(core::StoreStatus)> opened)
    : file_{std::move(file)}, contentLanguage_{std::move(contentLanguage)},
      opened_{std::move(opened)}
{}

StoragePlugin::~StoragePlugin() = default;

bool StoragePlugin::initialize(container::PluginContext &context, std::string &problem)
{
    host_.setContentLanguage(contentLanguage_);
    host_.start(file_.empty() ? ::softphone::storage::defaultDatabaseFile() : file_, opened_);

    if (!context.offer<IStorageAccess>(std::string{kStorageAccess}, container::Version{1, 0, 0},
                                       &host_)) {
        problem = "something else already offers " + std::string{kStorageAccess};
        return false;
    }
    return true;
}

void StoragePlugin::leaving() noexcept
{
    host_.stop();
}

container::Manifest storageManifest()
{
    container::Manifest manifest;
    manifest.id = "storage";
    manifest.version =
        container::parseVersion(core::versionString()).value_or(container::Version{});

    manifest.name = "Local database";
    manifest.summary = "The database everything that is kept is kept in";

    manifest.required = true;
    manifest.ui = false;
    return manifest;
}

}
