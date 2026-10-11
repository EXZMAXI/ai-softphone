// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "session_away.hpp"

#include <chrono>

namespace softphone::ui {

WhenAway answerTo(platform::Away what)
{
    WhenAway answer;
    switch (what) {
    case platform::Away::Sleeping:
        answer.closeTheFile = true;
        return answer;

    case platform::Away::Locked:

        answer.patience.quiet = std::chrono::minutes{1};
        return answer;

    case platform::Away::Unlocked:

        return answer;
    }
    return answer;
}

}
