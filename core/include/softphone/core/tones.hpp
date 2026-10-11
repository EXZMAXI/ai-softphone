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

#include <span>

namespace softphone::core {

enum class CallTone {
    None,

    Ringtone,

    Ringback,

    Disconnect,

    AutoAnswered,

    Busy,
};

[[nodiscard]] CallTone continuousTone(std::span<const CallInfo> calls) noexcept;

[[nodiscard]] bool ringsHere(const CallInfo &call) noexcept;

enum class ToneOutput {

    Speakers,

    Ringer,
};

[[nodiscard]] ToneOutput toneOutput(CallTone tone) noexcept;

enum class ToneLevel {

    Conversation,

    Ring,

    Notifications,
};

[[nodiscard]] ToneLevel toneLevel(CallTone tone) noexcept;

[[nodiscard]] bool testSoundAllowed(std::span<const CallInfo> calls) noexcept;

[[nodiscard]] bool endingIsAudible(const CallInfo &call) noexcept;

[[nodiscard]] bool answerIsAudible(const CallInfo &call) noexcept;

[[nodiscard]] bool failureIsAudible(const CallInfo &call) noexcept;

}
