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

#include <softphone/core/telemetry.hpp>

#include <softphone/core/account_config.hpp>
#include <softphone/core/buttons.hpp>
#include <softphone/core/call.hpp>
#include <softphone/core/logging.hpp>
#include <softphone/core/recordings.hpp>
#include <softphone/core/retention.hpp>
#include <softphone/core/ringtones.hpp>
#include <softphone/core/webhooks.hpp>

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

struct AudioDevice {
    std::string driver;
    std::string name;

    [[nodiscard]] bool systemDefault() const noexcept
    {
        return name.empty();
    }

    friend bool operator==(const AudioDevice &, const AudioDevice &) = default;

    friend auto operator<=>(const AudioDevice &, const AudioDevice &) = default;
};

struct Volumes {
    int microphone{80};
    int speakers{80};
    int ring{50};

    int notifications{50};

    friend bool operator==(const Volumes &, const Volumes &) = default;
};

[[nodiscard]] float volumeGain(int percent) noexcept;

struct AudioSettings {
    AudioDevice microphone;
    AudioDevice speakers;

    AudioDevice ringer;

    RingtoneChoice ringtone;

    Volumes volumes;

    friend bool operator==(const AudioSettings &, const AudioSettings &) = default;
};

struct CodecChoice {

    std::string id;
    bool enabled{true};

    friend bool operator==(const CodecChoice &, const CodecChoice &) = default;
};

enum class CallWaiting {

    Allowed,

    Busy,
};

[[nodiscard]] std::string_view toString(CallWaiting waiting) noexcept;
[[nodiscard]] std::optional<CallWaiting> parseCallWaiting(std::string_view text) noexcept;

struct CallSettings {

    std::vector<CodecChoice> codecs;

    CallWaiting callWaiting{CallWaiting::Allowed};

    std::string dialAccount;

    int redialIntervalSeconds{15};
    int redialGiveUpMinutes{30};

    int historyDays{0};

    friend bool operator==(const CallSettings &, const CallSettings &) = default;
};

inline constexpr int kMinRedialIntervalSeconds = 5;
inline constexpr int kMaxRedialIntervalSeconds = 120;
inline constexpr int kMinRedialGiveUpMinutes = 1;
inline constexpr int kMaxRedialGiveUpMinutes = 120;

inline constexpr int kMaxHistoryDays = kMaxRetentionDays;

enum class Theme { Default, Light, Dark };

enum class CallNotice {

    Banner,

    Raise,

    Silent,
};

[[nodiscard]] std::string_view toString(CallNotice notice) noexcept;
[[nodiscard]] std::optional<CallNotice> parseCallNotice(std::string_view text) noexcept;

enum class Glyphs {

    Solid,

    Outline,
};

[[nodiscard]] std::string_view toString(Glyphs glyphs) noexcept;
[[nodiscard]] std::optional<Glyphs> parseGlyphs(std::string_view text) noexcept;

enum class GlyphSize {
    Normal,
    Large,
};

[[nodiscard]] std::string_view toString(GlyphSize size) noexcept;
[[nodiscard]] std::optional<GlyphSize> parseGlyphSize(std::string_view text) noexcept;

enum class Visualisation {

    Off,

    Levels,

    Histogram,

    Wave,

    Spectrum,

    Aurora,

    Halo,

    Uprights,
};

[[nodiscard]] std::string_view toString(Visualisation drawing) noexcept;
[[nodiscard]] std::optional<Visualisation> parseVisualisation(std::string_view text) noexcept;

[[nodiscard]] bool needsSpectrum(Visualisation drawing) noexcept;

[[nodiscard]] bool fitsInARow(Visualisation drawing) noexcept;

enum class Layout {

    Windows,

    Compact,

    Workspace,

    WorkspaceAndPhone,

    Bar,

    NoWindow,
};

[[nodiscard]] std::string_view toString(Layout layout) noexcept;
[[nodiscard]] std::optional<Layout> parseLayout(std::string_view text) noexcept;

[[nodiscard]] constexpr bool needsANotificationArea(Layout layout) noexcept
{
    return layout == Layout::NoWindow;
}

[[nodiscard]] constexpr Layout honouredLayout(Layout wanted, bool hasNotificationArea) noexcept
{
    return (needsANotificationArea(wanted) && !hasNotificationArea) ? Layout::Windows : wanted;
}

struct AppearanceSettings {

    std::string language;

    Theme theme{Theme::Default};

    CallNotice callNotice{CallNotice::Banner};

    Glyphs glyphs{Glyphs::Solid};
    GlyphSize glyphSize{GlyphSize::Normal};

    Visualisation visualisation{Visualisation::Uprights};

    int endedCallSeconds{3};

    std::string barScreen;

    Layout layout{Layout::WorkspaceAndPhone};

    friend bool operator==(const AppearanceSettings &, const AppearanceSettings &) = default;
};

inline constexpr int kMinEndedCallSeconds = 0;
inline constexpr int kMaxEndedCallSeconds = 10;

enum class HotkeyAction {
    Answer,
    HangUp,
    HoldResume,
    Mute,
    ShowWindow,
};

struct Hotkey {
    HotkeyAction action{HotkeyAction::Answer};

    std::string sequence;

    bool global{false};

    friend bool operator==(const Hotkey &, const Hotkey &) = default;
};

struct HotkeySettings {

    std::vector<Hotkey> bindings;

    friend bool operator==(const HotkeySettings &, const HotkeySettings &) = default;
};

[[nodiscard]] std::string_view toString(HotkeyAction action) noexcept;
[[nodiscard]] std::optional<HotkeyAction> parseHotkeyAction(std::string_view text) noexcept;

enum class MenuEntry { NotAsked, Wanted, NotWanted };

struct StartupSettings {

    bool closeToTray{true};

    bool autostart{false};

    bool startMinimised{false};

    MenuEntry menuEntry{MenuEntry::NotAsked};

    bool callLinkDialsAtOnce{false};

    friend bool operator==(const StartupSettings &, const StartupSettings &) = default;
};

struct UpdateSettings {
    bool automatic{true};

    friend bool operator==(const UpdateSettings &, const UpdateSettings &) = default;
};

struct RegionSettings {

    std::string country;

    bool decided{false};

    friend bool operator==(const RegionSettings &, const RegionSettings &) = default;
};

struct TelemetrySettings {

    TelemetryMode mode{TelemetryMode::Extended};

    friend bool operator==(const TelemetrySettings &, const TelemetrySettings &) = default;
};

struct DiagnosticsSettings {

    LogLevel level{LogLevel::Info};

    bool systemLog{false};

    friend bool operator==(const DiagnosticsSettings &, const DiagnosticsSettings &) = default;
};

struct WebhookSettings {
    bool enabled{false};

    std::string url;

    HttpMethod method{HttpMethod::Post};

    std::vector<std::string> silenced;

    [[nodiscard]] bool wants(std::string_view event) const noexcept
    {
        for (const auto &quiet : silenced) {
            if (quiet == event) {
                return false;
            }
        }
        return true;
    }

    std::string urlTemplate;

    std::string headerName{"Authorization"};

    friend bool operator==(const WebhookSettings &, const WebhookSettings &) = default;
};

struct ApiSettings {
    bool enabled{false};

    std::uint16_t port{8377};

    std::vector<std::string> disabled;

    [[nodiscard]] bool serves(std::string_view resource) const noexcept
    {
        for (const auto &off : disabled) {
            if (off == resource) {
                return false;
            }
        }
        return true;
    }

    friend bool operator==(const ApiSettings &, const ApiSettings &) = default;
};

enum class RecordingPolicy {

    Never,

    Ask,

    Always,

    OnTheseAccounts,
};

[[nodiscard]] std::string_view toString(RecordingPolicy policy) noexcept;
[[nodiscard]] std::optional<RecordingPolicy> parseRecordingPolicy(std::string_view text) noexcept;

struct RecordingSettings {
    RecordingPolicy policy{RecordingPolicy::Never};

    std::vector<std::string> accounts;

    ConsentPosture consent{ConsentPosture::Announce};

    std::string announcement;

    int toneIntervalSeconds{15};

    bool keepNotices{false};

    bool keepStems{true};

    std::string library;

    RetentionPolicy retention;

    friend bool operator==(const RecordingSettings &, const RecordingSettings &) = default;
};

inline constexpr int kMinToneInterval = 5;
inline constexpr int kMaxToneInterval = 120;

struct Settings {
    AudioSettings audio;
    CallSettings calls;

    friend bool operator==(const Settings &, const Settings &) = default;
};

[[nodiscard]] ConsentPosture consentFor(const RecordingSettings &recording,
                                        const AccountConfig &account);

[[nodiscard]] RecordingPolicy recordingPolicyFor(const RecordingSettings &recording,
                                                 const std::string &accountId);

struct OwnedSettings {

    std::string owner;

    std::string value;

    friend bool operator==(const OwnedSettings &, const OwnedSettings &) = default;
};

struct StoredConfiguration {

    int schemaVersion{2};

    std::vector<AccountConfig> accounts;
    Settings settings;

    int nextAccountNumber{1};

    std::map<std::string, OwnedSettings> owned;

    friend bool operator==(const StoredConfiguration &, const StoredConfiguration &) = default;
};

inline constexpr int kCurrentSchemaVersion = 2;

void upsert(StoredConfiguration &configuration, const AccountConfig &account);

bool remove(StoredConfiguration &configuration, const std::string &accountId);

[[nodiscard]] const AccountConfig *find(const StoredConfiguration &configuration,
                                        const std::string &accountId);

[[nodiscard]] std::string mintAccountId(StoredConfiguration &configuration);

[[nodiscard]] std::vector<AccountConfig> enabledAccounts(const StoredConfiguration &configuration);

[[nodiscard]] std::string dialingAccount(const StoredConfiguration &configuration);

enum class SettingArea {
    Microphone,
    Speakers,
    Ringer,

    Ringtone,

    Volumes,
    Codecs,
    DialAccount,

    Redial,

    CallWaiting,

    AccountOrder,

    Account,

    Owned,
};

enum class ApplySchedule {

    Now,

    NextCall,

    WhenCallsEnd,

    NextStart,
};

struct SettingChange {
    SettingArea area{SettingArea::Microphone};
    ApplySchedule schedule{ApplySchedule::Now};

    std::string subject;

    std::vector<std::string> blockedBy;

    friend bool operator==(const SettingChange &, const SettingChange &) = default;
};

[[nodiscard]] std::vector<SettingChange> settingChanges(const StoredConfiguration &effective,
                                                        const StoredConfiguration &desired,
                                                        std::span<const CallInfo> calls);

[[nodiscard]] std::string_view toString(SettingArea area) noexcept;
[[nodiscard]] std::string_view toString(ApplySchedule schedule) noexcept;
[[nodiscard]] std::string_view toString(Theme theme) noexcept;

[[nodiscard]] std::optional<Theme> parseTheme(std::string_view text) noexcept;

[[nodiscard]] std::string_view toString(MenuEntry entry) noexcept;

[[nodiscard]] MenuEntry parseMenuEntry(std::string_view text) noexcept;

}
