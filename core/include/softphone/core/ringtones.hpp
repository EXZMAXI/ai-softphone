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
#include <span>
#include <string>
#include <string_view>

namespace softphone::core {

struct BundledRingtone {

    std::string_view id;

    std::string_view file;
};

[[nodiscard]] std::span<const BundledRingtone> bundledRingtones() noexcept;

[[nodiscard]] std::string_view defaultRingtone() noexcept;

[[nodiscard]] std::string_view bundledRingtoneFile(std::string_view id) noexcept;

struct RingtoneChoice {

    std::string bundled{std::string{defaultRingtone()}};

    std::string file;

    [[nodiscard]] bool ownFile() const noexcept
    {
        return bundled.empty() && !file.empty();
    }

    friend bool operator==(const RingtoneChoice &, const RingtoneChoice &) = default;
};

enum class RingtoneFault {
    None,

    Missing,

    NotAFile,

    Unreadable,

    NotAWave,

    Unsupported,

    Empty,
};

[[nodiscard]] RingtoneFault inspectRingtoneFile(const std::filesystem::path &file);

}
