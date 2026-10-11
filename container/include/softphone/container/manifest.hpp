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

#include <softphone/container/version.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace softphone::container {

struct Dependency {
    std::string id;

    VersionRange versions;
};

struct Manifest {

    std::string id;

    Version version;

    std::string name;
    std::string summary;

    std::vector<Dependency> requirements;

    std::vector<Dependency> suggestions;

    bool required{false};

    bool ui{false};
};

enum class ManifestProblem {
    None,

    Malformed,
    NoId,

    BadId,
    NoVersion,
    BadVersion,

    BadDependency,

    DependsOnItself,

    DependencyTwice,
};

[[nodiscard]] std::string_view toString(ManifestProblem problem) noexcept;

[[nodiscard]] bool isValidPluginId(std::string_view id) noexcept;

[[nodiscard]] ManifestProblem parseManifest(std::string_view text, Manifest &out);

[[nodiscard]] std::string toJson(const Manifest &manifest);

}
