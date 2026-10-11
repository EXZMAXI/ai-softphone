// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "api_settings_json.hpp"

#include <softphone/core/api_json.hpp>
#include <softphone/core/config_store.hpp>
#include <softphone/core/settings_groups.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <ranges>

namespace softphone::ui {
namespace {

using namespace softphone::core;

[[nodiscard]] std::string groupText(const core::StoredConfiguration &configuration,
                                    const char *name)
{
    const auto held = configuration.owned.find(name);
    return held == configuration.owned.end() ? std::string{} : held->second.value;
}

void put(core::StoredConfiguration &configuration, const char *name, std::string text,
         const std::string &owner)
{
    if (groupText(configuration, name) == text) {
        return;
    }
    configuration.owned[name] = core::OwnedSettings{owner, std::move(text)};
}

}

using namespace softphone::core;

std::string settingsToJson(const core::StoredConfiguration &configuration)
{
    const auto &settings = configuration.settings;
    const auto appearanceGroup = core::appearanceFrom(groupText(configuration, "appearance"));
    const auto updatesGroup = core::updatesFrom(groupText(configuration, "updates"));
    const auto startupGroup = core::startupFrom(groupText(configuration, "startup"));
    const auto hotkeysGroup = core::hotkeysFrom(groupText(configuration, "hotkeys"));
    const auto diagnosticsGroup = core::diagnosticsFrom(groupText(configuration, "diagnostics"));
    const auto webhooksGroup = core::webhooksFrom(groupText(configuration, "webhooks"));
    const auto apiGroup = core::apiFrom(groupText(configuration, "api"));
    nlohmann::json json;

    json["audio"] = {{"microphone", settings.audio.microphone.name},
                     {"speakers", settings.audio.speakers.name},
                     {"ringer", settings.audio.ringer.name},
                     {"ringtone", settings.audio.ringtone.ownFile()
                                      ? settings.audio.ringtone.file
                                      : settings.audio.ringtone.bundled},
                     {"volumes",
                      {{"microphone", settings.audio.volumes.microphone},
                       {"speakers", settings.audio.volumes.speakers},
                       {"ring", settings.audio.volumes.ring},
                       {"notifications", settings.audio.volumes.notifications}}}};

    auto codecs = nlohmann::json::array();
    for (const auto &codec : settings.calls.codecs) {
        codecs.push_back(nlohmann::json{{"id", codec.id}, {"enabled", codec.enabled}});
    }
    json["calls"] = {{"codecs", codecs},
                     {"dialAccount", settings.calls.dialAccount},
                     {"callWaiting", std::string{toString(settings.calls.callWaiting)}},
                     {"redialIntervalSeconds", settings.calls.redialIntervalSeconds},
                     {"redialGiveUpMinutes", settings.calls.redialGiveUpMinutes}};

    json["appearance"] = {{"language", appearanceGroup.language},
                          {"theme", std::string{toString(appearanceGroup.theme)}},
                          {"callNotice", std::string{toString(appearanceGroup.callNotice)}},
                          {"glyphs", std::string{toString(appearanceGroup.glyphs)}},
                          {"glyphSize", std::string{toString(appearanceGroup.glyphSize)}},
                          {"visualisation", std::string{toString(appearanceGroup.visualisation)}},
                          {"endedCallSeconds", appearanceGroup.endedCallSeconds},
                          {"layout", std::string{toString(appearanceGroup.layout)}}};

    json["updates"] = {{"automatic", updatesGroup.automatic}};

    json["startup"] = {{"closeToTray", startupGroup.closeToTray},
                       {"autostart", startupGroup.autostart},
                       {"startMinimised", startupGroup.startMinimised},
                       {"callLinkDialsAtOnce", startupGroup.callLinkDialsAtOnce}};

    auto hotkeys = nlohmann::json::array();
    for (const auto &hotkey : hotkeysGroup.bindings) {
        hotkeys.push_back(nlohmann::json{{"action", std::string{toString(hotkey.action)}},
                                         {"sequence", hotkey.sequence},
                                         {"global", hotkey.global}});
    }
    json["hotkeys"] = hotkeys;

    json["diagnostics"] = {{"level", std::string{toString(diagnosticsGroup.level)}},
                           {"systemLog", diagnosticsGroup.systemLog}};

    json["webhooks"] = {{"enabled", webhooksGroup.enabled},
                        {"url", webhooksGroup.url},
                        {"method", std::string{toString(webhooksGroup.method)}},
                        {"silenced", webhooksGroup.silenced},
                        {"urlTemplate", webhooksGroup.urlTemplate},
                        {"headerName", webhooksGroup.headerName}};

    json["api"] = {
        {"enabled", apiGroup.enabled}, {"port", apiGroup.port}, {"disabled", apiGroup.disabled}};

    return json.dump();
}

std::optional<core::StoredConfiguration> settingsFromJson(std::string_view body,
                                                          const core::StoredConfiguration &current,
                                                          std::string &problem)
{
    const auto json = nlohmann::json::parse(body, nullptr, false);
    if (json.is_discarded() || !json.is_object()) {
        problem = "the body is not a JSON object";
        return std::nullopt;
    }

    auto configuration = current;

    auto appearanceGroup = core::appearanceFrom(groupText(current, "appearance"));
    auto updatesGroup = core::updatesFrom(groupText(current, "updates"));
    auto startupGroup = core::startupFrom(groupText(current, "startup"));
    auto hotkeysGroup = core::hotkeysFrom(groupText(current, "hotkeys"));
    auto diagnosticsGroup = core::diagnosticsFrom(groupText(current, "diagnostics"));
    auto webhooksGroup = core::webhooksFrom(groupText(current, "webhooks"));
    auto apiGroup = core::apiFrom(groupText(current, "api"));

    if (const auto appearance = json.find("appearance");
        appearance != json.end() && appearance->is_object()) {
        appearanceGroup.language = appearance->value("language", appearanceGroup.language);
        if (const auto theme = parseTheme(appearance->value("theme", std::string{}))) {
            appearanceGroup.theme = *theme;
        }
        if (const auto notice = parseCallNotice(appearance->value("callNotice", std::string{}))) {
            appearanceGroup.callNotice = *notice;
        }
        if (const auto glyphs = parseGlyphs(appearance->value("glyphs", std::string{}))) {
            appearanceGroup.glyphs = *glyphs;
        }
        if (const auto drawing =
                parseVisualisation(appearance->value("visualisation", std::string{}))) {
            appearanceGroup.visualisation = *drawing;
        }
        if (const auto size = parseGlyphSize(appearance->value("glyphSize", std::string{}))) {
            appearanceGroup.glyphSize = *size;
        }
        if (const auto wait = appearance->find("endedCallSeconds");
            wait != appearance->end() && wait->is_number_integer()) {
            appearanceGroup.endedCallSeconds =
                std::clamp(wait->get<int>(), kMinEndedCallSeconds, kMaxEndedCallSeconds);
        }

        if (const auto layout = parseLayout(appearance->value("layout", std::string{}))) {
            appearanceGroup.layout = *layout;
        }
    }

    if (const auto updates = json.find("updates"); updates != json.end() && updates->is_object()) {
        updatesGroup.automatic = updates->value("automatic", updatesGroup.automatic);
    }

    if (const auto startup = json.find("startup"); startup != json.end() && startup->is_object()) {
        startupGroup.closeToTray = startup->value("closeToTray", startupGroup.closeToTray);
        startupGroup.autostart = startup->value("autostart", startupGroup.autostart);
        startupGroup.startMinimised = startup->value("startMinimised", startupGroup.startMinimised);
        startupGroup.callLinkDialsAtOnce =
            startup->value("callLinkDialsAtOnce", startupGroup.callLinkDialsAtOnce);
    }

    if (const auto diagnostics = json.find("diagnostics");
        diagnostics != json.end() && diagnostics->is_object()) {
        if (const auto level = parseLogLevel(diagnostics->value("level", std::string{}))) {
            diagnosticsGroup.level = *level;
        }
        diagnosticsGroup.systemLog = diagnostics->value("systemLog", diagnosticsGroup.systemLog);
    }

    if (const auto webhooks = json.find("webhooks");
        webhooks != json.end() && webhooks->is_object()) {
        webhooksGroup.enabled = webhooks->value("enabled", webhooksGroup.enabled);
        webhooksGroup.url = webhooks->value("url", webhooksGroup.url);
        if (const auto method = parseHttpMethod(webhooks->value("method", std::string{}))) {
            webhooksGroup.method = *method;
        }
        if (const auto silenced = webhooks->find("silenced");
            silenced != webhooks->end() && silenced->is_array()) {
            webhooksGroup.silenced = silenced->get<std::vector<std::string>>();
        }

        for (const auto &[key, event] :
             {std::pair{"onStarted", webhookEvent::kStarted},
              std::pair{"onEnded", webhookEvent::kEnded},
              std::pair{"onStateChanged", webhookEvent::kStateChanged}}) {
            const auto legacy = webhooks->find(key);
            if (legacy == webhooks->end() || !legacy->is_boolean()) {
                continue;
            }

            auto &quiet = webhooksGroup.silenced;
            const auto at = std::ranges::find(quiet, event);
            if (legacy->get<bool>() && at != quiet.end()) {
                quiet.erase(at);
            } else if (!legacy->get<bool>() && at == quiet.end()) {
                quiet.emplace_back(event);
            }
        }
        webhooksGroup.urlTemplate = webhooks->value("urlTemplate", webhooksGroup.urlTemplate);
        webhooksGroup.headerName = webhooks->value("headerName", webhooksGroup.headerName);

        if (!unknownPlaceholders(webhooksGroup.urlTemplate).empty()) {
            problem = "the URL template names something this phone does not know";
            return std::nullopt;
        }
    }

    if (const auto api = json.find("api"); api != json.end() && api->is_object()) {
        apiGroup.enabled = api->value("enabled", apiGroup.enabled);
        apiGroup.port = api->value("port", apiGroup.port);
        if (const auto disabled = api->find("disabled");
            disabled != api->end() && disabled->is_array()) {
            apiGroup.disabled = disabled->get<std::vector<std::string>>();
        }

        for (const auto *resource :
             {"contacts", "history", "calls", "accounts", "settings", "metrics"}) {
            const auto legacy = api->find(resource);
            if (legacy == api->end() || !legacy->is_boolean()) {
                continue;
            }

            auto &off = apiGroup.disabled;
            const auto at = std::ranges::find(off, resource);
            if (legacy->get<bool>() && at != off.end()) {
                off.erase(at);
            } else if (!legacy->get<bool>() && at == off.end()) {
                off.emplace_back(resource);
            }
        }
    }

    const auto &layout = core::defaultConfigLayout();
    put(configuration, "appearance", core::asText(appearanceGroup), layout.application);
    put(configuration, "updates", core::asText(updatesGroup), layout.application);
    put(configuration, "startup", core::asText(startupGroup), layout.application);
    put(configuration, "hotkeys", core::asText(hotkeysGroup), layout.application);
    put(configuration, "diagnostics", core::asText(diagnosticsGroup), layout.diagnostics);
    put(configuration, "webhooks", core::asText(webhooksGroup), layout.integration);
    put(configuration, "api", core::asText(apiGroup), layout.integration);

    return configuration;
}

}
