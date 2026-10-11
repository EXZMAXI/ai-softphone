// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/settings.hpp>

#include <softphone/core/uuid.hpp>

#include <algorithm>

namespace softphone::core {
namespace {

[[nodiscard]] bool live(const CallInfo &call) noexcept
{
    return call.state != CallState::Ended && call.state != CallState::Idle;
}

[[nodiscard]] bool anyLive(std::span<const CallInfo> calls) noexcept
{
    return std::ranges::any_of(calls, live);
}

[[nodiscard]] std::vector<std::string> callsOn(std::span<const CallInfo> calls,
                                               const std::string &accountId)
{
    std::vector<std::string> blocked;
    for (const auto &call : calls) {
        if (live(call) && call.accountId == accountId) {
            blocked.push_back(call.id);
        }
    }
    std::ranges::sort(blocked);
    return blocked;
}

[[nodiscard]] ApplySchedule nextCallOrNow(std::span<const CallInfo> calls) noexcept
{
    return anyLive(calls) ? ApplySchedule::NextCall : ApplySchedule::Now;
}

void add(std::vector<SettingChange> &changes, SettingArea area, ApplySchedule schedule)
{
    changes.push_back(SettingChange{area, schedule, {}, {}});
}

}

void upsert(StoredConfiguration &configuration, const AccountConfig &account)
{
    auto stored = account;

    for (auto &existing : configuration.accounts) {
        if (existing.id == account.id) {

            if (stored.uuid.empty()) {
                stored.uuid = existing.uuid;
            }
            existing = stored;
            return;
        }
    }

    configuration.accounts.push_back(stored);
}

bool remove(StoredConfiguration &configuration, const std::string &accountId)
{
    const auto before = configuration.accounts.size();
    std::erase_if(configuration.accounts,
                  [&accountId](const AccountConfig &account) { return account.id == accountId; });
    return configuration.accounts.size() != before;
}

std::string mintAccountId(StoredConfiguration &configuration)
{

    for (;;) {
        auto candidate = "account-" + std::to_string(configuration.nextAccountNumber);
        ++configuration.nextAccountNumber;
        if (find(configuration, candidate) == nullptr) {
            return candidate;
        }
    }
}

std::vector<AccountConfig> enabledAccounts(const StoredConfiguration &configuration)
{
    std::vector<AccountConfig> live;
    for (const auto &account : configuration.accounts) {
        if (account.enabled) {
            live.push_back(account);
        }
    }
    return live;
}

std::string dialingAccount(const StoredConfiguration &configuration)
{
    const auto *stored = find(configuration, configuration.settings.calls.dialAccount);
    if (stored != nullptr && stored->enabled) {
        return stored->id;
    }
    for (const auto &account : configuration.accounts) {
        if (account.enabled) {
            return account.id;
        }
    }
    return {};
}

const AccountConfig *find(const StoredConfiguration &configuration, const std::string &accountId)
{
    for (const auto &account : configuration.accounts) {
        if (account.id == accountId) {
            return &account;
        }
    }
    return nullptr;
}

std::vector<SettingChange> settingChanges(const StoredConfiguration &effective,
                                          const StoredConfiguration &desired,
                                          std::span<const CallInfo> calls)
{
    std::vector<SettingChange> changes;

    if (effective.settings.audio.microphone != desired.settings.audio.microphone) {
        add(changes, SettingArea::Microphone, ApplySchedule::Now);
    }
    if (effective.settings.audio.speakers != desired.settings.audio.speakers) {
        add(changes, SettingArea::Speakers, ApplySchedule::Now);
    }
    if (effective.settings.audio.ringer != desired.settings.audio.ringer) {
        add(changes, SettingArea::Ringer, ApplySchedule::Now);
    }

    if (effective.settings.audio.ringtone != desired.settings.audio.ringtone) {
        add(changes, SettingArea::Ringtone, ApplySchedule::Now);
    }

    if (effective.settings.audio.volumes != desired.settings.audio.volumes) {
        add(changes, SettingArea::Volumes, ApplySchedule::Now);
    }

    if (effective.settings.calls.codecs != desired.settings.calls.codecs) {
        add(changes, SettingArea::Codecs, nextCallOrNow(calls));
    }
    if (effective.settings.calls.redialIntervalSeconds !=
            desired.settings.calls.redialIntervalSeconds ||
        effective.settings.calls.redialGiveUpMinutes !=
            desired.settings.calls.redialGiveUpMinutes) {

        add(changes, SettingArea::Redial, ApplySchedule::Now);
    }
    if (effective.settings.calls.callWaiting != desired.settings.calls.callWaiting) {

        add(changes, SettingArea::CallWaiting, ApplySchedule::Now);
    }
    if (effective.settings.calls.dialAccount != desired.settings.calls.dialAccount) {

        add(changes, SettingArea::DialAccount, ApplySchedule::Now);
    }

    {
        std::vector<std::string> before;
        std::vector<std::string> after;
        for (const auto &account : effective.accounts) {
            before.push_back(account.id);
        }
        for (const auto &account : desired.accounts) {
            after.push_back(account.id);
        }

        auto sortedBefore = before;
        auto sortedAfter = after;
        std::ranges::sort(sortedBefore);
        std::ranges::sort(sortedAfter);

        if (sortedBefore == sortedAfter && before != after) {
            add(changes, SettingArea::AccountOrder, ApplySchedule::Now);
        }
    }

    std::vector<std::string> ids;
    for (const auto &account : effective.accounts) {
        ids.push_back(account.id);
    }
    for (const auto &account : desired.accounts) {
        ids.push_back(account.id);
    }
    std::ranges::sort(ids);
    ids.erase(std::ranges::unique(ids).begin(), ids.end());

    std::vector<std::string> groups;
    for (const auto &[name, group] : effective.owned) {
        groups.push_back(name);
    }
    for (const auto &[name, group] : desired.owned) {
        groups.push_back(name);
    }
    std::ranges::sort(groups);
    groups.erase(std::ranges::unique(groups).begin(), groups.end());

    for (const auto &name : groups) {
        const auto before = effective.owned.find(name);
        const auto after = desired.owned.find(name);
        const auto had = before != effective.owned.end();
        const auto has = after != desired.owned.end();
        if (had && has && before->second == after->second) {
            continue;
        }
        changes.push_back(SettingChange{SettingArea::Owned, ApplySchedule::Now, name, {}});
    }

    for (const auto &id : ids) {

        const auto *before = find(effective, id);
        const auto *after = find(desired, id);
        if (before != nullptr && after != nullptr && *before == *after) {
            continue;
        }

        const bool removed = before != nullptr && after == nullptr;
        auto blocked = removed ? callsOn(calls, id) : std::vector<std::string>{};
        const auto schedule = blocked.empty() ? ApplySchedule::Now : ApplySchedule::WhenCallsEnd;
        changes.push_back(SettingChange{SettingArea::Account, schedule, id, std::move(blocked)});
    }

    return changes;
}

std::string_view toString(CallNotice notice) noexcept
{
    switch (notice) {
    case CallNotice::Banner:
        return "banner";
    case CallNotice::Raise:
        return "raise";
    case CallNotice::Silent:
        return "silent";
    }
    return "banner";
}

std::optional<CallNotice> parseCallNotice(std::string_view text) noexcept
{
    if (text == "banner") {
        return CallNotice::Banner;
    }
    if (text == "raise") {
        return CallNotice::Raise;
    }
    if (text == "silent") {
        return CallNotice::Silent;
    }
    return std::nullopt;
}

std::string_view toString(HotkeyAction action) noexcept
{

    switch (action) {
    case HotkeyAction::Answer:
        return "answer";
    case HotkeyAction::HangUp:
        return "hang-up";
    case HotkeyAction::HoldResume:
        return "hold-resume";
    case HotkeyAction::Mute:
        return "mute";
    case HotkeyAction::ShowWindow:
        return "show-window";
    }
    return "unknown";
}

std::optional<HotkeyAction> parseHotkeyAction(std::string_view text) noexcept
{
    if (text == "answer") {
        return HotkeyAction::Answer;
    }
    if (text == "hang-up") {
        return HotkeyAction::HangUp;
    }
    if (text == "hold-resume") {
        return HotkeyAction::HoldResume;
    }
    if (text == "mute") {
        return HotkeyAction::Mute;
    }
    if (text == "show-window") {
        return HotkeyAction::ShowWindow;
    }
    return std::nullopt;
}

float volumeGain(int percent) noexcept
{
    return static_cast<float>(std::clamp(percent, 0, 100)) / 100.0F;
}

std::string_view toString(SettingArea area) noexcept
{
    switch (area) {
    case SettingArea::Microphone:
        return "microphone";
    case SettingArea::Speakers:
        return "speakers";
    case SettingArea::Ringer:
        return "ringer";
    case SettingArea::Ringtone:
        return "ringtone";
    case SettingArea::Volumes:
        return "volumes";
    case SettingArea::Codecs:
        return "codecs";
    case SettingArea::DialAccount:
        return "dial-account";
    case SettingArea::Redial:
        return "redial";
    case SettingArea::CallWaiting:
        return "call-waiting";
    case SettingArea::AccountOrder:
        return "account-order";
    case SettingArea::Account:
        return "account";
    case SettingArea::Owned:
        return "owned";
    }
    return "unknown";
}

std::string_view toString(ApplySchedule schedule) noexcept
{
    switch (schedule) {
    case ApplySchedule::Now:
        return "now";
    case ApplySchedule::NextCall:
        return "next-call";
    case ApplySchedule::WhenCallsEnd:
        return "when-calls-end";
    case ApplySchedule::NextStart:
        return "next-start";
    }
    return "unknown";
}

std::string_view toString(CallWaiting waiting) noexcept
{
    switch (waiting) {
    case CallWaiting::Allowed:
        return "allowed";
    case CallWaiting::Busy:
        return "busy";
    }
    return "allowed";
}

std::optional<CallWaiting> parseCallWaiting(std::string_view text) noexcept
{
    if (text == "allowed") {
        return CallWaiting::Allowed;
    }
    if (text == "busy") {
        return CallWaiting::Busy;
    }
    return std::nullopt;
}

std::string_view toString(Theme theme) noexcept
{
    switch (theme) {
    case Theme::Default:
        return "default";
    case Theme::Light:
        return "light";
    case Theme::Dark:
        return "dark";
    }
    return "unknown";
}

std::optional<Theme> parseTheme(std::string_view text) noexcept
{
    if (text == "default") {
        return Theme::Default;
    }
    if (text == "light") {
        return Theme::Light;
    }
    if (text == "dark") {
        return Theme::Dark;
    }
    return std::nullopt;
}

std::string_view toString(MenuEntry entry) noexcept
{
    switch (entry) {
    case MenuEntry::NotAsked:
        return "not-asked";
    case MenuEntry::Wanted:
        return "wanted";
    case MenuEntry::NotWanted:
        return "not-wanted";
    }
    return "unknown";
}

MenuEntry parseMenuEntry(std::string_view text) noexcept
{
    if (text == "wanted") {
        return MenuEntry::Wanted;
    }
    if (text == "not-wanted") {
        return MenuEntry::NotWanted;
    }
    return MenuEntry::NotAsked;
}

std::string_view toString(Layout layout) noexcept
{
    switch (layout) {
    case Layout::Windows:
        return "windows";
    case Layout::Compact:
        return "compact";
    case Layout::Workspace:
        return "workspace";
    case Layout::WorkspaceAndPhone:
        return "workspace-and-phone";
    case Layout::Bar:
        return "bar";
    case Layout::NoWindow:
        return "no-window";
    }
    return "unknown";
}

std::optional<Layout> parseLayout(std::string_view text) noexcept
{
    if (text == "windows") {
        return Layout::Windows;
    }
    if (text == "compact") {
        return Layout::Compact;
    }
    if (text == "workspace") {
        return Layout::Workspace;
    }
    if (text == "workspace-and-phone") {
        return Layout::WorkspaceAndPhone;
    }
    if (text == "bar") {
        return Layout::Bar;
    }
    if (text == "no-window") {
        return Layout::NoWindow;
    }
    return std::nullopt;
}

std::string_view toString(Glyphs glyphs) noexcept
{
    switch (glyphs) {
    case Glyphs::Solid:
        return "solid";
    case Glyphs::Outline:
        return "outline";
    }
    return "unknown";
}

std::optional<Glyphs> parseGlyphs(std::string_view text) noexcept
{
    if (text == "solid") {
        return Glyphs::Solid;
    }
    if (text == "outline") {
        return Glyphs::Outline;
    }
    return std::nullopt;
}

std::string_view toString(GlyphSize size) noexcept
{
    switch (size) {
    case GlyphSize::Normal:
        return "normal";
    case GlyphSize::Large:
        return "large";
    }
    return "unknown";
}

std::optional<GlyphSize> parseGlyphSize(std::string_view text) noexcept
{
    if (text == "normal") {
        return GlyphSize::Normal;
    }
    if (text == "large") {
        return GlyphSize::Large;
    }
    return std::nullopt;
}

std::string_view toString(Visualisation drawing) noexcept
{
    switch (drawing) {
    case Visualisation::Off:
        return "off";
    case Visualisation::Levels:
        return "levels";
    case Visualisation::Histogram:
        return "histogram";
    case Visualisation::Wave:
        return "wave";
    case Visualisation::Spectrum:
        return "spectrum";
    case Visualisation::Aurora:
        return "aurora";
    case Visualisation::Halo:
        return "halo";
    case Visualisation::Uprights:
        return "uprights";
    }
    return "unknown";
}

std::optional<Visualisation> parseVisualisation(std::string_view text) noexcept
{
    if (text == "off") {
        return Visualisation::Off;
    }
    if (text == "levels") {
        return Visualisation::Levels;
    }
    if (text == "histogram") {
        return Visualisation::Histogram;
    }
    if (text == "wave") {
        return Visualisation::Wave;
    }
    if (text == "spectrum") {
        return Visualisation::Spectrum;
    }
    if (text == "aurora") {
        return Visualisation::Aurora;
    }
    if (text == "halo") {
        return Visualisation::Halo;
    }
    if (text == "uprights") {
        return Visualisation::Uprights;
    }
    return std::nullopt;
}

bool needsSpectrum(Visualisation drawing) noexcept
{
    return drawing == Visualisation::Spectrum;
}

bool fitsInARow(Visualisation drawing) noexcept
{
    return drawing != Visualisation::Halo && drawing != Visualisation::Uprights;
}

std::string_view toString(RecordingPolicy policy) noexcept
{
    switch (policy) {
    case RecordingPolicy::Never:
        return "never";
    case RecordingPolicy::Ask:
        return "ask";
    case RecordingPolicy::Always:
        return "always";
    case RecordingPolicy::OnTheseAccounts:
        return "these-accounts";
    }
    return "never";
}

std::optional<RecordingPolicy> parseRecordingPolicy(std::string_view text) noexcept
{
    if (text == "never") {
        return RecordingPolicy::Never;
    }
    if (text == "ask") {
        return RecordingPolicy::Ask;
    }
    if (text == "always") {
        return RecordingPolicy::Always;
    }
    if (text == "these-accounts") {
        return RecordingPolicy::OnTheseAccounts;
    }
    return std::nullopt;
}

ConsentPosture consentFor(const RecordingSettings &recording, const AccountConfig &account)
{
    if (const auto own = parseConsentPosture(account.consent)) {
        return *own;
    }
    return recording.consent;
}

RecordingPolicy recordingPolicyFor(const RecordingSettings &recording, const std::string &accountId)
{
    if (recording.policy != RecordingPolicy::OnTheseAccounts) {
        return recording.policy;
    }

    const bool named = std::find(recording.accounts.begin(), recording.accounts.end(), accountId) !=
                       recording.accounts.end();
    return named ? RecordingPolicy::Always : RecordingPolicy::Never;
}

}
