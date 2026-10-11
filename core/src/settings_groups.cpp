// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/settings_groups.hpp>

#include <softphone/core/countries.hpp>
#include <softphone/core/webhook_events.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <ranges>

namespace softphone::core {
namespace {

[[nodiscard]] nlohmann::json objectIn(std::string_view text)
{
    if (text.empty()) {
        return nlohmann::json::object();
    }
    auto parsed = nlohmann::json::parse(text, nullptr, false);
    if (parsed.is_object() || parsed.is_array()) {
        return parsed;
    }
    return nlohmann::json::object();
}

[[nodiscard]] nlohmann::json asJson(const AppearanceSettings &settings)
{
    return nlohmann::json{
        {"language", settings.language},
        {"theme", toString(settings.theme)},
        {"callNotice", toString(settings.callNotice)},
        {"glyphs", toString(settings.glyphs)},
        {"glyphSize", toString(settings.glyphSize)},
        {"visualisation", toString(settings.visualisation)},
        {"endedCallSeconds", settings.endedCallSeconds},
        {"layout", toString(settings.layout)},
        {"barScreen", settings.barScreen},
    };
}

void readInto(const nlohmann::json &group, AppearanceSettings &settings)
{
    if (!group.is_object()) {
        return;
    }

    settings.language = group.value("language", std::string{});
    if (const auto theme = parseTheme(group.value("theme", std::string{"default"}))) {
        settings.theme = *theme;
    }

    if (const auto notice = parseCallNotice(group.value("callNotice", std::string{"banner"}))) {
        settings.callNotice = *notice;
    }
    if (const auto glyphs = parseGlyphs(group.value("glyphs", std::string{"solid"}))) {
        settings.glyphs = *glyphs;
    }
    if (const auto size = parseGlyphSize(group.value("glyphSize", std::string{"normal"}))) {
        settings.glyphSize = *size;
    }

    if (const auto drawing =
            parseVisualisation(group.value("visualisation", std::string{"uprights"}))) {
        settings.visualisation = *drawing;
    }

    settings.endedCallSeconds =
        std::clamp(group.value("endedCallSeconds", 3), kMinEndedCallSeconds, kMaxEndedCallSeconds);

    if (const auto layout =
            parseLayout(group.value("layout", std::string{"workspace-and-phone"}))) {
        settings.layout = *layout;
    }

    settings.barScreen = group.value("barScreen", std::string{});
}

[[nodiscard]] nlohmann::json asJson(const UpdateSettings &settings)
{
    return nlohmann::json{
        {"automatic", settings.automatic},
    };
}

void readInto(const nlohmann::json &group, UpdateSettings &settings)
{
    if (!group.is_object()) {
        return;
    }

    settings.automatic = group.value("automatic", true);
}

[[nodiscard]] nlohmann::json asJson(const RegionSettings &settings)
{
    return nlohmann::json{
        {"country", settings.country},
        {"decided", settings.decided},
    };
}

void readInto(const nlohmann::json &group, RegionSettings &settings)
{
    if (!group.is_object()) {
        return;
    }

    auto country = group.value("country", std::string{});
    settings.country = isCountry(country) ? std::move(country) : std::string{};
    settings.decided = group.value("decided", false);
}

[[nodiscard]] nlohmann::json asJson(const TelemetrySettings &settings)
{
    return nlohmann::json{
        {"mode", std::string{toString(settings.mode)}},
    };
}

void readInto(const nlohmann::json &group, TelemetrySettings &settings)
{
    if (!group.is_object()) {
        return;
    }

    settings.mode = parseTelemetryMode(group.value("mode", std::string{"extended"}));
}

[[nodiscard]] nlohmann::json asJson(const StartupSettings &settings)
{
    return nlohmann::json{
        {"closeToTray", settings.closeToTray},
        {"autostart", settings.autostart},
        {"startMinimised", settings.startMinimised},
        {"menuEntry", std::string{toString(settings.menuEntry)}},
        {"callLinkDialsAtOnce", settings.callLinkDialsAtOnce},
    };
}

void readInto(const nlohmann::json &group, StartupSettings &settings)
{
    if (!group.is_object()) {
        return;
    }

    settings.closeToTray = group.value("closeToTray", true);
    settings.autostart = group.value("autostart", false);
    settings.startMinimised = group.value("startMinimised", false);

    settings.menuEntry = parseMenuEntry(group.value("menuEntry", std::string{"not-asked"}));
    settings.callLinkDialsAtOnce = group.value("callLinkDialsAtOnce", false);
}

[[nodiscard]] nlohmann::json asJson(const HotkeySettings &settings)
{

    nlohmann::json hotkeys = nlohmann::json::array();
    for (const auto &binding : settings.bindings) {
        hotkeys.push_back(nlohmann::json{{"action", toString(binding.action)},
                                         {"sequence", binding.sequence},
                                         {"global", binding.global}});
    }

    return hotkeys;
}

void readInto(const nlohmann::json &group, HotkeySettings &settings)
{
    if (!group.is_array()) {
        return;
    }

    for (const auto &entry : group) {
        if (!entry.is_object()) {
            continue;
        }

        const auto action = parseHotkeyAction(entry.value("action", std::string{}));
        if (!action.has_value()) {

            continue;
        }

        settings.bindings.push_back(
            Hotkey{*action, entry.value("sequence", std::string{}), entry.value("global", false)});
    }
}

[[nodiscard]] nlohmann::json asJson(const DiagnosticsSettings &settings)
{
    return nlohmann::json{
        {"level", toString(settings.level)},
        {"systemLog", settings.systemLog},
    };
}

void readInto(const nlohmann::json &group, DiagnosticsSettings &settings)
{
    if (!group.is_object()) {
        return;
    }

    if (const auto level = parseLogLevel(group.value("level", std::string{"info"}))) {
        settings.level = *level;
    }
    settings.systemLog = group.value("systemLog", false);
}

[[nodiscard]] nlohmann::json asJson(const ButtonSettings &settings)
{

    nlohmann::json buttons = nlohmann::json::array();
    for (const auto &button : settings.buttons) {
        buttons.push_back(nlohmann::json{{"id", button.id},
                                         {"title", button.title},
                                         {"number", button.number},
                                         {"account", button.accountId},
                                         {"watched", button.watched}});
    }

    return buttons;
}

void readInto(const nlohmann::json &group, ButtonSettings &settings)
{
    if (!group.is_array()) {
        return;
    }

    for (const auto &entry : group) {
        if (!entry.is_object()) {
            continue;
        }

        PhoneButton button;
        button.id = entry.value("id", std::string{});
        button.title = entry.value("title", std::string{});
        button.number = entry.value("number", std::string{});
        button.accountId = entry.value("account", std::string{});
        button.watched = entry.value("watched", true);

        if (button.id.empty() || button.number.empty()) {
            continue;
        }

        settings.buttons.push_back(std::move(button));
    }
}

[[nodiscard]] nlohmann::json asJson(const WebhookSettings &settings)
{
    return nlohmann::json{
        {"enabled", settings.enabled},         {"url", settings.url},
        {"method", toString(settings.method)}, {"silenced", settings.silenced},
        {"urlTemplate", settings.urlTemplate}, {"headerName", settings.headerName},

    };
}

void readInto(const nlohmann::json &group, WebhookSettings &settings)
{
    if (!group.is_object()) {
        return;
    }

    settings.enabled = group.value("enabled", false);
    settings.url = group.value("url", std::string{});
    if (const auto method = parseHttpMethod(group.value("method", std::string{"POST"}))) {
        settings.method = *method;
    }
    if (const auto silenced = group.find("silenced");
        silenced != group.end() && silenced->is_array()) {
        settings.silenced = silenced->get<std::vector<std::string>>();
    } else {

        for (const auto &[key, event] :
             {std::pair{"onStarted", webhookEvent::kStarted},
              std::pair{"onEnded", webhookEvent::kEnded},
              std::pair{"onStateChanged", webhookEvent::kStateChanged}}) {
            if (!group.value(key, true)) {
                settings.silenced.emplace_back(event);
            }
        }
    }

    settings.urlTemplate = renamedPlaceholders(group.value("urlTemplate", std::string{}));
    settings.headerName = group.value("headerName", std::string{"Authorization"});
}

[[nodiscard]] nlohmann::json asJson(const RecordingSettings &settings)
{
    return nlohmann::json{
        {"policy", toString(settings.policy)},
        {"accounts", settings.accounts},
        {"consent", toString(settings.consent)},
        {"announcement", settings.announcement},
        {"toneIntervalSeconds", settings.toneIntervalSeconds},
        {"keepStems", settings.keepStems},
        {"keepNotices", settings.keepNotices},
        {"library", settings.library},
        {"maxAgeDays", settings.retention.maxAgeDays},
        {"maxSizeMegabytes", settings.retention.maxSizeMegabytes},
        {"dropStemsAfterDays", settings.retention.dropStemsAfterDays},
        {"freeSpaceFloorMegabytes", settings.retention.freeSpaceFloorMegabytes},
    };
}

void readInto(const nlohmann::json &group, RecordingSettings &settings)
{
    if (!group.is_object()) {
        return;
    }

    if (const auto policy = parseRecordingPolicy(group.value("policy", std::string{"never"}))) {
        settings.policy = *policy;
    }
    if (const auto accounts = group.find("accounts");
        accounts != group.end() && accounts->is_array()) {
        settings.accounts.clear();
        for (const auto &account : *accounts) {
            if (account.is_string()) {
                settings.accounts.push_back(account.get<std::string>());
            }
        }
    }
    if (const auto consent = parseConsentPosture(group.value("consent", std::string{"announce"}))) {

        settings.consent = *consent == ConsentPosture::Confirm ? ConsentPosture::Silent : *consent;
    }
    settings.announcement = group.value("announcement", std::string{});
    settings.toneIntervalSeconds =
        std::clamp(group.value("toneIntervalSeconds", 15), kMinToneInterval, kMaxToneInterval);

    settings.keepStems = group.value("keepStems", true);

    settings.keepNotices = group.value("keepNotices", false);
    settings.library = group.value("library", std::string{});

    auto &retention = settings.retention;
    retention.maxAgeDays = std::clamp(group.value("maxAgeDays", 0), 0, kMaxRetentionDays);
    retention.maxSizeMegabytes =
        std::clamp(group.value("maxSizeMegabytes", 0), 0, kMaxLibraryMegabytes);
    retention.dropStemsAfterDays =
        std::clamp(group.value("dropStemsAfterDays", 0), 0, kMaxRetentionDays);
    retention.freeSpaceFloorMegabytes = std::clamp(group.value("freeSpaceFloorMegabytes", 500),
                                                   kMinFreeSpaceMegabytes, kMaxFreeSpaceMegabytes);
}

[[nodiscard]] nlohmann::json asJson(const ApiSettings &settings)
{
    return nlohmann::json{
        {"enabled", settings.enabled}, {"port", settings.port}, {"disabled", settings.disabled},

    };
}

void readInto(const nlohmann::json &group, ApiSettings &settings)
{
    if (!group.is_object()) {
        return;
    }

    settings.enabled = group.value("enabled", false);
    settings.port = group.value("port", static_cast<std::uint16_t>(8377));
    if (const auto disabled = group.find("disabled");
        disabled != group.end() && disabled->is_array()) {
        settings.disabled = disabled->get<std::vector<std::string>>();
    } else {

        for (const auto *resource :
             {"contacts", "history", "calls", "accounts", "settings", "metrics"}) {
            if (!group.value(resource, true)) {
                settings.disabled.emplace_back(resource);
            }
        }
    }
}

}

std::string asText(const AppearanceSettings &settings)
{
    return asJson(settings).dump();
}

AppearanceSettings appearanceFrom(std::string_view text)
{
    AppearanceSettings settings;
    readInto(objectIn(text), settings);
    return settings;
}

std::string asText(const UpdateSettings &settings)
{
    return asJson(settings).dump();
}

UpdateSettings updatesFrom(std::string_view text)
{
    UpdateSettings settings;
    readInto(objectIn(text), settings);
    return settings;
}

std::string asText(const RegionSettings &settings)
{
    return asJson(settings).dump();
}

RegionSettings regionFrom(std::string_view text)
{
    RegionSettings settings;
    readInto(objectIn(text), settings);
    return settings;
}

std::string asText(const TelemetrySettings &settings)
{
    return asJson(settings).dump();
}

TelemetrySettings telemetryFrom(std::string_view text)
{
    TelemetrySettings settings;
    readInto(objectIn(text), settings);
    return settings;
}

std::string asText(const StartupSettings &settings)
{
    return asJson(settings).dump();
}

StartupSettings startupFrom(std::string_view text)
{
    StartupSettings settings;
    readInto(objectIn(text), settings);
    return settings;
}

std::string asText(const HotkeySettings &settings)
{
    return asJson(settings).dump();
}

HotkeySettings hotkeysFrom(std::string_view text)
{
    HotkeySettings settings;
    readInto(objectIn(text), settings);
    return settings;
}

std::string asText(const DiagnosticsSettings &settings)
{
    return asJson(settings).dump();
}

DiagnosticsSettings diagnosticsFrom(std::string_view text)
{
    DiagnosticsSettings settings;
    readInto(objectIn(text), settings);
    return settings;
}

std::string asText(const ButtonSettings &settings)
{
    return asJson(settings).dump();
}

ButtonSettings buttonsFrom(std::string_view text)
{
    ButtonSettings settings;
    readInto(objectIn(text), settings);
    return settings;
}

std::string asText(const WebhookSettings &settings)
{
    return asJson(settings).dump();
}

WebhookSettings webhooksFrom(std::string_view text)
{
    WebhookSettings settings;
    readInto(objectIn(text), settings);
    return settings;
}

std::string asText(const RecordingSettings &settings)
{
    return asJson(settings).dump();
}

RecordingSettings recordingFrom(std::string_view text)
{
    RecordingSettings settings;
    readInto(objectIn(text), settings);
    return settings;
}

std::string asText(const ApiSettings &settings)
{
    return asJson(settings).dump();
}

ApiSettings apiFrom(std::string_view text)
{
    ApiSettings settings;
    readInto(objectIn(text), settings);
    return settings;
}

}
