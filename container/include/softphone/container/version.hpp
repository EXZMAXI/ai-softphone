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

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace softphone::container {

struct Version {
    int major{0};
    int minor{0};
    int patch{0};

    [[nodiscard]] friend auto operator<=>(const Version &, const Version &) = default;
    [[nodiscard]] friend bool operator==(const Version &, const Version &) = default;
};

[[nodiscard]] std::optional<Version> parseVersion(std::string_view text);

[[nodiscard]] std::string toString(const Version &version);

struct VersionRange {

    std::optional<Version> atLeast;

    std::optional<Version> below;

    [[nodiscard]] bool allows(const Version &version) const;

    [[nodiscard]] bool unbounded() const noexcept
    {
        return !atLeast.has_value() && !below.has_value();
    }
};

[[nodiscard]] std::optional<VersionRange> parseVersionRange(std::string_view text);

[[nodiscard]] std::string toString(const VersionRange &range);

}
