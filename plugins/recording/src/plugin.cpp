// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/recording/plugin.hpp>

#include <softphone/container/loading.hpp>

#include <softphone/core/version.hpp>

#include <softphone/plugins/mediastorage/plugin.hpp>
#include <softphone/plugins/softphone/plugin.hpp>

#include <memory>
#include <string>
#include <utility>

namespace softphone::plugins::recording {

bool RecordingPlugin::initialize(container::PluginContext &context, std::string &problem)
{

    if (context.find<core::IRecordingStore>(mediastorage::kRecordingStore) == nullptr) {
        problem = "the recordings library is not there";
        return false;
    }

    auto *const engine = context.find<plugins::softphone::EngineHost>(plugins::softphone::kEngine);
    if (engine == nullptr) {

        return true;
    }

    gate_ = &engine->recording();
    engine_ = engine;

    engine->armRecording(true);

    if (!context.offer<core::RecordingController>(std::string{kController},
                                                  container::Version{1, 0, 0}, gate_)) {
        problem = "something else already offers " + std::string{kController};
        return false;
    }

    consent_ = std::make_unique<ConsentPolicy>(*gate_);

    if (!context.offer<IConsentPolicy>(std::string{kConsent}, container::Version{1, 0, 0},
                                       consent_.get())) {
        problem = "something else already offers " + std::string{kConsent};
        return false;
    }

    engine->post([consent = consent_.get(), lines = engine->storedConfiguration().accounts]() {
        consent->accountsChanged(lines);
    });

    return true;
}

void RecordingPlugin::leaving() noexcept
{

    if (engine_ != nullptr) {
        engine_->armRecording(false);
    }
}

container::Manifest recordingManifest()
{
    container::Manifest manifest;
    manifest.id = std::string{kOwner};
    manifest.name = "Recording";
    manifest.summary = "Recording calls, and telling the other party about it";

    manifest.version =
        container::parseVersion(core::versionString()).value_or(container::Version{});

    manifest.requirements.push_back({std::string{plugins::softphone::kOwner}, {}});
    manifest.requirements.push_back({std::string{mediastorage::kOwner}, {}});

    manifest.required = false;

    manifest.ui = true;
    return manifest;
}

}

SOFTPHONE_PLUGIN(::softphone::plugins::recording::RecordingPlugin,
                 ::softphone::plugins::recording::recordingManifest)
