// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "autostart_apple.hpp"

#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>

#include <mach-o/dyld.h>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>
#include <vector>

namespace softphone::ui {
namespace {

[[nodiscard]] std::string entryName()
{
    return std::string{kLaunchAgentLabel} + ".plist";
}

}

std::filesystem::path defaultLaunchAgentDirectory()
{

    if (const auto *profile = std::getenv("SOFTPHONE_PROFILE");
        profile != nullptr && *profile != '\0') {
        return std::filesystem::path{profile} / "Library" / "LaunchAgents";
    }
    if (const auto *home = std::getenv("HOME"); home != nullptr && *home != '\0') {
        return std::filesystem::path{home} / "Library" / "LaunchAgents";
    }
    return std::filesystem::path{"Library"} / "LaunchAgents";
}

std::filesystem::path runningBinary()
{

    std::uint32_t size = 0;
    static_cast<void>(_NSGetExecutablePath(nullptr, &size));
    if (size == 0) {
        return {};
    }

    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        return {};
    }

    const std::filesystem::path started{core::fromUtf8(std::string{buffer.data()})};
    std::error_code error;
    auto resolved = std::filesystem::canonical(started, error);
    return error ? started : resolved;
}

AppleAutostart::AppleAutostart(std::filesystem::path directory, std::filesystem::path binary)
    : directory_{std::move(directory)}, binary_{std::move(binary)}
{}

std::filesystem::path AppleAutostart::entryPath() const
{
    return directory_ / entryName();
}

AutostartState AppleAutostart::state() const
{
    std::ifstream file{entryPath()};
    if (!file) {
        return {};
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    return readLaunchAgentPlist(contents.str());
}

bool AppleAutostart::enable(bool startMinimised)
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
                                     "cannot create " + core::asUtf8(directory_) + ": " +
                                         error.message());
        return false;
    }

    std::ofstream file{entryPath(), std::ios::trunc};
    if (!file) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot write " + core::asUtf8(entryPath()));
        return false;
    }

    file << launchAgentPlist(core::asUtf8(binary_), startMinimised);
    if (!file) {
        return false;
    }

    core::Logger::instance().log(core::LogLevel::Info, "ui",
                                 "the session will start " + core::asUtf8(binary_) +
                                     (startMinimised ? " minimised" : ""));
    return true;
}

bool AppleAutostart::disable()
{
    std::error_code error;
    std::filesystem::remove(entryPath(), error);
    if (error) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot remove " + core::asUtf8(entryPath()) + ": " +
                                         error.message());
        return false;
    }
    return true;
}

std::unique_ptr<IAutostart> makeAutostart()
{
    return std::make_unique<AppleAutostart>();
}

}
