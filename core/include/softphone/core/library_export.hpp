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

#include <softphone/core/recordings.hpp>
#include <softphone/core/taxonomy.hpp>

#include <span>
#include <string>
#include <vector>

namespace softphone::core {

struct LibraryColumns {
    std::string when{"When"};
    std::string direction{"Direction"};
    std::string party{"Party"};
    std::string number{"Number"};
    std::string account{"Account"};
    std::string seconds{"Seconds"};
    std::string category{"Category"};
    std::string tags{"Tags"};
    std::string redFlags{"Flags"};
    std::string language{"Language"};
    std::string summary{"Summary"};

    std::string incoming{"in"};
    std::string outgoing{"out"};
};

[[nodiscard]] std::string renderLibraryCsv(std::span<const Recording> rows,
                                           const LibraryColumns &columns);

[[nodiscard]] std::string csvField(std::string_view value);

}
