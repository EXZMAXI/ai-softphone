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

#include <softphone/core/account_config.hpp>

#include <optional>
#include <string>

namespace softphone::plugins::softphone {

struct Refiling {
    std::string from;
    std::string to;

    [[nodiscard]] friend bool operator==(const Refiling &left, const Refiling &right) noexcept
    {
        return left.from == right.from && left.to == right.to;
    }
};

[[nodiscard]] inline std::optional<Refiling> refilingFor(const std::string &was,
                                                         const core::AccountConfig &now)
{
    const auto to = core::credentialName(now);
    if (was.empty() || to.empty() || was == to) {
        return std::nullopt;
    }
    return Refiling{was, to};
}

}
