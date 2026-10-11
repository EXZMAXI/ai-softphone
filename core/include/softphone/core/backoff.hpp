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

namespace softphone::core {

struct BackoffPolicy {
    std::chrono::milliseconds initial{std::chrono::seconds{2}};
    std::chrono::milliseconds max{std::chrono::minutes{5}};
    double multiplier{2.0};

    double jitterRatio{0.2};
};

using JitterSource = std::function<double()>;

[[nodiscard]] JitterSource defaultJitterSource();

[[nodiscard]] std::chrono::milliseconds backoffDelay(const BackoffPolicy &policy, unsigned attempt,
                                                     double jitterSample) noexcept;

}
