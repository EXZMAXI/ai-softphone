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

#include <softphone/core/level_meter.hpp>
#include <softphone/core/settings.hpp>

#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

enum class DeviceRole { Capture, Playback };

struct AudioDeviceInfo {
    std::string driver;
    std::string name;

    unsigned inputs{0};
    unsigned outputs{0};

    std::string microphoneDescription;
    std::string speakerDescription;

    [[nodiscard]] const std::string &describedAs(DeviceRole role) const noexcept
    {
        return role == DeviceRole::Capture ? microphoneDescription : speakerDescription;
    }

    [[nodiscard]] bool canCapture() const noexcept
    {
        return inputs > 0;
    }

    [[nodiscard]] bool canPlay() const noexcept
    {
        return outputs > 0;
    }

    friend bool operator==(const AudioDeviceInfo &, const AudioDeviceInfo &) = default;
};

enum class DeviceUse {

    SystemDefault,

    Chosen,

    Missing,
};

struct DeviceSelection {
    DeviceUse use{DeviceUse::SystemDefault};

    AudioDevice wanted;

    std::optional<std::size_t> position;

    friend bool operator==(const DeviceSelection &, const DeviceSelection &) = default;
};

struct AudioApplyResult {
    DeviceSelection microphone;
    DeviceSelection speakers;

    DeviceSelection ringer;

    bool ringerOnItsOwnDevice{false};

    friend bool operator==(const AudioApplyResult &, const AudioApplyResult &) = default;
};

[[nodiscard]] DeviceSelection chooseDevice(const AudioDevice &wanted,
                                           std::span<const AudioDeviceInfo> available,
                                           DeviceRole role);

[[nodiscard]] AudioApplyResult planRinger(const AudioDevice &wanted,
                                          std::span<const AudioDeviceInfo> available,
                                          AudioApplyResult result);

[[nodiscard]] std::vector<AudioDeviceInfo>
devicesWorthOffering(std::span<const AudioDeviceInfo> available);

[[nodiscard]] std::string_view toString(DeviceUse use) noexcept;
[[nodiscard]] std::string_view toString(DeviceRole role) noexcept;

class IAudioDeviceBackend {
public:
    IAudioDeviceBackend() = default;
    IAudioDeviceBackend(const IAudioDeviceBackend &) = delete;
    IAudioDeviceBackend &operator=(const IAudioDeviceBackend &) = delete;
    IAudioDeviceBackend(IAudioDeviceBackend &&) = delete;
    IAudioDeviceBackend &operator=(IAudioDeviceBackend &&) = delete;
    virtual ~IAudioDeviceBackend() = default;

    [[nodiscard]] virtual std::vector<AudioDeviceInfo> devices() = 0;

    virtual AudioApplyResult applyAudio(const AudioSettings &audio) = 0;

    virtual void applyVolumes(const Volumes &volumes) = 0;

    using DeviceLostCallback = std::function<void(DeviceRole)>;
    virtual void setDeviceLostCallback(DeviceLostCallback callback) = 0;

    using DevicesChangedCallback = std::function<void()>;
    virtual void setDevicesChangedCallback(DevicesChangedCallback callback) = 0;

    [[nodiscard]] virtual std::string systemDefaultDevice(DeviceRole role) const = 0;

    virtual void startMetering() = 0;
    virtual void stopMetering() = 0;

    [[nodiscard]] virtual std::optional<AudioLevels> audioLevels() const = 0;
};

}
