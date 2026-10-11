// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/backoff.hpp>

#include <algorithm>
#include <random>

namespace softphone::core {

JitterSource defaultJitterSource()
{
    return []() {
        static thread_local std::mt19937 engine{std::random_device{}()};
        std::uniform_real_distribution<double> distribution{0.0, 1.0};
        return distribution(engine);
    };
}

std::chrono::milliseconds backoffDelay(const BackoffPolicy &policy, unsigned attempt,
                                       double jitterSample) noexcept
{
    using Rep = std::chrono::milliseconds::rep;

    const auto cap = static_cast<double>(policy.max.count());
    double delay = static_cast<double>(policy.initial.count());

    for (unsigned i = 0; i < attempt && delay < cap; ++i) {
        delay *= policy.multiplier;
    }
    delay = std::min(delay, cap);

    const auto sample = std::clamp(jitterSample, 0.0, 1.0);
    delay *= 1.0 + (policy.jitterRatio * ((2.0 * sample) - 1.0));
    delay = std::clamp(delay, 0.0, cap);

    return std::chrono::milliseconds{static_cast<Rep>(delay)};
}

}
