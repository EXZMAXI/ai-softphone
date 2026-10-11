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

#include <string>
#include <string_view>
#include <vector>

namespace softphone::capture {

struct KnownApplication {

    std::string process;

    std::string name;

    std::string venue;
};

struct Applications {
    std::vector<KnownApplication> known;

    std::vector<std::string> never;
};

[[nodiscard]] std::string plainly(std::string_view process);

[[nodiscard]] std::string_view shippedApplications();

[[nodiscard]] Applications readApplications(std::string_view text);

void useApplications(Applications table);

[[nodiscard]] std::string_view knownApplication(std::string_view process);

[[nodiscard]] std::string_view venueOf(std::string_view process);

[[nodiscard]] bool neverAConversation(std::string_view process);

[[nodiscard]] std::string nameFor(std::string_view process, std::string_view title);

}
