// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/paths.hpp>

#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

namespace softphone::core {
namespace {

[[nodiscard]] const char *value(const char *name)
{
    const auto *found = std::getenv(name);
    return (found != nullptr && *found != '\0') ? found : nullptr;
}

[[nodiscard]] std::filesystem::path under(const char *variable,
                                          const std::filesystem::path &fallback, const char *kind)
{
    if (const auto *profile = value(std::string{kProfileVariable}.c_str())) {
        return std::filesystem::path{profile} / kind;
    }
    if (const auto *home = value(variable)) {
        return std::filesystem::path{home} / "ai-softphone";
    }
    if (const auto *home = value("HOME")) {
        return std::filesystem::path{home} / fallback / "ai-softphone";
    }
    return std::filesystem::path{".ai-softphone"};
}

}

std::filesystem::path configDirectory()
{
    return under("XDG_CONFIG_HOME", ".config", "config");
}

std::filesystem::path dataDirectory()
{
    return under("XDG_DATA_HOME", std::filesystem::path{".local"} / "share", "data");
}

std::filesystem::path stateDirectory()
{
    return under("XDG_STATE_HOME", std::filesystem::path{".local"} / "state", "state");
}

std::filesystem::path cacheDirectory()
{
    return under("XDG_CACHE_HOME", ".cache", "cache");
}

std::filesystem::path recordingsDirectory()
{

    return dataDirectory() / "recordings";
}

std::string forNarrowFileApi(const std::filesystem::path &file)
{

    return file.string();
}

bool restrictToOwner(const std::filesystem::path &file)
{
    std::error_code error;
    std::filesystem::permissions(
        file, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write, error);
    return !error;
}

}
