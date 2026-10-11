// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "list_edit.hpp"

#include <algorithm>

namespace softphone::ui {

std::optional<Removal> removalBetween(const std::vector<std::string> &was,
                                      const std::vector<std::string> &now)
{

    std::size_t front = 0;
    while (front < was.size() && front < now.size() && was[front] == now[front]) {
        ++front;
    }

    if (front == was.size()) {
        return std::nullopt;
    }

    const auto tail = was.size() - front;

    std::size_t howMany = tail;
    if (front < now.size()) {
        const auto found =
            std::find(was.begin() + static_cast<std::ptrdiff_t>(front) + 1, was.end(), now[front]);
        if (found == was.end()) {

            return std::nullopt;
        }
        howMany =
            static_cast<std::size_t>(found - (was.begin() + static_cast<std::ptrdiff_t>(front)));
    }

    const auto rest = tail - howMany;
    if (now.size() < front + rest) {
        return std::nullopt;
    }
    for (std::size_t at = 0; at < rest; ++at) {
        if (was[front + howMany + at] != now[front + at]) {
            return std::nullopt;
        }
    }

    return Removal{static_cast<int>(front), static_cast<int>(howMany),
                   static_cast<int>(now.size() - (front + rest))};
}

}
