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

#include <string>
#include <string_view>

namespace softphone::core {

[[nodiscard]] std::string asText(const AppearanceSettings &settings);
[[nodiscard]] AppearanceSettings appearanceFrom(std::string_view text);

[[nodiscard]] std::string asText(const UpdateSettings &settings);
[[nodiscard]] UpdateSettings updatesFrom(std::string_view text);

[[nodiscard]] std::string asText(const RegionSettings &settings);
[[nodiscard]] RegionSettings regionFrom(std::string_view text);

[[nodiscard]] std::string asText(const TelemetrySettings &settings);
[[nodiscard]] TelemetrySettings telemetryFrom(std::string_view text);

[[nodiscard]] std::string asText(const StartupSettings &settings);
[[nodiscard]] StartupSettings startupFrom(std::string_view text);

[[nodiscard]] std::string asText(const HotkeySettings &settings);
[[nodiscard]] HotkeySettings hotkeysFrom(std::string_view text);

[[nodiscard]] std::string asText(const DiagnosticsSettings &settings);
[[nodiscard]] DiagnosticsSettings diagnosticsFrom(std::string_view text);

[[nodiscard]] std::string asText(const ButtonSettings &settings);
[[nodiscard]] ButtonSettings buttonsFrom(std::string_view text);

[[nodiscard]] std::string asText(const WebhookSettings &settings);
[[nodiscard]] WebhookSettings webhooksFrom(std::string_view text);

[[nodiscard]] std::string asText(const RecordingSettings &settings);
[[nodiscard]] RecordingSettings recordingFrom(std::string_view text);

[[nodiscard]] std::string asText(const ApiSettings &settings);
[[nodiscard]] ApiSettings apiFrom(std::string_view text);

}
