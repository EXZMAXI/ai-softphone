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
#include <cstdint>
#include <functional>

namespace softphone::core {

class IScheduler {
public:
    using TimerId = std::uint64_t;
    static constexpr TimerId kNoTimer = 0;

    IScheduler() = default;
    IScheduler(const IScheduler &) = delete;
    IScheduler &operator=(const IScheduler &) = delete;
    IScheduler(IScheduler &&) = delete;
    IScheduler &operator=(IScheduler &&) = delete;
    virtual ~IScheduler() = default;

    [[nodiscard]] virtual std::chrono::steady_clock::time_point now() const = 0;

    [[nodiscard]] virtual TimerId scheduleAfter(std::chrono::milliseconds delay,
                                                std::function<void()> callback) = 0;

    virtual void cancel(TimerId id) = 0;
};

}
