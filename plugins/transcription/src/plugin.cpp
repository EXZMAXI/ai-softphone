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
#include <softphone/plugins/storage/storage_access.hpp>
#include <softphone/plugins/transcription/plugin.hpp>

#include <softphone/container/loading.hpp>

#include <softphone/core/version.hpp>

namespace softphone::plugins::transcription {

bool TranscriptionPlugin::initialize(container::PluginContext &context, std::string &problem)
{
    auto *const database = context.find<storage::IStorageAccess>(storage::kStorageAccess);
    if (database == nullptr) {
        problem = "the storage service is not there";
        return false;
    }

    database->declareCollections(std::string{kOwner}, kCollectionsVersion,
                                 std::string{collectionsDdl()});

    transcripts_ = database->transcripts();
    providers_ = database->providers();
    jobs_ = database->jobs();
    if (transcripts_ == nullptr || providers_ == nullptr || jobs_ == nullptr) {
        problem = "the store offers no transcript, provider or job repository";
        return false;
    }

    const auto offered =
        context.offer<core::ITranscriptStore>(std::string{kTranscriptStore},
                                              container::Version{1, 0, 0}, transcripts_) &&
        context.offer<core::IProviderStore>(std::string{kProviderStore},
                                            container::Version{1, 0, 0}, providers_) &&
        context.offer<core::IJobStore>(std::string{kJobStore}, container::Version{1, 0, 0}, jobs_);
    if (!offered) {
        problem = "something else already offers one of the transcription services";
        return false;
    }
    return true;
}

container::Manifest transcriptionManifest()
{
    container::Manifest manifest;
    manifest.id = std::string{kOwner};
    manifest.version =
        container::parseVersion(core::versionString()).value_or(container::Version{});
    manifest.name = "Transcription";
    manifest.summary = "Recognisers, the queue that drives them, and the transcripts they produce";

    manifest.requirements.push_back({std::string{mediastorage::kOwner}, {}});

    manifest.suggestions.push_back({"taxonomy", {}});

    manifest.required = false;
    manifest.ui = true;
    return manifest;
}

}

SOFTPHONE_PLUGIN(::softphone::plugins::transcription::TranscriptionPlugin,
                 ::softphone::plugins::transcription::transcriptionManifest)
