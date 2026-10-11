// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/audio_devices.hpp>

#include <algorithm>
#include <array>

namespace softphone::core {

namespace {

constexpr std::array kPlumbingPrefixes{
    "hw:",
    "plughw:",
    "dmix:",
    "dsnoop:",
    "front:",
    "rear:",
    "side:",
    "center_lfe:",
    "surround",
    "iec958:",
    "usbstream:",
    "null",

    "hdmi:",
};

constexpr std::array kSoundServerNames{
    "default", "pipewire", "pulse", "jack", "oss", "sysdefault",
};

[[nodiscard]] bool isPlumbing(const std::string &name) noexcept
{
    return std::any_of(kPlumbingPrefixes.begin(), kPlumbingPrefixes.end(),
                       [&name](const char *prefix) { return name.starts_with(prefix); });
}

[[nodiscard]] bool isTheSoundServer(const std::string &name) noexcept
{
    return std::any_of(kSoundServerNames.begin(), kSoundServerNames.end(),
                       [&name](const char *server) { return name == server; });
}

[[nodiscard]] bool suits(const AudioDeviceInfo &device, DeviceRole role) noexcept
{
    return role == DeviceRole::Capture ? device.canCapture() : device.canPlay();
}

}

DeviceSelection chooseDevice(const AudioDevice &wanted, std::span<const AudioDeviceInfo> available,
                             DeviceRole role)
{
    if (wanted.systemDefault()) {
        return DeviceSelection{DeviceUse::SystemDefault, wanted, std::nullopt};
    }

    for (std::size_t position = 0; position < available.size(); ++position) {
        const auto &device = available[position];
        if (!suits(device, role) || device.name != wanted.name) {
            continue;
        }
        if (!wanted.driver.empty() && device.driver != wanted.driver) {
            continue;
        }
        return DeviceSelection{DeviceUse::Chosen, wanted, position};
    }

    return DeviceSelection{DeviceUse::Missing, wanted, std::nullopt};
}

AudioApplyResult planRinger(const AudioDevice &wanted, std::span<const AudioDeviceInfo> available,
                            AudioApplyResult result)
{
    result.ringer = chooseDevice(wanted, available, DeviceRole::Playback);
    result.ringerOnItsOwnDevice = false;

    if (result.ringer.use != DeviceUse::Chosen) {

        return result;
    }

    if (result.speakers.position == result.ringer.position) {
        return result;
    }

    result.ringerOnItsOwnDevice = true;
    return result;
}

std::vector<AudioDeviceInfo> devicesWorthOffering(std::span<const AudioDeviceInfo> available)
{
    std::vector<AudioDeviceInfo> offered;
    offered.reserve(available.size());

    for (const auto &device : available) {
        if (!isPlumbing(device.name) && !isTheSoundServer(device.name)) {
            offered.push_back(device);
        }
    }

    if (offered.empty()) {
        return {available.begin(), available.end()};
    }

    return offered;
}

std::string_view toString(DeviceUse use) noexcept
{
    switch (use) {
    case DeviceUse::SystemDefault:
        return "system-default";
    case DeviceUse::Chosen:
        return "chosen";
    case DeviceUse::Missing:
        return "missing";
    }
    return "unknown";
}

std::string_view toString(DeviceRole role) noexcept
{
    switch (role) {
    case DeviceRole::Capture:
        return "capture";
    case DeviceRole::Playback:
        return "playback";
    }
    return "unknown";
}

}
