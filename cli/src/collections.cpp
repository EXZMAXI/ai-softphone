// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "collections.hpp"

#include <softphone/plugins/directory/plugin.hpp>
#include <softphone/plugins/mediastorage/plugin.hpp>
#include <softphone/plugins/processing/plugin.hpp>
#include <softphone/plugins/softphone/plugin.hpp>
#include <softphone/plugins/taxonomy/plugin.hpp>
#include <softphone/plugins/transcription/plugin.hpp>
#include <softphone/storage/sqlite_store.hpp>

#include <string>

namespace softphone::cli {

void declarePluginCollections(storage::SqliteStore &store)
{

    static_cast<void>(store.declareCollections(std::string{plugins::softphone::kOwner},
                                               plugins::softphone::kCollectionsVersion,
                                               std::string{plugins::softphone::collectionsDdl()}));
    static_cast<void>(store.declareCollections(std::string{plugins::directory::kOwner},
                                               plugins::directory::kCollectionsVersion,
                                               std::string{plugins::directory::collectionsDdl()}));
    static_cast<void>(store.declareCollections(
        std::string{plugins::mediastorage::kOwner}, plugins::mediastorage::kCollectionsVersion,
        std::string{plugins::mediastorage::collectionsDdl()}));
    static_cast<void>(store.declareCollections(std::string{plugins::taxonomy::kOwner},
                                               plugins::taxonomy::kCollectionsVersion,
                                               std::string{plugins::taxonomy::collectionsDdl()}));
    static_cast<void>(store.declareCollections(
        std::string{plugins::transcription::kOwner}, plugins::transcription::kCollectionsVersion,
        std::string{plugins::transcription::collectionsDdl()}));
    static_cast<void>(store.declareCollections(std::string{plugins::processing::kOwner},
                                               plugins::processing::kCollectionsVersion,
                                               std::string{plugins::processing::collectionsDdl()}));
}

}
