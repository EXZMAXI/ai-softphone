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

#include <softphone/core/settings.hpp>

#include <functional>
#include <span>
#include <string>
#include <vector>

namespace softphone::core {

class ISettingsSink {
public:
    ISettingsSink() = default;
    ISettingsSink(const ISettingsSink &) = delete;
    ISettingsSink &operator=(const ISettingsSink &) = delete;
    ISettingsSink(ISettingsSink &&) = delete;
    ISettingsSink &operator=(ISettingsSink &&) = delete;
    virtual ~ISettingsSink() = default;

    virtual void applyAudio(const AudioSettings &audio) = 0;

    virtual void applyRingtone(const RingtoneChoice &ringtone) = 0;

    virtual void applyVolumes(const Volumes &volumes) = 0;
    virtual void applyCallSettings(const CallSettings &calls) = 0;

    virtual void applyAccount(const AccountConfig &account) = 0;

    virtual void removeAccount(const AccountConfig &account) = 0;

    virtual void applyOwned(const std::string &group, const std::string &value) = 0;
};

class SettingsController {
public:

    using PersistCallback = std::function<void(const StoredConfiguration &)>;

    using ChangedCallback = std::function<void()>;

    SettingsController(ISettingsSink &sink, StoredConfiguration effective,
                       PersistCallback persist = {}, ChangedCallback onChanged = {});

    SettingsController(const SettingsController &) = delete;
    SettingsController &operator=(const SettingsController &) = delete;
    SettingsController(SettingsController &&) = delete;
    SettingsController &operator=(SettingsController &&) = delete;
    ~SettingsController() = default;

    [[nodiscard]] const StoredConfiguration &effective() const noexcept
    {
        return effective_;
    }

    [[nodiscard]] const StoredConfiguration &desired() const noexcept
    {
        return desired_;
    }

    [[nodiscard]] const std::vector<SettingChange> &pending() const noexcept
    {
        return pending_;
    }

    std::vector<SettingChange> request(StoredConfiguration wanted, std::span<const CallInfo> calls);

    void callsChanged(std::span<const CallInfo> calls);

    void withdrawPending();

private:
    void applyWhatCan(std::span<const CallInfo> calls);
    void applyOne(const SettingChange &change);

    ISettingsSink &sink_;
    StoredConfiguration effective_;
    StoredConfiguration desired_;
    std::vector<SettingChange> pending_;
    PersistCallback persist_;
    ChangedCallback onChanged_;
};

}
