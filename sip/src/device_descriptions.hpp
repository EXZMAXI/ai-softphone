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

#include <map>
#include <span>
#include <string>
#include <vector>

namespace softphone::sip {

struct SoundServerNode {
    std::string card;
    unsigned device{0};
    core::DeviceRole role{core::DeviceRole::Capture};
    std::string description;
};

struct AudioEndpoint {
    std::string name;
    core::DeviceRole role{core::DeviceRole::Capture};
};

[[nodiscard]] std::vector<core::AudioDeviceInfo>
describedWith(std::vector<core::AudioDeviceInfo> devices, std::span<const SoundServerNode> nodes,
              const std::map<std::string, std::string> &alsaWords);

[[nodiscard]] std::vector<core::AudioDeviceInfo>
describedWithEndpoints(std::vector<core::AudioDeviceInfo> devices,
                       std::span<const AudioEndpoint> endpoints);

[[nodiscard]] std::string asText(std::string reported);

[[nodiscard]] std::vector<core::AudioDeviceInfo>
described(std::vector<core::AudioDeviceInfo> devices);

}
