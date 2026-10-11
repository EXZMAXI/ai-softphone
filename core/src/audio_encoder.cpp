// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/audio_encoder.hpp>

namespace softphone::core {

std::string_view toString(EncoderStatus status) noexcept
{
    switch (status) {
    case EncoderStatus::Ok:
        return "ok";
    case EncoderStatus::Unwritable:
        return "unwritable";
    case EncoderStatus::Failed:
        return "failed";
    case EncoderStatus::NoRoom:
        return "no-room";
    }
    return "failed";
}

}
