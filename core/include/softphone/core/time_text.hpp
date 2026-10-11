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

#include <chrono>
#include <ctime>
#include <string>

namespace softphone::core {

[[nodiscard]] std::tm utcParts(std::chrono::system_clock::time_point when) noexcept;

[[nodiscard]] std::tm localParts(std::chrono::system_clock::time_point when) noexcept;

[[nodiscard]] std::string isoUtc(std::chrono::system_clock::time_point when);

[[nodiscard]] std::string unixMilliseconds(std::chrono::system_clock::time_point when);

}
