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

#include <softphone/core/call.hpp>
#include <softphone/core/registration.hpp>

#include <span>
#include <string>
#include <vector>

namespace softphone::ui {

enum class TrayStatus {

    Offline,

    Degraded,

    Failing,

    Connected,

    Ringing,
};

struct TrayLine {
    std::string label;
    bool enabled{true};
    core::RegistrationState state{core::RegistrationState::Offline};

    friend bool operator==(const TrayLine &, const TrayLine &) = default;
};

struct TrayPresentation {
    TrayStatus status{TrayStatus::Offline};

    int lines{0};
    int connected{0};
    int failed{0};

    bool ringing{false};
    bool inCall{false};

    int recordings{0};

    bool missed{false};

    bool muted{false};
    bool held{false};

    std::string callLabel;

    std::string lineLabel;

    bool canAnswer{false};
    bool canReject{false};
    bool canHangUp{false};
    bool canMute{false};

    friend bool operator==(const TrayPresentation &, const TrayPresentation &) = default;
};

[[nodiscard]] TrayPresentation trayPresentation(std::span<const TrayLine> lines,
                                                std::span<const core::CallInfo> calls,
                                                int recordings = 0, bool missed = false);

}
