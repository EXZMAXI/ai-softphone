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

namespace softphone::core {

namespace hosts {

[[nodiscard]] std::string siteFor(std::string_view country);

[[nodiscard]] std::string docsFor(std::string_view country);

[[nodiscard]] std::string docsPageFor(std::string_view country, std::string_view language);

[[nodiscard]] std::vector<std::string> telemetryHostsFor(std::string_view country);

[[nodiscard]] bool anythingFor(std::string_view country);

}

}
