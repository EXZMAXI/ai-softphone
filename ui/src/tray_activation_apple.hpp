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

#include <string_view>

namespace softphone::ui {

[[nodiscard]] bool clickCountIsRead(unsigned long eventType) noexcept;

enum class TrayGuard {

    installed,

    noCocoaPlugin,

    qtHasMoved,
};

[[nodiscard]] std::string_view toString(TrayGuard found) noexcept;

TrayGuard keepTheTrayFromAborting();

}
