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
#include <system_error>

namespace softphone::core {
namespace {

[[nodiscard]] std::filesystem::path library(const std::filesystem::path &what)
{

    if (const auto *asked = std::getenv("SOFTPHONE_PROFILE"); asked != nullptr && *asked != '\0') {
        return std::filesystem::path{asked} / what / "ai-softphone";
    }

    const auto *home = std::getenv("HOME");
    if (home == nullptr || *home == '\0') {
        return std::filesystem::path{".ai-softphone"};
    }
    return std::filesystem::path{home} / "Library" / what / "ai-softphone";
}

}

std::filesystem::path configDirectory()
{
    return library("Preferences");
}

std::filesystem::path dataDirectory()
{
    return library("Application Support");
}

std::filesystem::path stateDirectory()
{
    return library("Logs");
}

std::filesystem::path cacheDirectory()
{
    return library("Caches");
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
