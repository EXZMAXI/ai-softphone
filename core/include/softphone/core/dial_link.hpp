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

#include <array>
#include <string>
#include <string_view>

namespace softphone::core {

inline constexpr std::array<std::string_view, 4> kDialSchemes{"tel", "callto", "sip", "sips"};

struct DialLink {

    bool ours{false};

    std::string target;

    friend bool operator==(const DialLink &, const DialLink &) = default;
};

[[nodiscard]] DialLink dialLink(std::string_view uri);

[[nodiscard]] std::string percentDecoded(std::string_view text);

}
