// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/webui/plugin.hpp>

#include <softphone/container/loading.hpp>
#include <softphone/core/api.hpp>
#include <softphone/core/paths.hpp>
#include <softphone/core/version.hpp>
#include <softphone/plugins/integration/events.hpp>
#include <softphone/plugins/integration/plugin.hpp>
#include <softphone/plugins/mediastorage/plugin.hpp>
#include <softphone/plugins/transcription/plugin.hpp>
#include <softphone/plugins/webui/pages.hpp>

#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::plugins::webui {
namespace {

constexpr auto kHtml = "text/html; charset=utf-8";

[[nodiscard]] std::string soundType(std::string_view format)
{
    if (format == "mp3") {
        return "audio/mpeg";
    }
    if (format == "wav") {
        return "audio/wav";
    }
    if (format == "ogg" || format == "opus") {
        return "audio/ogg";
    }
    if (format == "m4a" || format == "aac") {
        return "audio/mp4";
    }
    if (format == "flac") {
        return "audio/flac";
    }
    return "application/octet-stream";
}

[[nodiscard]] core::ApiResponse html(std::string body, int status = 200)
{
    core::ApiResponse response;
    response.status = status;
    response.contentType = kHtml;
    response.body = std::move(body);
    return response;
}

}

bool WebUiPlugin::initialize(container::PluginContext &context, std::string &problem)
{

    services_ = &context.services();

    if (!context.offer<IPageLanguage>(std::string{kPageLanguage}, container::Version{1, 0, 0},
                                      this)) {
        problem = "the page language is already offered by something else";
        return false;
    }
    return true;
}

core::IRecordingStore *WebUiPlugin::recordings() const
{
    return services_ == nullptr
               ? nullptr
               : services_->find<core::IRecordingStore>(mediastorage::kRecordingStore);
}

std::filesystem::path WebUiPlugin::soundRoot() const
{
    return soundRoot_.empty() ? core::recordingsDirectory() : soundRoot_;
}

core::ITranscriptStore *WebUiPlugin::readings() const
{
    return services_ == nullptr
               ? nullptr
               : services_->find<core::ITranscriptStore>(transcription::kTranscriptStore);
}

bool WebUiPlugin::ready(container::PluginContext &context, std::string &problem)
{
    static_cast<void>(problem);

    auto *const routes = context.find<core::ApiRouter>(integration::kRouteRegistry);
    if (routes == nullptr) {

        return true;
    }

    routes_ = routes;

    routes->add(core::ApiRoute{"GET", "/ui", core::ApiAccess::Read, std::string{kOwner},
                               "The recordings, as a page a browser can open"},
                [this](const core::ApiRequest &request) {

                    const auto &words = speakingTo(request);

                    auto *const library = recordings();
                    if (library == nullptr) {
                        return html(unavailablePage(words, words.noLibrary), 503);
                    }

                    core::RecordingQuery query;
                    query.limit = kListLength;

                    std::vector<core::Recording> found;
                    if (library->recordings(query, found) != core::StoreStatus::Ok) {
                        return html(unavailablePage(words, words.couldNotRead), 503);
                    }
                    return html(listPage(found, words));
                });

    routes->add(core::ApiRoute{"GET", "/ui/recordings/{id}", core::ApiAccess::Read,
                               std::string{kOwner}, "One recording and its transcript, as a page"},
                [this](const core::ApiRequest &request) {
                    const auto &words = speakingTo(request);

                    auto *const library = recordings();
                    if (library == nullptr) {
                        return html(unavailablePage(words, words.noLibrary), 503);
                    }

                    core::Recording recording;
                    if (library->recording(request.id, recording) != core::StoreStatus::Ok) {
                        return html(missingPage(words), 404);
                    }

                    std::optional<core::Transcript> reading;
                    if (auto *const transcripts = readings(); transcripts != nullptr) {
                        core::Transcript found;
                        if (transcripts->primaryTranscript(recording.id, found) ==
                            core::StoreStatus::Ok) {
                            reading = std::move(found);
                        }
                    }
                    return html(recordingPage(words, recording, reading));
                });

    routes->add(
        core::ApiRoute{"GET", "/ui/recordings/{id}/audio", core::ApiAccess::Read,
                       std::string{kOwner}, "The sound of one recording, for the page above"},
        [this](const core::ApiRequest &request) {
            const auto &words = speakingTo(request);

            auto *const library = recordings();
            if (library == nullptr) {
                return html(unavailablePage(words, words.noLibrary), 503);
            }

            core::Recording recording;
            if (library->recording(request.id, recording) != core::StoreStatus::Ok ||
                recording.filePath.empty()) {
                return html(missingPage(words), 404);
            }

            const auto file = soundRoot() / core::fromUtf8(recording.filePath);

            std::ifstream sound{file, std::ios::binary};
            if (!sound) {

                return html(unavailablePage(words, words.soundGone), 404);
            }

            sound.seekg(0, std::ios::end);
            const auto size =
                static_cast<std::uint64_t>(std::max<std::streamoff>(0, sound.tellg()));

            core::ApiResponse response;
            response.contentType = soundType(recording.format);
            response.size = size;

            response.headers.emplace_back("Accept-Ranges", "bytes");

            response.reader = [file](std::uint64_t offset, std::size_t length, std::string &out) {
                std::ifstream piece{file, std::ios::binary};
                if (!piece) {
                    return false;
                }
                piece.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
                out.resize(length);
                piece.read(out.data(), static_cast<std::streamsize>(length));
                out.resize(static_cast<std::size_t>(std::max<std::streamsize>(0, piece.gcount())));
                return !out.empty();
            };
            return response;
        });

    return true;
}

void WebUiPlugin::leaving() noexcept
{

    if (routes_ != nullptr) {
        routes_->removeRoutesOf(core::ApiResource{kOwner});
        routes_ = nullptr;
    }
}

container::Manifest webUiManifest()
{
    container::Manifest manifest;
    manifest.id = std::string{kOwner};
    manifest.version =
        container::parseVersion(core::versionString()).value_or(container::Version{});

    manifest.name = "Browser interface";
    manifest.summary = "The recordings and their transcripts, opened in a browser";

    manifest.requirements.push_back({std::string{integration::kOwner}, {}});
    manifest.suggestions.push_back({std::string{mediastorage::kOwner}, {}});
    manifest.suggestions.push_back({std::string{transcription::kOwner}, {}});

    manifest.required = false;

    manifest.ui = true;
    return manifest;
}

}

SOFTPHONE_PLUGIN(::softphone::plugins::webui::WebUiPlugin,
                 ::softphone::plugins::webui::webUiManifest)
