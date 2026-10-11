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

#include "autostart.hpp"

#include <filesystem>
#include <string>

namespace softphone::ui {

inline constexpr auto kRunKey = R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Run)";

inline constexpr auto kApprovedKey =
    R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run)";

inline constexpr auto kRunValue = "AI Softphone";

class RunKeyAutostart final : public IAutostart {
public:

    explicit RunKeyAutostart(std::string key = kRunKey,
                             std::filesystem::path binary = runningBinary());

    [[nodiscard]] AutostartState state() const override;
    [[nodiscard]] bool enable(bool startMinimised) override;
    [[nodiscard]] bool disable() override;

    [[nodiscard]] std::string command() const;

private:
    std::string key_;
    std::filesystem::path binary_;
};

}
