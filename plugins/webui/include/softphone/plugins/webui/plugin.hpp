// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#pragma once

#include <softphone/container/manifest.hpp>
#include <softphone/container/plugin.hpp>
#include <softphone/core/api.hpp>
#include <softphone/core/recordings.hpp>
#include <softphone/core/transcripts.hpp>
#include <softphone/plugins/webui/words.hpp>

#include <atomic>
#include <filesystem>
#include <string_view>
#include <utility>

namespace softphone::plugins::webui {

class IPageLanguage {
public:
    IPageLanguage() = default;
    IPageLanguage(const IPageLanguage &) = delete;
    IPageLanguage &operator=(const IPageLanguage &) = delete;
    IPageLanguage(IPageLanguage &&) = delete;
    IPageLanguage &operator=(IPageLanguage &&) = delete;
    virtual ~IPageLanguage() = default;

    virtual void speak(std::string_view tag) = 0;
};

class WebUiPlugin final : public container::IPlugin, public IPageLanguage {
public:
    bool initialize(container::PluginContext &context, std::string &problem) override;
    bool ready(container::PluginContext &context, std::string &problem) override;

    void leaving() noexcept override;

    void serveSoundFrom(std::filesystem::path root)
    {
        soundRoot_ = std::move(root);
    }

    void speak(std::string_view tag) override
    {
        phone_.store(wordsIn(tag), std::memory_order_relaxed);
    }

private:

    [[nodiscard]] const Words &speakingTo(const core::ApiRequest &request) const
    {
        return wordsFor(request.header("Accept-Language"), phone_.load(std::memory_order_relaxed));
    }

    [[nodiscard]] std::filesystem::path soundRoot() const;

    [[nodiscard]] core::IRecordingStore *recordings() const;
    [[nodiscard]] core::ITranscriptStore *readings() const;

    container::ServiceRegistry *services_{nullptr};

    core::ApiRouter *routes_{nullptr};

    std::filesystem::path soundRoot_;

    std::atomic<const Words *> phone_{nullptr};
};

[[nodiscard]] container::Manifest webUiManifest();

inline constexpr std::string_view kOwner = "webui";

inline constexpr std::string_view kPageLanguage = "webui.language";

inline constexpr std::size_t kListLength = 500;

}
