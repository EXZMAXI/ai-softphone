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
#include <optional>
#include <string>
#include <vector>

namespace softphone::ui {

struct Removal {

    int at{0};

    int howMany{0};

    int appended{0};
};

[[nodiscard]] std::optional<Removal> removalBetween(const std::vector<std::string> &was,
                                                    const std::vector<std::string> &now);

}
