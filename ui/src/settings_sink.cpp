// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "settings_sink.hpp"

#include <softphone/core/settings_groups.hpp>

#include <softphone/core/logging.hpp>

#include <string>
#include <utility>

namespace softphone::ui {

ShellSettingsSink::ShellSettingsSink(core::IAudioDeviceBackend &audio, core::ICodecBackend &codecs,
                                     core::Engine &engine, core::CallController &calls,
                                     core::RedialController &redial, AudioReporter report,
                                     CodecReporter reportCodecs,
                                     AppearanceReporter reportAppearance,
                                     SecretForgetter forgetSecret, StartupReporter reportStartup,
                                     IAutostart *autostart, HotkeysReporter reportHotkeys)
    : audio_{audio}, codecs_{codecs}, engine_{engine}, calls_{calls}, redial_{redial},
      report_{std::move(report)}, reportCodecs_{std::move(reportCodecs)},
      reportAppearance_{std::move(reportAppearance)}, reportStartup_{std::move(reportStartup)},
      autostart_{autostart}, reportHotkeys_{std::move(reportHotkeys)},
      forgetSecret_{std::move(forgetSecret)}
{

    audio_.setDeviceLostCallback([this](core::DeviceRole) { applyAudio(chosen_); });

    audio_.setDevicesChangedCallback([this]() { republish(); });

    ownedBy("buttons", [this](const std::string &text) { applyButtons(core::buttonsFrom(text)); });
    ownedBy("appearance",
            [this](const std::string &text) { applyAppearance(core::appearanceFrom(text)); });
    ownedBy("startup", [this](const std::string &text) { applyStartup(core::startupFrom(text)); });
    ownedBy("hotkeys", [this](const std::string &text) { applyHotkeys(core::hotkeysFrom(text)); });
    ownedBy("diagnostics",
            [this](const std::string &text) { applyDiagnostics(core::diagnosticsFrom(text)); });
    ownedBy("webhooks",
            [this](const std::string &text) { applyWebhooks(core::webhooksFrom(text)); });
    ownedBy("recording",
            [this](const std::string &text) { applyRecording(core::recordingFrom(text)); });
    ownedBy("api", [this](const std::string &text) { applyApi(core::apiFrom(text)); });
    ownedBy("updates", [this](const std::string &text) { applyUpdates(core::updatesFrom(text)); });
}

void ShellSettingsSink::reapplyAudio(const core::AudioSettings &audio)
{
    if (everApplied_ && applied_ == audio) {

        return;
    }
    applyAudio(audio);
}

void ShellSettingsSink::forgetTheAudio()
{
    everApplied_ = false;
}

void ShellSettingsSink::applyAudio(const core::AudioSettings &audio)
{
    chosen_ = audio;
    applied_ = audio;
    everApplied_ = true;
    result_ = audio_.applyAudio(audio);

    defaultWas_[0] = audio_.systemDefaultDevice(core::DeviceRole::Capture);
    defaultWas_[1] = audio_.systemDefaultDevice(core::DeviceRole::Playback);

    calls_.audioDeviceChanged();

    audio_.applyVolumes(audio.volumes);

    if (devices_.empty()) {
        devices_ = audio_.devices();
    }

    publish();
}

void ShellSettingsSink::applyRingtone(const core::RingtoneChoice &ringtone)
{

    chosen_.ringtone = ringtone;
    publish();
}

void ShellSettingsSink::applyVolumes(const core::Volumes &volumes)
{

    chosen_.volumes = volumes;
    audio_.applyVolumes(volumes);
    publish();
}

void ShellSettingsSink::applyCallSettings(const core::CallSettings &calls)
{

    const auto available = codecs_.codecs();
    const auto planned = core::planCodecs(calls.codecs, available);
    codecs_.applyCodecs(planned);

    redial_.setPolicy(core::RedialPolicy{std::chrono::seconds{calls.redialIntervalSeconds},
                                         std::chrono::minutes{calls.redialGiveUpMinutes}});

    calls_.setCallWaiting(calls.callWaiting);

    if (reportCodecs_) {
        reportCodecs_(CodecReport{available, planned, calls});
    }
}

void ShellSettingsSink::setButtonsReporter(ButtonsReporter report)
{
    reportButtons_ = std::move(report);
}

void ShellSettingsSink::applyButtons(const core::ButtonSettings &buttons)
{
    if (reportButtons_) {
        reportButtons_(buttons);
    }
}

void ShellSettingsSink::applyAppearance(const core::AppearanceSettings &appearance)
{

    core::Logger::instance().log(
        core::LogLevel::Debug, "ui",
        "appearance: language " +
            (appearance.language.empty() ? std::string{"(system)"} : appearance.language) +
            ", theme " + std::string{toString(appearance.theme)});

    if (reportAppearance_) {
        reportAppearance_(appearance);
    }
}

void ShellSettingsSink::applyStartup(const core::StartupSettings &startup)
{

    core::Logger::instance().log(core::LogLevel::Debug, "ui",
                                 std::string{"startup: close to tray "} +
                                     (startup.closeToTray ? "on" : "off") + ", autostart " +
                                     (startup.autostart ? "on" : "off") + ", start minimised " +
                                     (startup.startMinimised ? "on" : "off"));

    if (autostart_ != nullptr) {
        const bool wrote =
            startup.autostart ? autostart_->enable(startup.startMinimised) : autostart_->disable();
        if (!wrote) {

            core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                         "the session's autostart entry could not be "
                                         "written; see the message above");
        }
    }

    if (menuEntry_ != nullptr) {
        bool wrote = true;
        switch (startup.menuEntry) {
        case core::MenuEntry::Wanted:
            wrote = menuEntry_->add();
            break;
        case core::MenuEntry::NotWanted:
            wrote = menuEntry_->remove();
            break;
        case core::MenuEntry::NotAsked:
            break;
        }

        if (!wrote) {
            core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                         "the applications menu entry could not be written; "
                                         "see the message above");
        }
    }

    if (reportStartup_) {
        reportStartup_(startup);
    }
}

void ShellSettingsSink::applyHotkeys(const core::HotkeySettings &hotkeys)
{
    core::Logger::instance().log(core::LogLevel::Debug, "ui",
                                 "hotkeys: " + std::to_string(hotkeys.bindings.size()) + " bound");

    if (reportHotkeys_) {
        reportHotkeys_(hotkeys);
    }
}

void ShellSettingsSink::setMenuEntry(IMenuEntry *entry)
{
    menuEntry_ = entry;
}

void ShellSettingsSink::setDiagnosticsReporter(DiagnosticsReporter report)
{
    reportDiagnostics_ = std::move(report);
}

void ShellSettingsSink::applyDiagnostics(const core::DiagnosticsSettings &diagnostics)
{

    const auto told = core::logLevelFromEnvironment();
    core::Logger::instance().setLevel(told.value_or(diagnostics.level));
    core::Logger::instance().setSink(core::makeSink(core::defaultLogFile(), diagnostics.systemLog));

    core::Logger::instance().log(diagnostics.level, "ui",
                                 "logging at " + std::string{core::toString(diagnostics.level)});

    if (reportDiagnostics_) {
        reportDiagnostics_(diagnostics);
    }
}

void ShellSettingsSink::setWebhooksReporter(WebhooksReporter report)
{
    reportWebhooks_ = std::move(report);
}

void ShellSettingsSink::applyWebhooks(const core::WebhookSettings &webhooks)
{
    core::Logger::instance().log(core::LogLevel::Info, "ui",
                                 webhooks.enabled ? "webhooks to " + webhooks.url
                                                  : std::string{"webhooks off"});

    if (reportWebhooks_) {
        reportWebhooks_(webhooks);
    }
}

void ShellSettingsSink::setRecordingReporter(RecordingReporter report)
{
    reportRecording_ = std::move(report);
}

void ShellSettingsSink::applyRecording(const core::RecordingSettings &recording)
{
    core::Logger::instance().log(core::LogLevel::Info, "ui",
                                 std::string{"recording "} +
                                     std::string{toString(recording.policy)} + ", consent " +
                                     std::string{toString(recording.consent)});

    if (reportRecording_) {
        reportRecording_(recording);
    }
}

void ShellSettingsSink::setApiReporter(ApiReporter report)
{
    reportApi_ = std::move(report);
}

void ShellSettingsSink::applyApi(const core::ApiSettings &api)
{
    if (reportApi_) {
        reportApi_(api);
    }
}

void ShellSettingsSink::setUpdateReporter(UpdateReporter report)
{
    reportUpdates_ = std::move(report);
}

void ShellSettingsSink::applyUpdates(const core::UpdateSettings &updates)
{
    if (reportUpdates_) {
        reportUpdates_(updates);
    }
}

void ShellSettingsSink::applyAccount(const core::AccountConfig &account)
{
    if (const auto problem = core::validate(account); problem != core::ConfigProblem::None) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "the settings hold an account the engine will not take: " +
                                         std::string{toString(problem)});
        return;
    }

    calls_.setAccountPolicy(account.id, core::callPolicyFor(account));

    if (!account.enabled) {

        static_cast<void>(engine_.stopRegistration(account.id));
        return;
    }

    if (const auto problem = engine_.addAccount(account); problem != core::ConfigProblem::None) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "the settings hold an account the engine will not take: " +
                                         std::string{toString(problem)});
        return;
    }
    static_cast<void>(engine_.startRegistration(account.id));
}

void ShellSettingsSink::removeAccount(const core::AccountConfig &account)
{
    static_cast<void>(engine_.stopRegistration(account.id));
    static_cast<void>(engine_.removeAccount(account.id));
    calls_.forgetAccountPolicy(account.id);

    if (forgetSecret_) {
        forgetSecret_(account);
    }
}

void ShellSettingsSink::ownedBy(const std::string &group,
                                std::function<void(const std::string &)> apply)
{
    if (apply) {
        owners_[group] = std::move(apply);
    } else {
        owners_.erase(group);
    }
}

void ShellSettingsSink::applyOwned(const std::string &group, const std::string &value)
{
    if (const auto owner = owners_.find(group); owner != owners_.end()) {
        core::Logger::instance().log(core::LogLevel::Debug, "ui",
                                     "settings group '" + group + "' to whoever claimed it");
        owner->second(value);
        return;
    }

    core::Logger::instance().log(core::LogLevel::Debug, "ui",
                                 "no plugin has claimed the settings group '" + group +
                                     "'; it is kept as it is");
}

void ShellSettingsSink::republish()
{
    devices_ = audio_.devices();

    core::AudioApplyResult wouldBe;
    wouldBe.microphone =
        core::chooseDevice(chosen_.microphone, devices_, core::DeviceRole::Capture);
    wouldBe.speakers = core::chooseDevice(chosen_.speakers, devices_, core::DeviceRole::Playback);
    wouldBe = core::planRinger(chosen_.ringer, devices_, std::move(wouldBe));

    const bool defaultMoved =
        everApplied_ &&
        ((result_.microphone.use == core::DeviceUse::SystemDefault &&
          audio_.systemDefaultDevice(core::DeviceRole::Capture) != defaultWas_[0]) ||
         ((result_.speakers.use == core::DeviceUse::SystemDefault ||
           result_.ringer.use == core::DeviceUse::SystemDefault) &&
          audio_.systemDefaultDevice(core::DeviceRole::Playback) != defaultWas_[1]));

    if (defaultMoved || wouldBe.microphone.use != result_.microphone.use ||
        wouldBe.speakers.use != result_.speakers.use || wouldBe.ringer.use != result_.ringer.use ||
        wouldBe.ringerOnItsOwnDevice != result_.ringerOnItsOwnDevice) {
        applyAudio(chosen_);
        return;
    }

    publish();
}

void ShellSettingsSink::publish()
{
    if (!report_) {
        return;
    }

    auto offered = core::devicesWorthOffering(devices_);

    core::Logger::instance().log(core::LogLevel::Debug, "ui",
                                 "offering " + std::to_string(offered.size()) + " of " +
                                     std::to_string(devices_.size()) + " sound devices");

    report_(AudioReport{std::move(offered), chosen_, result_});
}

}
