// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/capture/detector.hpp>

#include <utility>

namespace softphone::capture {

ConversationGate::ConversationGate(DetectorSettings settings)
    : settings_{std::move(settings)}, talking_{settings_.conversing}
{}

void ConversationGate::setSettings(DetectorSettings settings)
{
    settings_ = std::move(settings);
}

ConversationGate::Move ConversationGate::saw(bool held, bool duplex, bool audible,
                                             std::chrono::steady_clock::time_point now)
{

    const bool conversing = held && duplex;

    if (!conversing) {
        heard_ = false;
    } else if (audible) {
        heard_ = true;
    }

    if (conversing == talking_) {

        pending_ = false;
        return Move::Nothing;
    }

    if (!pending_) {
        pending_ = true;
        since_ = now;
        return Move::Nothing;
    }

    const auto wanted = talking_ ? settings_.holdOff : settings_.settle;
    if (now - since_ < wanted) {
        return Move::Nothing;
    }

    if (conversing && !heard_) {
        return Move::Nothing;
    }

    talking_ = conversing;
    pending_ = false;
    return talking_ ? Move::Began : Move::Ended;
}

}
