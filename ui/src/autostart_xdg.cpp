// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "autostart_xdg.hpp"

#include <softphone/core/logging.hpp>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>

namespace softphone::ui {
namespace {

constexpr auto kEntryName = "ai-softphone.desktop";

[[nodiscard]] const char *environmentValue(const char *name)
{
    const auto *value = std::getenv(name);
    return (value != nullptr && *value != '\0') ? value : nullptr;
}

}

std::filesystem::path defaultAutostartDirectory()
{
    if (const auto *configHome = environmentValue("XDG_CONFIG_HOME")) {
        return std::filesystem::path{configHome} / "autostart";
    }
    if (const auto *home = environmentValue("HOME")) {
        return std::filesystem::path{home} / ".config" / "autostart";
    }
    return std::filesystem::path{".config"} / "autostart";
}

std::filesystem::path runningBinary()
{
    std::error_code error;
    auto resolved = std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error) {
        return resolved;
    }
    return {};
}

XdgAutostart::XdgAutostart(std::filesystem::path directory, std::filesystem::path binary)
    : directory_{std::move(directory)}, binary_{std::move(binary)}
{}

std::filesystem::path XdgAutostart::entryPath() const
{
    return directory_ / kEntryName;
}

AutostartState XdgAutostart::state() const
{
    std::ifstream file{entryPath()};
    if (!file) {
        return {};
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    return readAutostartEntry(contents.str());
}

bool XdgAutostart::enable(bool startMinimised)
{
    if (binary_.empty()) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot start with the session: this process cannot say "
                                     "where its own binary is");
        return false;
    }

    std::error_code error;
    std::filesystem::create_directories(directory_, error);
    if (error) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot create " + directory_.string() + ": " +
                                         error.message());
        return false;
    }

    std::ofstream file{entryPath(), std::ios::trunc};
    if (!file) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot write " + entryPath().string());
        return false;
    }

    file << autostartEntry(binary_.string(), startMinimised);
    if (!file) {
        return false;
    }

    core::Logger::instance().log(core::LogLevel::Info, "ui",
                                 "the session will start " + binary_.string() +
                                     (startMinimised ? " minimised" : ""));
    return true;
}

bool XdgAutostart::disable()
{
    std::error_code error;
    std::filesystem::remove(entryPath(), error);
    if (error) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot remove " + entryPath().string() + ": " +
                                         error.message());
        return false;
    }
    return true;
}

std::unique_ptr<IAutostart> makeAutostart()
{
    return std::make_unique<XdgAutostart>();
}

}
