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

#include <cstddef>
#include <span>
#include <string>

namespace softphone::capture {

struct Device {

    std::string id;
    std::string name;

    bool console{false};
    bool communications{false};

    bool held{false};

    float peak{0.0F};
};

inline constexpr float kAudible = 0.0005F;

inline constexpr std::size_t kNoDevice = static_cast<std::size_t>(-1);

[[nodiscard]] std::size_t theOneCarryingSound(std::span<const Device> outputs);

[[nodiscard]] std::size_t theOneBeingListenedTo(std::span<const Device> inputs);

}
