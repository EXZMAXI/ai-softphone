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

#include "autostart.hpp"
#include "menu_entry.hpp"
#include <softphone/core/audio_devices.hpp>
#include <softphone/core/call_controller.hpp>
#include <softphone/core/codecs.hpp>
#include <softphone/core/engine.hpp>
#include <softphone/core/redial.hpp>

#include <softphone/core/settings_controller.hpp>

#include <array>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace softphone::ui {

struct AudioReport {
    std::vector<core::AudioDeviceInfo> devices;
    core::AudioSettings chosen;
    core::AudioApplyResult result;
};

struct CodecReport {
    std::vector<core::CodecInfo> available;
    std::vector<core::CodecChoice> chosen;

    core::CallSettings calls;
};

class ShellSettingsSink final : public core::ISettingsSink {
public:

    using AudioReporter = std::function<void(AudioReport)>;

    using CodecReporter = std::function<void(CodecReport)>;

    using AppearanceReporter = std::function<void(core::AppearanceSettings)>;

    using SecretForgetter = std::function<void(const core::AccountConfig &account)>;

    using StartupReporter = std::function<void(core::StartupSettings)>;

    using HotkeysReporter = std::function<void(core::HotkeySettings)>;

    ShellSettingsSink(core::IAudioDeviceBackend &audio, core::ICodecBackend &codecs,
                      core::Engine &engine, core::CallController &calls,
                      core::RedialController &redial, AudioReporter report = {},
                      CodecReporter reportCodecs = {}, AppearanceReporter reportAppearance = {},
                      SecretForgetter forgetSecret = {}, StartupReporter reportStartup = {},
                      IAutostart *autostart = nullptr, HotkeysReporter reportHotkeys = {});

    using DiagnosticsReporter = std::function<void(core::DiagnosticsSettings)>;
    void setDiagnosticsReporter(DiagnosticsReporter report);

    void setMenuEntry(IMenuEntry *entry);

    using WebhooksReporter = std::function<void(core::WebhookSettings)>;
    void setWebhooksReporter(WebhooksReporter report);

    using RecordingReporter = std::function<void(core::RecordingSettings)>;
    void setRecordingReporter(RecordingReporter report);

    using ButtonsReporter = std::function<void(core::ButtonSettings)>;
    void setButtonsReporter(ButtonsReporter report);

    using ApiReporter = std::function<void(core::ApiSettings)>;
    void setApiReporter(ApiReporter report);

    using UpdateReporter = std::function<void(core::UpdateSettings)>;
    void setUpdateReporter(UpdateReporter report);

    void applyAudio(const core::AudioSettings &audio) override;

    void reapplyAudio(const core::AudioSettings &audio);

    void forgetTheAudio();
    void applyRingtone(const core::RingtoneChoice &ringtone) override;
    void applyVolumes(const core::Volumes &volumes) override;
    void applyCallSettings(const core::CallSettings &calls) override;
    void applyAccount(const core::AccountConfig &account) override;
    void removeAccount(const core::AccountConfig &account) override;

    void applyOwned(const std::string &group, const std::string &value) override;

    void applyButtons(const core::ButtonSettings &buttons);
    void applyAppearance(const core::AppearanceSettings &appearance);
    void applyStartup(const core::StartupSettings &startup);
    void applyHotkeys(const core::HotkeySettings &hotkeys);
    void applyDiagnostics(const core::DiagnosticsSettings &diagnostics);
    void applyWebhooks(const core::WebhookSettings &webhooks);
    void applyRecording(const core::RecordingSettings &recording);
    void applyApi(const core::ApiSettings &api);
    void applyUpdates(const core::UpdateSettings &updates);

    void ownedBy(const std::string &group, std::function<void(const std::string &)> apply);

    void publish();

    void republish();

private:

    std::map<std::string, std::function<void(const std::string &)>> owners_;

    core::IAudioDeviceBackend &audio_;
    core::ICodecBackend &codecs_;
    core::Engine &engine_;
    core::CallController &calls_;
    core::RedialController &redial_;
    ButtonsReporter reportButtons_;
    AudioReporter report_;
    CodecReporter reportCodecs_;
    AppearanceReporter reportAppearance_;
    StartupReporter reportStartup_;
    IAutostart *autostart_{nullptr};
    IMenuEntry *menuEntry_{nullptr};
    HotkeysReporter reportHotkeys_;
    DiagnosticsReporter reportDiagnostics_;
    WebhooksReporter reportWebhooks_;
    RecordingReporter reportRecording_;
    ApiReporter reportApi_;
    UpdateReporter reportUpdates_;
    SecretForgetter forgetSecret_;

    std::vector<core::AudioDeviceInfo> devices_;
    core::AudioSettings chosen_;
    core::AudioApplyResult result_;

    std::array<std::string, 2> defaultWas_;

    core::AudioSettings applied_;
    bool everApplied_{false};
};

}
