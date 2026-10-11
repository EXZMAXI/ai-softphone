// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/storage/storage_access.hpp>
#include <softphone/plugins/taxonomy/plugin.hpp>

#include <softphone/container/loading.hpp>

#include <softphone/core/version.hpp>
#include <softphone/plugins/taxonomy/events.hpp>

namespace softphone::plugins::taxonomy {

bool TaxonomyPlugin::initialize(container::PluginContext &context, std::string &problem)
{
    auto *const database = context.find<storage::IStorageAccess>(storage::kStorageAccess);
    if (database == nullptr) {

        problem = "the storage service is not there";
        return false;
    }

    database->declareCollections(std::string{kOwner}, kCollectionsVersion,
                                 std::string{collectionsDdl()});
    database->seedShippedDictionaries();

    context_ = &context;
    store_ = database->taxonomy();
    if (store_ == nullptr) {
        problem = "the store offers no dictionary repository";
        return false;
    }

    if (!context.offer<core::ITaxonomyStore>(std::string{kTaxonomyStore},
                                             container::Version{1, 0, 0}, store_)) {
        problem = "something else already offers " + std::string{kTaxonomyStore};
        return false;
    }

    if (!context.offer<IDictionaryAnnouncer>(std::string{kAnnouncer}, container::Version{1, 0, 0},
                                             static_cast<IDictionaryAnnouncer *>(this))) {
        problem = "something else already offers " + std::string{kAnnouncer};
        return false;
    }
    return true;
}

void TaxonomyPlugin::announceChange()
{
    if (context_ != nullptr) {
        context_->publish(events::DictionariesChanged{});
    }
}

container::Manifest taxonomyManifest()
{
    container::Manifest manifest;
    manifest.id = std::string{kOwner};
    manifest.version =
        container::parseVersion(core::versionString()).value_or(container::Version{});
    manifest.name = "Dictionaries";
    manifest.summary = "Categories, tags and red flags — the codes everything else points at";

    manifest.requirements.push_back({"storage", {}});

    manifest.required = false;
    manifest.ui = true;
    return manifest;
}

}

SOFTPHONE_PLUGIN(::softphone::plugins::taxonomy::TaxonomyPlugin,
                 ::softphone::plugins::taxonomy::taxonomyManifest)
