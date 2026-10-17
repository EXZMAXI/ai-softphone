// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/crash.hpp>

#include "crash_detail.hpp"

#include <cstdlib>

namespace softphone::core {

bool writeDumpOfAnotherProcess(unsigned long, unsigned long, unsigned long long,
                               const std::string &) noexcept
{

    return false;
}

void reportAndEnd(std::string_view) noexcept
{

    ::_Exit(3);
}

bool installCrashHandler(const std::filesystem::path &)
{
    crashDetail::prepareBanner();

    crashDetail::rememberPreviousRun();
    return false;
}

}
