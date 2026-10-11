// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/store.hpp>

namespace softphone::core {

std::string_view toString(StoreStatus status) noexcept
{
    switch (status) {
    case StoreStatus::Ok:
        return "ok";
    case StoreStatus::Missing:
        return "missing";
    case StoreStatus::Unavailable:
        return "unavailable";
    case StoreStatus::Unwritable:
        return "unwritable";
    case StoreStatus::Malformed:
        return "malformed";
    case StoreStatus::UnsupportedSchema:
        return "unsupported-schema";
    case StoreStatus::Busy:
        return "busy";
    case StoreStatus::Failed:
        return "failed";
    }
    return "unknown";
}

}
