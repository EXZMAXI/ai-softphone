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

#include <softphone/core/logging.hpp>

#include <chrono>
#include <mutex>
#include <string>
#include <string_view>

namespace softphone::sip {

class CancellerStarvation {
public:

    [[nodiscard]] static bool counted(std::string_view text)
    {
        if (text.find("(your application is buggy and/or got xruns)") == std::string_view::npos) {
            return false;
        }

        std::string summary;
        {
            const std::lock_guard<std::mutex> held{lock()};
            auto &since = begun();
            auto &many = count();
            const auto now = std::chrono::steady_clock::now();
            if (many == 0) {
                since = now;
            }
            ++many;
            if (now - since >= std::chrono::minutes{1}) {
                summary = sentence(many, now - since);
                many = 0;
            }
        }
        if (!summary.empty()) {
            core::Logger::instance().log(core::LogLevel::Info, "pjsip", summary);
        }
        return true;
    }

    static void flush()
    {
        std::string summary;
        {
            const std::lock_guard<std::mutex> held{lock()};
            auto &many = count();
            if (many == 0) {
                return;
            }
            summary = sentence(many, std::chrono::steady_clock::now() - begun());
            many = 0;
        }
        core::Logger::instance().log(core::LogLevel::Info, "pjsip", summary);
    }

private:
    [[nodiscard]] static std::string sentence(unsigned many,
                                              std::chrono::steady_clock::duration over)
    {
        const auto seconds =
            std::chrono::duration_cast<std::chrono::duration<double>>(over).count();
        auto rate = std::to_string(seconds > 0.1 ? static_cast<double>(many) / seconds : 0.0);
        rate.resize(rate.find('.') + 2);
        return "the echo canceller was starved " + std::to_string(many) + " times in " +
               std::to_string(static_cast<int>(seconds + 0.5)) + " s (" + rate +
               " a second): playback and capture are not reaching it one frame for one";
    }

    [[nodiscard]] static std::mutex &lock()
    {
        static std::mutex one;
        return one;
    }

    [[nodiscard]] static unsigned &count()
    {
        static unsigned many{0};
        return many;
    }

    [[nodiscard]] static std::chrono::steady_clock::time_point &begun()
    {
        static std::chrono::steady_clock::time_point when{};
        return when;
    }
};

}
