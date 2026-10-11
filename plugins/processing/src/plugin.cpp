// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/processing/plugin.hpp>

#include <softphone/container/loading.hpp>
#include <softphone/plugins/storage/storage_access.hpp>
#include <softphone/plugins/transcription/plugin.hpp>

#include <softphone/core/version.hpp>

namespace softphone::plugins::processing {

bool ProcessingPlugin::initialize(container::PluginContext &context, std::string &problem)
{
    auto *const database = context.find<storage::IStorageAccess>(storage::kStorageAccess);
    if (database == nullptr) {
        problem = "the storage service is not there";
        return false;
    }

    database->declareCollections(std::string{kOwner}, kCollectionsVersion,
                                 std::string{collectionsDdl()});
    database->seedShippedPrompts();

    templates_ = database->templates();
    automations_ = database->automations();
    artifacts_ = database->artifacts();
    usage_ = database->usage();
    if (templates_ == nullptr || automations_ == nullptr || artifacts_ == nullptr ||
        usage_ == nullptr) {
        problem = "the store offers no template, rule, artefact or ledger repository";
        return false;
    }

    const auto offered =
        context.offer<core::ITemplateStore>(std::string{kTemplateStore},
                                            container::Version{1, 0, 0}, templates_) &&
        context.offer<core::IAutomationStore>(std::string{kAutomationStore},
                                              container::Version{1, 0, 0}, automations_) &&
        context.offer<core::IArtifactStore>(std::string{kArtifactStore},
                                            container::Version{1, 0, 0}, artifacts_) &&
        context.offer<core::IUsageStore>(std::string{kUsageStore}, container::Version{1, 0, 0},
                                         usage_);
    if (!offered) {
        problem = "something else already offers one of the processing services";
        return false;
    }
    return true;
}

container::Manifest processingManifest()
{
    container::Manifest manifest;
    manifest.id = std::string{kOwner};
    manifest.version =
        container::parseVersion(core::versionString()).value_or(container::Version{});
    manifest.name = "Processing";
    manifest.summary = "Prompts, the rules that fire them, what a model answers, and what it cost";

    manifest.requirements.push_back({"storage", {}});

    manifest.requirements.push_back({std::string{transcription::kOwner}, {}});

    manifest.suggestions.push_back({"taxonomy", {}});

    manifest.required = false;
    manifest.ui = true;
    return manifest;
}

}

SOFTPHONE_PLUGIN(::softphone::plugins::processing::ProcessingPlugin,
                 ::softphone::plugins::processing::processingManifest)
