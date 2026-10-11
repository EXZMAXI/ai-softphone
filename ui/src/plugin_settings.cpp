// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "plugin_settings.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>

namespace softphone::ui {
namespace {

[[nodiscard]] nlohmann::json objectIn(const std::string &text)
{
    if (text.empty()) {
        return nlohmann::json::object();
    }
    auto parsed = nlohmann::json::parse(text, nullptr, false);
    return parsed.is_object() ? std::move(parsed) : nlohmann::json::object();
}

}

std::string groupText(const core::StoredConfiguration &configuration, std::string_view name)
{
    const auto held = configuration.owned.find(std::string{name});
    return held == configuration.owned.end() ? std::string{} : held->second.value;
}

void putGroup(core::StoredConfiguration &configuration, std::string_view name, std::string text,
              const std::string &owner)
{
    if (groupText(configuration, name) == text) {
        return;
    }
    configuration.owned[std::string{name}] = core::OwnedSettings{owner, std::move(text)};
}

std::string asText(const PipelineSettings &settings)
{
    return nlohmann::json{
        {"automatic", settings.automatic},
        {"monthlyCeilingMicros", settings.monthlyCeilingMicros},
        {"monthlyCeilingTokens", settings.monthlyCeilingTokens},
        {"currency", settings.currency},
    }
        .dump();
}

PipelineSettings pipelineFrom(const std::string &text)
{
    const auto group = objectIn(text);
    PipelineSettings settings;
    settings.automatic = group.value("automatic", settings.automatic);
    settings.monthlyCeilingMicros =
        group.value("monthlyCeilingMicros", settings.monthlyCeilingMicros);
    settings.monthlyCeilingTokens =
        group.value("monthlyCeilingTokens", settings.monthlyCeilingTokens);
    settings.currency = group.value("currency", settings.currency);
    return settings;
}

std::string asText(const RealtimeSettings &settings)
{
    return nlohmann::json{
        {"monthlyRecogniserMicros", settings.monthlyRecogniserMicros},
        {"monthlyModelMicros", settings.monthlyModelMicros},
        {"currency", settings.currency},
        {"recogniserId", settings.recogniserId},
        {"modelId", settings.modelId},
        {"offered", settings.offered},
        {"columnTextPixels", settings.columnTextPixels},
        {"repeatTheLast", settings.repeatTheLast},
        {"repeatTextPixels", settings.repeatTextPixels},
    }
        .dump();
}

RealtimeSettings realtimeFrom(const std::string &text)
{
    const auto group = objectIn(text);
    RealtimeSettings settings;
    settings.monthlyRecogniserMicros =
        group.value("monthlyRecogniserMicros", settings.monthlyRecogniserMicros);
    settings.monthlyModelMicros = group.value("monthlyModelMicros", settings.monthlyModelMicros);
    settings.currency = group.value("currency", settings.currency);
    settings.recogniserId = group.value("recogniserId", settings.recogniserId);
    settings.modelId = group.value("modelId", settings.modelId);
    settings.offered = group.value("offered", settings.offered);

    settings.columnTextPixels =
        std::clamp(group.value("columnTextPixels", settings.columnTextPixels),
                   kLeastColumnTextPixels, kMostColumnTextPixels);
    settings.repeatTheLast = group.value("repeatTheLast", settings.repeatTheLast);
    settings.repeatTextPixels =
        std::clamp(group.value("repeatTextPixels", settings.repeatTextPixels),
                   kLeastRepeatTextPixels, kMostRepeatTextPixels);
    return settings;
}

std::string asText(const TranscriptionSettings &settings)
{
    return nlohmann::json{
        {"automatic", settings.automatic},
        {"language", settings.language},
    }
        .dump();
}

TranscriptionSettings transcriptionFrom(const std::string &text)
{
    const auto group = objectIn(text);
    TranscriptionSettings settings;
    settings.automatic = group.value("automatic", settings.automatic);
    settings.language = group.value("language", settings.language);
    return settings;
}

namespace {

[[nodiscard]] nlohmann::json
asJson(const std::vector<plugins::capture::ApplicationPolicy> &policies)
{
    auto written = nlohmann::json::array();
    for (const auto &one : policies) {
        written.push_back(
            nlohmann::json{{"process", one.process}, {"answer", std::string{asText(one.answer)}}});
    }
    return written;
}

[[nodiscard]] std::vector<plugins::capture::ApplicationPolicy>
policiesFrom(const nlohmann::json &written)
{
    std::vector<plugins::capture::ApplicationPolicy> read;
    if (!written.is_array()) {
        return read;
    }
    for (const auto &one : written) {
        if (!one.is_object()) {
            continue;
        }
        auto process = one.value("process", std::string{});
        if (process.empty()) {
            continue;
        }

        if (const auto answer =
                plugins::capture::parseOnFinding(one.value("answer", std::string{}))) {
            read.push_back(plugins::capture::ApplicationPolicy{std::move(process), *answer});
        }
    }
    return read;
}

}

std::string asText(const CaptureSettings &settings)
{
    return nlohmann::json{
        {"enabled", settings.enabled},
        {"onFinding", std::string{asText(settings.onFinding)}},
        {"preRollSeconds", settings.preRollSeconds},
        {"remind", settings.remind},
        {"outputDevice", settings.outputDevice},
        {"inputDevice", settings.inputDevice},
        {"perApplication", asJson(settings.perApplication)},
        {"pageDrawing", std::string{core::toString(settings.pageDrawing)}},
        {"stripDrawing", std::string{core::toString(settings.stripDrawing)}},
    }
        .dump();
}

CaptureSettings captureFrom(const std::string &text)
{
    const auto group = objectIn(text);
    CaptureSettings settings;
    settings.enabled = group.value("enabled", settings.enabled);
    settings.preRollSeconds = group.value("preRollSeconds", settings.preRollSeconds);
    settings.remind = group.value("remind", settings.remind);
    settings.outputDevice = group.value("outputDevice", settings.outputDevice);
    settings.inputDevice = group.value("inputDevice", settings.inputDevice);

    if (const auto said =
            plugins::capture::parseOnFinding(group.value("onFinding", std::string{}))) {
        settings.onFinding = *said;
    } else if (group.contains("automatic")) {
        settings.onFinding = group.value("automatic", false) ? plugins::capture::OnFinding::Record
                                                             : plugins::capture::OnFinding::Never;
    }

    if (group.contains("perApplication")) {
        settings.perApplication = policiesFrom(group.at("perApplication"));
    } else {
        for (const auto &one : group.value("excluded", std::vector<std::string>{})) {
            settings.perApplication.push_back(
                plugins::capture::ApplicationPolicy{one, plugins::capture::OnFinding::Never});
        }
    }

    if (const auto drawn = core::parseVisualisation(group.value("pageDrawing", std::string{}))) {
        settings.pageDrawing = *drawn;
    }
    if (const auto drawn = core::parseVisualisation(group.value("stripDrawing", std::string{}))) {
        settings.stripDrawing = *drawn;
    }

    if (!core::fitsInARow(settings.stripDrawing)) {
        settings.stripDrawing = core::Visualisation::Levels;
    }

    settings.preRollSeconds = std::min(settings.preRollSeconds, kMaxCapturePreRollSeconds);
    return settings;
}

plugins::capture::ICaptureControl::Settings asControllerSettings(const CaptureSettings &settings,
                                                                 std::uint32_t processId)
{
    plugins::capture::ICaptureControl::Settings wanted;
    wanted.enabled = settings.enabled;
    wanted.onFinding = settings.onFinding;
    wanted.perApplication = settings.perApplication;
    wanted.preRollSeconds = settings.preRollSeconds;
    wanted.remind = settings.remind;
    wanted.outputDevice = settings.outputDevice;
    wanted.inputDevice = settings.inputDevice;
    wanted.ownProcessId = processId;
    return wanted;
}

}
