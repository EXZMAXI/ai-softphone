// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/settings_controller.hpp>

#include <algorithm>
#include <utility>

namespace softphone::core {

SettingsController::SettingsController(ISettingsSink &sink, StoredConfiguration effective,
                                       PersistCallback persist, ChangedCallback onChanged)
    : sink_{sink}, effective_{std::move(effective)}, desired_{effective_},
      persist_{std::move(persist)}, onChanged_{std::move(onChanged)}
{}

std::vector<SettingChange> SettingsController::request(StoredConfiguration wanted,
                                                       std::span<const CallInfo> calls)
{

    wanted.schemaVersion = kCurrentSchemaVersion;

    const auto changes = settingChanges(effective_, wanted, calls);

    if (changes.empty() && wanted == desired_) {
        return {};
    }

    desired_ = std::move(wanted);
    applyWhatCan(calls);

    if (persist_) {
        persist_(desired_);
    }

    return changes;
}

void SettingsController::callsChanged(std::span<const CallInfo> calls)
{

    applyWhatCan(calls);
}

void SettingsController::withdrawPending()
{
    if (pending_.empty()) {
        return;
    }

    desired_ = effective_;
    pending_.clear();

    if (persist_) {
        persist_(desired_);
    }
    if (onChanged_) {
        onChanged_();
    }
}

void SettingsController::applyWhatCan(std::span<const CallInfo> calls)
{
    const auto before = pending_;

    std::vector<SettingChange> waiting;
    bool audio = false;
    bool ringtone = false;
    bool volumes = false;
    bool callSettings = false;

    struct AccountChange {
        std::string id;
        bool present{false};

        std::optional<AccountConfig> was;
    };

    std::vector<AccountChange> accounts;

    std::vector<std::string> owned;

    for (const auto &change : settingChanges(effective_, desired_, calls)) {
        if (change.schedule == ApplySchedule::WhenCallsEnd ||
            change.schedule == ApplySchedule::NextStart) {
            waiting.push_back(change);
            continue;
        }

        std::optional<AccountConfig> was;
        if (change.area == SettingArea::Account) {
            if (const auto *account = find(effective_, change.subject)) {
                was = *account;
            }
        }

        applyOne(change);

        switch (change.area) {
        case SettingArea::Microphone:
        case SettingArea::Speakers:
        case SettingArea::Ringer:
            audio = true;
            break;
        case SettingArea::Ringtone:
            ringtone = true;
            break;
        case SettingArea::Volumes:
            volumes = true;
            break;
        case SettingArea::Codecs:
        case SettingArea::DialAccount:
        case SettingArea::Redial:
        case SettingArea::CallWaiting:
            callSettings = true;
            break;
        case SettingArea::AccountOrder:

            break;
        case SettingArea::Account:
            accounts.push_back(AccountChange{
                change.subject, find(desired_, change.subject) != nullptr, std::move(was)});
            break;
        case SettingArea::Owned:

            owned.push_back(change.subject);
            break;
        }
    }

    for (const auto &group : owned) {
        const auto held = effective_.owned.find(group);
        sink_.applyOwned(group,
                         held == effective_.owned.end() ? std::string{} : held->second.value);
    }

    if (audio) {
        sink_.applyAudio(effective_.settings.audio);
    }
    if (ringtone) {
        sink_.applyRingtone(effective_.settings.audio.ringtone);
    }
    if (volumes) {
        sink_.applyVolumes(effective_.settings.audio.volumes);
    }
    if (callSettings) {
        sink_.applyCallSettings(effective_.settings.calls);
    }
    for (const auto &account : accounts) {
        if (account.present) {
            if (const auto *applied = find(effective_, account.id)) {
                sink_.applyAccount(*applied);
            }
        } else if (account.was) {
            sink_.removeAccount(*account.was);
        }
    }

    pending_ = std::move(waiting);

    if (pending_ != before && onChanged_) {
        onChanged_();
    }
}

void SettingsController::applyOne(const SettingChange &change)
{
    switch (change.area) {
    case SettingArea::Microphone:
        effective_.settings.audio.microphone = desired_.settings.audio.microphone;
        break;
    case SettingArea::Speakers:
        effective_.settings.audio.speakers = desired_.settings.audio.speakers;
        break;
    case SettingArea::Ringer:
        effective_.settings.audio.ringer = desired_.settings.audio.ringer;
        break;
    case SettingArea::Ringtone:
        effective_.settings.audio.ringtone = desired_.settings.audio.ringtone;
        break;
    case SettingArea::Volumes:
        effective_.settings.audio.volumes = desired_.settings.audio.volumes;
        break;
    case SettingArea::Codecs:
        effective_.settings.calls.codecs = desired_.settings.calls.codecs;
        break;
    case SettingArea::DialAccount:
        effective_.settings.calls.dialAccount = desired_.settings.calls.dialAccount;
        break;
    case SettingArea::Redial:
        effective_.settings.calls.redialIntervalSeconds =
            desired_.settings.calls.redialIntervalSeconds;
        effective_.settings.calls.redialGiveUpMinutes = desired_.settings.calls.redialGiveUpMinutes;
        break;
    case SettingArea::CallWaiting:
        effective_.settings.calls.callWaiting = desired_.settings.calls.callWaiting;
        break;
    case SettingArea::AccountOrder: {

        std::vector<AccountConfig> reordered;
        reordered.reserve(effective_.accounts.size());
        for (const auto &wanted : desired_.accounts) {
            if (const auto *account = find(effective_, wanted.id)) {
                reordered.push_back(*account);
            }
        }
        for (const auto &account : effective_.accounts) {
            if (find(desired_, account.id) == nullptr) {
                reordered.push_back(account);
            }
        }
        effective_.accounts = std::move(reordered);
        break;
    }
    case SettingArea::Account:

        if (const auto *account = find(desired_, change.subject)) {
            upsert(effective_, *account);
        } else {
            remove(effective_, change.subject);
        }

        effective_.nextAccountNumber =
            std::max(effective_.nextAccountNumber, desired_.nextAccountNumber);
        break;
    case SettingArea::Owned:

        if (const auto wanted = desired_.owned.find(change.subject);
            wanted != desired_.owned.end()) {
            effective_.owned[change.subject] = wanted->second;
        } else {
            effective_.owned.erase(change.subject);
        }
        break;
    }
}

}
