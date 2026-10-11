// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/capture/control.hpp>

#include <softphone/capture/applications.hpp>

#include <algorithm>

namespace softphone::plugins::capture {

OnFinding policyFor(OnFinding fallback, const std::vector<ApplicationPolicy> &exceptions,
                    std::string_view process)
{
    if (process.empty()) {
        return fallback;
    }

    const auto plain = cap::plainly(process);
    const auto found = std::ranges::find_if(exceptions, [&plain](const ApplicationPolicy &one) {
        return cap::plainly(one.process) == plain;
    });
    return found == exceptions.end() ? fallback : found->answer;
}

std::vector<std::string> neverReport(OnFinding fallback,
                                     const std::vector<ApplicationPolicy> &exceptions)
{

    if (fallback == OnFinding::Never) {
        return {};
    }

    std::vector<std::string> never;
    for (const ApplicationPolicy &one : exceptions) {
        if (one.answer == OnFinding::Never && !one.process.empty()) {
            never.push_back(one.process);
        }
    }
    return never;
}

}
