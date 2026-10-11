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

#include <filesystem>
#include <string>
#include <string_view>

namespace softphone::core {

[[nodiscard]] std::filesystem::path configDirectory();

[[nodiscard]] std::filesystem::path dataDirectory();

[[nodiscard]] std::filesystem::path stateDirectory();

[[nodiscard]] std::filesystem::path cacheDirectory();

[[nodiscard]] std::filesystem::path recordingsDirectory();

struct ChosenLocations {

    std::filesystem::path profile;

    std::filesystem::path recordings;
};

[[nodiscard]] ChosenLocations locationsIn(std::string_view document);

inline constexpr std::string_view kProfileVariable = "SOFTPHONE_PROFILE";

inline constexpr std::string_view kLocationsFile = "paths.json";

[[nodiscard]] bool restrictToOwner(const std::filesystem::path &file);

[[nodiscard]] std::string asUtf8(const std::filesystem::path &file);

[[nodiscard]] std::string readablePath(const std::filesystem::path &file);

[[nodiscard]] std::filesystem::path fromUtf8(std::string_view text);

[[nodiscard]] std::string forNarrowFileApi(const std::filesystem::path &file);

}
