// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/sip/system_default_device.hpp>

namespace softphone::sip {

std::optional<std::size_t> positionOfDefault(std::span<const core::AudioDeviceInfo> devices,
                                             const std::string &name, core::DeviceRole role)
{
    if (name.empty()) {
        return std::nullopt;
    }

    for (std::size_t at = 0; at < devices.size(); ++at) {
        const auto &device = devices[at];
        if (device.name != name) {
            continue;
        }

        const bool suits =
            role == core::DeviceRole::Capture ? device.canCapture() : device.canPlay();
        if (suits) {
            return at;
        }
    }

    return std::nullopt;
}

std::optional<std::size_t> systemDefaultPosition(std::span<const core::AudioDeviceInfo> devices,
                                                 core::DeviceRole role)
{
    const auto name = systemDefaultDeviceName(role);
    if (!name) {
        return std::nullopt;
    }
    return positionOfDefault(devices, *name, role);
}

}
