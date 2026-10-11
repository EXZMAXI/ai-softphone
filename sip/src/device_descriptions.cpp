// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "device_descriptions.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

namespace softphone::sip {
namespace {

[[nodiscard]] std::string cardOf(const std::string &name)
{
    const auto marker = name.find("CARD=");
    if (marker == std::string::npos) {
        return {};
    }

    const auto start = marker + 5;
    const auto end = name.find(',', start);
    return name.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

[[nodiscard]] unsigned deviceOf(const std::string &name)
{
    const auto marker = name.find("DEV=");
    if (marker == std::string::npos) {
        return 0;
    }

    unsigned value = 0;
    for (auto at = marker + 4;
         at < name.size() && std::isdigit(static_cast<unsigned char>(name[at])); ++at) {
        value = value * 10 + static_cast<unsigned>(name[at] - '0');
    }
    return value;
}

[[nodiscard]] std::string wordsFor(std::span<const SoundServerNode> nodes, const std::string &name,
                                   core::DeviceRole role)
{
    const auto card = cardOf(name);
    if (card.empty()) {
        return {};
    }

    std::vector<const SoundServerNode *> candidates;
    for (const auto &node : nodes) {
        if (node.card == card && node.role == role) {
            candidates.push_back(&node);
        }
    }

    const auto wanted = deviceOf(name);
    const auto exact =
        std::find_if(candidates.begin(), candidates.end(),
                     [wanted](const SoundServerNode *node) { return node->device == wanted; });
    if (exact != candidates.end()) {
        return (*exact)->description;
    }

    return candidates.size() == 1 ? candidates.front()->description : std::string{};
}

void dropWordsThatDoNotDistinguish(std::vector<core::AudioDeviceInfo> &devices,
                                   core::DeviceRole role)
{
    std::set<std::string> offered;
    for (const auto &device : core::devicesWorthOffering(devices)) {
        offered.insert(device.name);
    }

    std::map<std::string, int> uses;
    for (const auto &device : devices) {
        const auto &words = device.describedAs(role);
        if (!words.empty() && offered.contains(device.name)) {
            ++uses[words];
        }
    }

    for (auto &device : devices) {
        auto &words = role == core::DeviceRole::Capture ? device.microphoneDescription
                                                        : device.speakerDescription;
        if (!words.empty() && uses[words] > 1) {
            words.clear();
        }
    }
}

constexpr std::size_t kWmmeNameLimit = 31;

[[nodiscard]] std::size_t characters(const std::string &text)
{
    std::size_t count = 0;
    for (const auto byte : text) {

        if ((static_cast<unsigned char>(byte) & 0xC0U) != 0x80U) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] std::string wholeNameFor(std::span<const AudioEndpoint> endpoints,
                                       const std::string &name, core::DeviceRole role)
{

    if (characters(name) < kWmmeNameLimit) {
        return {};
    }

    const auto sameRole = [role](const AudioEndpoint &endpoint) { return endpoint.role == role; };

    if (std::any_of(endpoints.begin(), endpoints.end(), [&](const AudioEndpoint &endpoint) {
            return sameRole(endpoint) && endpoint.name == name;
        })) {
        return {};
    }

    const AudioEndpoint *only = nullptr;
    for (const auto &endpoint : endpoints) {
        if (!sameRole(endpoint) || !endpoint.name.starts_with(name)) {
            continue;
        }

        if (only != nullptr) {
            return {};
        }
        only = &endpoint;
    }

    return only == nullptr ? std::string{} : only->name;
}

}

std::vector<core::AudioDeviceInfo> describedWith(std::vector<core::AudioDeviceInfo> devices,
                                                 std::span<const SoundServerNode> nodes,
                                                 const std::map<std::string, std::string> &alsa)
{
    for (auto &device : devices) {

        if (device.driver != "ALSA") {
            continue;
        }

        if (device.canCapture()) {
            device.microphoneDescription = wordsFor(nodes, device.name, core::DeviceRole::Capture);
        }
        if (device.canPlay()) {
            device.speakerDescription = wordsFor(nodes, device.name, core::DeviceRole::Playback);
        }
    }

    dropWordsThatDoNotDistinguish(devices, core::DeviceRole::Capture);
    dropWordsThatDoNotDistinguish(devices, core::DeviceRole::Playback);

    for (auto &device : devices) {
        if (device.driver != "ALSA") {
            continue;
        }

        const auto found = alsa.find(device.name);
        if (found == alsa.end()) {
            continue;
        }

        if (device.canCapture() && device.microphoneDescription.empty()) {
            device.microphoneDescription = found->second;
        }
        if (device.canPlay() && device.speakerDescription.empty()) {
            device.speakerDescription = found->second;
        }
    }

    return devices;
}

std::vector<core::AudioDeviceInfo>
describedWithEndpoints(std::vector<core::AudioDeviceInfo> devices,
                       std::span<const AudioEndpoint> endpoints)
{
    for (auto &device : devices) {

        if (device.driver != "WMME") {
            continue;
        }

        if (device.canCapture()) {
            device.microphoneDescription =
                wholeNameFor(endpoints, device.name, core::DeviceRole::Capture);
        }
        if (device.canPlay()) {
            device.speakerDescription =
                wholeNameFor(endpoints, device.name, core::DeviceRole::Playback);
        }
    }

    dropWordsThatDoNotDistinguish(devices, core::DeviceRole::Capture);
    dropWordsThatDoNotDistinguish(devices, core::DeviceRole::Playback);

    return devices;
}

}
