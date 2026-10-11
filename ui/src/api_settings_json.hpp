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

#include <softphone/core/settings.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace softphone::ui {

[[nodiscard]] std::string settingsToJson(const core::StoredConfiguration &configuration);

[[nodiscard]] std::optional<core::StoredConfiguration>
settingsFromJson(std::string_view body, const core::StoredConfiguration &current,
                 std::string &problem);

}
