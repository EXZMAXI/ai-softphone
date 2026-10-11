// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/capture/system_capture.hpp>

namespace softphone::capture {

std::string_view toString(Side side) noexcept
{
    return side == Side::Near ? "near" : "far";
}

std::string_view toString(CaptureError error) noexcept
{
    switch (error) {
    case CaptureError::None:
        return "none";
    case CaptureError::NoBackend:
        return "no-backend";
    case CaptureError::NoServer:
        return "no-server";
    case CaptureError::NoDevice:
        return "no-device";
    case CaptureError::Refused:
        return "refused";
    }
    return "none";
}

}
