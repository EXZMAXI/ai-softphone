// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/monolith/plugin.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/version.hpp>

#include <string>

namespace softphone::plugins::monolith {
namespace {

[[nodiscard]] core::LogLevel levelFor(container::Severity severity)
{
    switch (severity) {
    case container::Severity::Error:
        return core::LogLevel::Error;
    case container::Severity::Warning:
        return core::LogLevel::Warning;
    case container::Severity::Info:
        return core::LogLevel::Info;
    case container::Severity::Trace:
        return core::LogLevel::Trace;
    }
    return core::LogLevel::Info;
}

}

container::LogSink logThroughTheCore()
{
    return [](container::Severity severity, std::string_view category, std::string_view message) {
        core::Logger::instance().log(levelFor(severity), category, message);
    };
}

bool MonolithPlugin::initialize(container::PluginContext &context, std::string &problem)
{
    static_cast<void>(context);
    static_cast<void>(problem);

    return true;
}

core::ConfigDocument settingsDocument(container::Settings &settings)
{
    core::ConfigDocument document;
    document.read = [&settings](std::string_view section) { return settings.section(section); };
    document.write = [&settings](std::string_view section, const std::string &text) {
        settings.setSection(section, text);
    };
    document.erase = [&settings](std::string_view section) { settings.removeSection(section); };

    document.sections = [&settings]() { return settings.owners(); };
    document.flush = [&settings]() {

        return settings.save() == container::SettingsStatus::Ok;
    };
    document.shownAs = settings.file();
    return document;
}

container::Manifest monolithManifest()
{
    container::Manifest manifest;
    manifest.id = std::string{kOwner};

    manifest.version =
        container::parseVersion(core::versionString()).value_or(container::Version{});

    manifest.name = "Application";
    manifest.summary = "Everything this product does, before it was cut into plugins";

    manifest.requirements.push_back({"storage", {}});

    manifest.required = true;
    manifest.ui = true;
    return manifest;
}

}
