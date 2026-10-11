// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "tray_presentation.hpp"

namespace softphone::ui {
namespace {

[[nodiscard]] bool over(const core::CallInfo &call) noexcept
{

    return call.state == core::CallState::Ended || call.state == core::CallState::Idle;
}

const core::CallInfo *inFront(std::span<const core::CallInfo> calls)
{
    const core::CallInfo *parked = nullptr;
    const core::CallInfo *dialling = nullptr;

    for (const auto &call : calls) {
        if (over(call) || call.state == core::CallState::RingingIn) {
            continue;
        }

        if (call.state != core::CallState::Active) {
            if (dialling == nullptr) {
                dialling = &call;
            }
            continue;
        }

        if (!call.heldByUs && !call.heldByThem) {
            return &call;
        }
        if (parked == nullptr) {
            parked = &call;
        }
    }

    return parked != nullptr ? parked : dialling;
}

}

TrayPresentation trayPresentation(std::span<const TrayLine> lines,
                                  std::span<const core::CallInfo> calls, int recordings,
                                  bool missed)
{
    TrayPresentation shown;
    shown.recordings = recordings;
    shown.missed = missed;

    for (const auto &line : lines) {

        if (!line.enabled) {
            continue;
        }

        ++shown.lines;

        switch (line.state) {
        case core::RegistrationState::Registered:
            ++shown.connected;
            break;
        case core::RegistrationState::Failed:
            ++shown.failed;
            if (shown.lineLabel.empty()) {
                shown.lineLabel = line.label;
            }
            break;
        case core::RegistrationState::Offline:
        case core::RegistrationState::Registering:
        case core::RegistrationState::Unregistering:
            break;
        }
    }

    const core::CallInfo *ringing = nullptr;

    for (const auto &call : calls) {
        if (!over(call) && call.state == core::CallState::RingingIn) {
            ringing = &call;
            break;
        }
    }

    const auto *front = inFront(calls);

    shown.ringing = ringing != nullptr;
    shown.inCall = front != nullptr && front->state == core::CallState::Active;

    if (front != nullptr) {
        shown.muted = front->muted;
        shown.held = front->heldByUs;
    }

    shown.canAnswer = shown.ringing;
    shown.canReject = shown.ringing;
    shown.canHangUp = front != nullptr;
    shown.canMute = shown.inCall;

    if (ringing != nullptr) {
        shown.callLabel = ringing->remoteLabel();
    } else if (front != nullptr) {
        shown.callLabel = front->remoteLabel();
    }

    if (shown.ringing) {
        shown.status = TrayStatus::Ringing;
    } else if (shown.connected > 0 && shown.connected == shown.lines) {
        shown.status = TrayStatus::Connected;
    } else if (shown.connected > 0) {
        shown.status = TrayStatus::Degraded;
    } else if (shown.failed > 0) {
        shown.status = TrayStatus::Failing;
    } else {
        shown.status = TrayStatus::Offline;
    }

    return shown;
}

}
