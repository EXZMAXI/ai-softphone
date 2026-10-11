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
#include <functional>
#include <string>
#include <string_view>

namespace softphone::core {

namespace exchange {

constexpr std::chrono::minutes kKeptFor{60};

constexpr std::size_t kRoomFor = 16U * 1024U * 1024U;

void keep(std::string_view artifactId, int part, int of, std::string_view request,
          std::string_view response);

[[nodiscard]] std::string readBack(std::string_view artifactId);

[[nodiscard]] bool kept(std::string_view artifactId);

void sweep();

void forgetAll();

using Clock = std::function<std::chrono::steady_clock::time_point()>;
void setClock(Clock clock);

}
}
