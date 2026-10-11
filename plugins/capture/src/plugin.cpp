// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/capture/plugin.hpp>

#include <softphone/container/loading.hpp>

#include <softphone/core/version.hpp>

#include <softphone/media/mp3.hpp>
#include <softphone/plugins/mediastorage/plugin.hpp>

#include <softphone/capture/applications.hpp>
#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

namespace softphone::plugins::capture {

bool CapturePlugin::initialize(container::PluginContext &context, std::string &problem)
{
    auto *const store = context.find<core::IRecordingStore>(mediastorage::kRecordingStore);
    if (store == nullptr) {
        problem = "the recordings library is not there";
        return false;
    }

    backend_ = softphone::capture::makeSystemCapture();

    const auto ours = core::configDirectory() / "applications.txt";
    std::error_code wrong;
    if (std::filesystem::exists(ours, wrong) && !wrong) {
        std::ifstream file{ours};
        if (file.is_open()) {
            std::ostringstream text;
            text << file.rdbuf();
            auto read = softphone::capture::readApplications(text.str());
            if (!read.known.empty() || !read.never.empty()) {
                core::Logger::instance().log(
                    core::LogLevel::Info, "capture",
                    "using the table of applications in " + core::asUtf8(ours) + ": " +
                        std::to_string(read.known.size()) + " named, " +
                        std::to_string(read.never.size()) + " never a conversation");
                softphone::capture::useApplications(std::move(read));
            } else {

                core::Logger::instance().log(core::LogLevel::Warning, "capture",
                                             "there is a table of applications in " +
                                                 core::asUtf8(ours) +
                                                 " and nothing in it could be read; keeping the "
                                                 "one this build ships");
            }
        }
    }

    controller_ = std::make_unique<CaptureController>(
        backend_.get(),
        [](const std::filesystem::path &file,
           const core::EncoderSettings &format) -> std::unique_ptr<core::IAudioEncoder> {
            auto encoder = std::make_unique<media::Mp3Encoder>();
            if (!core::succeeded(encoder->open(file, format))) {
                return nullptr;
            }
            return encoder;
        },
        core::recordingsDirectory(), store);

    if (!context.offer<ICaptureControl>(std::string{kController}, container::Version{1, 0, 0},
                                        controller_.get())) {
        problem = "something else already offers " + std::string{kController};
        return false;
    }

    return true;
}

void CapturePlugin::leaving() noexcept
{

    controller_.reset();
    backend_.reset();
}

container::Manifest captureManifest()
{
    container::Manifest manifest;
    manifest.id = std::string{kOwner};
    manifest.name = "Capture";
    manifest.summary = "Recording a conversation happening in another application";

    manifest.version =
        container::parseVersion(core::versionString()).value_or(container::Version{});

    manifest.requirements.push_back({std::string{mediastorage::kOwner}, {}});

    manifest.required = false;
    manifest.ui = true;
    return manifest;
}

}

SOFTPHONE_PLUGIN(::softphone::plugins::capture::CapturePlugin,
                 ::softphone::plugins::capture::captureManifest)
