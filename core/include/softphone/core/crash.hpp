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
#include <optional>
#include <string>
#include <string_view>

namespace softphone::core {

struct CrashReport {

    std::filesystem::path report;

    std::filesystem::path dump;

    std::string when;
};

bool installCrashHandler(const std::filesystem::path &directory);

void leaveBreadcrumb(std::string_view text) noexcept;

[[nodiscard]] std::optional<CrashReport> lastRunCrashed();

void markCleanExit() noexcept;

void discardCrashReport(const CrashReport &report) noexcept;

inline constexpr std::string_view kWriteCrashDumpOption = "--write-crash-dump";

[[nodiscard]] std::optional<int> answeredCrashDumpRequest(int argc, char **argv);

[[nodiscard]] bool writeDumpOfAnotherProcess(unsigned long processId, unsigned long threadId,
                                             unsigned long long exceptionPointers,
                                             const std::string &dumpFile) noexcept;

void writeCrashReport(int descriptor, std::string_view reason) noexcept;

[[noreturn]] void reportAndEnd(std::string_view reason) noexcept;

}
