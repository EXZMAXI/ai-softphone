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

#include <thread>
#include <utility>

namespace softphone::core {

class JoiningThread {
public:
    JoiningThread() noexcept = default;

    template<typename Function, typename... Arguments>
    explicit JoiningThread(Function &&function, Arguments &&...arguments)
        : thread_{std::forward<Function>(function), std::forward<Arguments>(arguments)...}
    {}

    JoiningThread(const JoiningThread &) = delete;
    JoiningThread &operator=(const JoiningThread &) = delete;

    JoiningThread(JoiningThread &&other) noexcept = default;

    JoiningThread &operator=(JoiningThread &&other) noexcept
    {
        if (this != &other) {
            join();
            thread_ = std::move(other.thread_);
        }
        return *this;
    }

    ~JoiningThread()
    {
        join();
    }

    [[nodiscard]] bool joinable() const noexcept
    {
        return thread_.joinable();
    }

    void join() noexcept
    {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

private:
    std::thread thread_;
};

}
