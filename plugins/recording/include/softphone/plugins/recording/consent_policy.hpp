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

#include <softphone/core/account_config.hpp>
#include <softphone/core/recording_controller.hpp>
#include <softphone/core/settings.hpp>

#include <map>
#include <string>
#include <vector>

namespace softphone::plugins::recording {

class IConsentPolicy {
public:
    IConsentPolicy() = default;
    IConsentPolicy(const IConsentPolicy &) = delete;
    IConsentPolicy &operator=(const IConsentPolicy &) = delete;
    IConsentPolicy(IConsentPolicy &&) = delete;
    IConsentPolicy &operator=(IConsentPolicy &&) = delete;
    virtual ~IConsentPolicy() = default;

    virtual void apply(const core::RecordingSettings &settings, std::string noticeFile,
                       std::string toneFile, std::string shippedNotice) = 0;

    virtual void accountsChanged(std::vector<core::AccountConfig> accounts) = 0;
};

class ConsentPolicy final : public IConsentPolicy {
public:

    explicit ConsentPolicy(core::RecordingController &gate) noexcept : gate_{gate} {}

    void apply(const core::RecordingSettings &settings, std::string noticeFile,
               std::string toneFile, std::string shippedNotice) override;

    void accountsChanged(std::vector<core::AccountConfig> accounts) override;

    [[nodiscard]] const core::RecordingController::ConsentPlan &plan() const noexcept
    {
        return consent_;
    }

    [[nodiscard]] std::map<std::string, core::RecordingController::AccountRecording>
    perLine() const;

private:
    void refresh();

    core::RecordingController &gate_;

    core::RecordingSettings settings_;
    std::string noticeFile_;
    std::string toneFile_;
    std::string shippedNotice_;

    core::RecordingController::ConsentPlan consent_;
    std::vector<core::AccountConfig> accounts_;
};

}
