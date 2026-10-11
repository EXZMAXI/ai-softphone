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

#include <softphone/core/ringtones.hpp>
#include <softphone/core/tones.hpp>

#include <string>

namespace softphone::ui {

[[nodiscard]] std::string toneFile(core::CallTone tone, const core::RingtoneChoice &ringtone);

[[nodiscard]] std::string ringtoneFile(const core::RingtoneChoice &ringtone);

[[nodiscard]] std::string testSoundFile();

[[nodiscard]] std::string dtmfFile(char digit);

[[nodiscard]] std::string noticeFile(const std::string &chosen);

[[nodiscard]] std::string recordingToneFile();

[[nodiscard]] std::string shippedNoticeFile();

[[nodiscard]] bool toneLoops(core::CallTone tone) noexcept;

}
