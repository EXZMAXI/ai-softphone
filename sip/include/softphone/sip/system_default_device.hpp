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

#include <softphone/core/audio_devices.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>

namespace softphone::sip {

[[nodiscard]] std::optional<std::string> systemDefaultDeviceName(core::DeviceRole role);

[[nodiscard]] std::optional<std::size_t>
positionOfDefault(std::span<const core::AudioDeviceInfo> devices, const std::string &name,
                  core::DeviceRole role);

class DeviceWatch {
public:
    DeviceWatch() = default;
    DeviceWatch(const DeviceWatch &) = delete;
    DeviceWatch &operator=(const DeviceWatch &) = delete;
    DeviceWatch(DeviceWatch &&) = delete;
    DeviceWatch &operator=(DeviceWatch &&) = delete;
    virtual ~DeviceWatch() = default;
};

[[nodiscard]] std::unique_ptr<DeviceWatch> watchTheMachinesDevices(std::function<void()> moved);

[[nodiscard]] std::optional<std::size_t>
systemDefaultPosition(std::span<const core::AudioDeviceInfo> devices, core::DeviceRole role);

}
