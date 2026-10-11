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

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace softphone::ui {

struct AutostartState {

    bool present{false};

    bool enabled{false};

    bool startMinimised{false};

    std::string command;

    friend bool operator==(const AutostartState &, const AutostartState &) = default;
};

inline constexpr std::string_view kMinimisedArgument = "--minimised";

[[nodiscard]] std::string autostartEntry(const std::string &binary, bool startMinimised);

[[nodiscard]] AutostartState readAutostartEntry(std::string_view text);

[[nodiscard]] std::string runCommand(const std::string &binary, bool startMinimised);

[[nodiscard]] AutostartState readRunCommand(std::string_view text);

inline constexpr std::string_view kLaunchAgentLabel = SOFTPHONE_BUNDLE_IDENTIFIER;

[[nodiscard]] std::string launchAgentPlist(const std::string &binary, bool startMinimised);

[[nodiscard]] AutostartState readLaunchAgentPlist(std::string_view text);

class IAutostart {
public:
    IAutostart() = default;
    IAutostart(const IAutostart &) = delete;
    IAutostart &operator=(const IAutostart &) = delete;
    IAutostart(IAutostart &&) = delete;
    IAutostart &operator=(IAutostart &&) = delete;
    virtual ~IAutostart() = default;

    [[nodiscard]] virtual AutostartState state() const = 0;

    [[nodiscard]] virtual bool enable(bool startMinimised) = 0;

    [[nodiscard]] virtual bool disable() = 0;
};

[[nodiscard]] std::filesystem::path runningBinary();

[[nodiscard]] std::unique_ptr<IAutostart> makeAutostart();

}
