// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/recording/consent_policy.hpp>

#include <chrono>
#include <utility>

namespace softphone::plugins::recording {

void ConsentPolicy::apply(const core::RecordingSettings &settings, std::string noticeFile,
                          std::string toneFile, std::string shippedNotice)
{
    settings_ = settings;
    noticeFile_ = std::move(noticeFile);
    toneFile_ = std::move(toneFile);
    shippedNotice_ = std::move(shippedNotice);

    core::RecordingController::ConsentPlan plan;
    plan.posture = settings_.consent;
    plan.announcement = noticeFile_;
    plan.shippedAnnouncement = shippedNotice_;
    plan.tone = toneFile_;
    plan.interval = std::chrono::seconds{settings_.toneIntervalSeconds};
    plan.keepInTheRecording = settings_.keepNotices;

    consent_ = plan;

    gate_.setConsent(plan);
    gate_.setKeepStems(settings_.keepStems);

    refresh();
}

void ConsentPolicy::accountsChanged(std::vector<core::AccountConfig> accounts)
{
    accounts_ = std::move(accounts);
    refresh();
}

std::map<std::string, core::RecordingController::AccountRecording> ConsentPolicy::perLine() const
{
    std::map<std::string, core::RecordingController::AccountRecording> policies;
    for (const core::AccountConfig &account : accounts_) {
        core::RecordingController::AccountRecording line;

        switch (core::recordingPolicyFor(settings_, account.id)) {
        case core::RecordingPolicy::Always:
            line.when = core::RecordingController::WhenToRecord::Always;
            break;
        case core::RecordingPolicy::Ask:
            line.when = core::RecordingController::WhenToRecord::Ask;
            break;
        case core::RecordingPolicy::Never:
        case core::RecordingPolicy::OnTheseAccounts:

            line.when = core::RecordingController::WhenToRecord::Never;
            break;
        }

        line.consent = consent_;
        line.consent.posture = core::consentFor(settings_, account);

        policies.emplace(account.id, std::move(line));
    }

    return policies;
}

void ConsentPolicy::refresh()
{

    gate_.setAccountPolicies(perLine());
}

}
