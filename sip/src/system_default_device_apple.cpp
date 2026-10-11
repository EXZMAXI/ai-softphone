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

#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace softphone::sip {
namespace {

template<typename T>
class Owned {
public:
    explicit Owned(T value) noexcept : value_{value} {}

    Owned(const Owned &) = delete;
    Owned &operator=(const Owned &) = delete;
    Owned(Owned &&) = delete;
    Owned &operator=(Owned &&) = delete;

    ~Owned()
    {
        if (value_ != nullptr) {
            ::CFRelease(value_);
        }
    }

    [[nodiscard]] T get() const noexcept
    {
        return value_;
    }

private:
    T value_{};
};

[[nodiscard]] std::string asUtf8(CFStringRef text)
{
    if (text == nullptr) {
        return {};
    }

    const auto length = ::CFStringGetLength(text);
    const auto room = ::CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    std::vector<char> buffer(static_cast<std::size_t>(room), '\0');
    if (!::CFStringGetCString(text, buffer.data(), room, kCFStringEncodingUTF8)) {
        return {};
    }
    return std::string{buffer.data()};
}

}

std::optional<std::string> systemDefaultDeviceName(core::DeviceRole role)
{
    AudioObjectPropertyAddress which{
        role == core::DeviceRole::Capture ? kAudioHardwarePropertyDefaultInputDevice
                                          : kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};

    AudioDeviceID device = kAudioObjectUnknown;
    UInt32 size = sizeof(device);
    if (::AudioObjectGetPropertyData(kAudioObjectSystemObject, &which, 0, nullptr, &size,
                                     &device) != noErr ||
        device == kAudioObjectUnknown) {
        return std::nullopt;
    }

    const AudioObjectPropertyAddress named{
        kAudioObjectPropertyName, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};

    CFStringRef raw = nullptr;
    size = sizeof(raw);
    if (::AudioObjectGetPropertyData(device, &named, 0, nullptr, &size, &raw) != noErr ||
        raw == nullptr) {
        return std::nullopt;
    }

    const Owned<CFStringRef> name{raw};
    auto text = asUtf8(name.get());
    if (text.empty()) {
        return std::nullopt;
    }
    return text;
}

namespace {

constexpr AudioObjectPropertyAddress kWhatMoves[] = {
    {kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
     kAudioObjectPropertyElementMain},
    {kAudioHardwarePropertyDefaultInputDevice, kAudioObjectPropertyScopeGlobal,
     kAudioObjectPropertyElementMain},
    {kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal,
     kAudioObjectPropertyElementMain},
};

constexpr auto kSettling = std::chrono::milliseconds{400};

class CoreAudioWatch final : public DeviceWatch {
public:
    explicit CoreAudioWatch(std::function<void()> moved) : moved_{std::move(moved)}
    {
        for (const auto &what : kWhatMoves) {
            static_cast<void>(
                ::AudioObjectAddPropertyListener(kAudioObjectSystemObject, &what, heard, this));
        }
        settling_ = std::thread{[this]() { settle(); }};
    }

    CoreAudioWatch(const CoreAudioWatch &) = delete;
    CoreAudioWatch &operator=(const CoreAudioWatch &) = delete;
    CoreAudioWatch(CoreAudioWatch &&) = delete;
    CoreAudioWatch &operator=(CoreAudioWatch &&) = delete;

    ~CoreAudioWatch() override
    {

        for (const auto &what : kWhatMoves) {
            static_cast<void>(
                ::AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &what, heard, this));
        }

        watching_.store(false, std::memory_order_release);
        wake_.notify_all();
        if (settling_.joinable()) {
            settling_.join();
        }
    }

private:
    static OSStatus heard(AudioObjectID , UInt32 ,
                          const AudioObjectPropertyAddress * , void *owner) noexcept
    {

        auto *const self = static_cast<CoreAudioWatch *>(owner);
        self->something_.store(true, std::memory_order_release);
        self->wake_.notify_one();
        return noErr;
    }

    void settle()
    {
        while (watching_.load(std::memory_order_acquire)) {
            {
                std::unique_lock sleeping{waking_};
                wake_.wait(sleeping, [this] {
                    return something_.load(std::memory_order_acquire) ||
                           !watching_.load(std::memory_order_acquire);
                });
            }
            if (!watching_.load(std::memory_order_acquire)) {
                return;
            }

            std::this_thread::sleep_for(kSettling);
            something_.store(false, std::memory_order_release);
            if (watching_.load(std::memory_order_acquire) && moved_) {
                moved_();
            }
        }
    }

    std::function<void()> moved_;
    std::thread settling_;
    std::atomic<bool> watching_{true};
    std::atomic<bool> something_{false};
    std::mutex waking_;
    std::condition_variable wake_;
};

}

std::unique_ptr<DeviceWatch> watchTheMachinesDevices(std::function<void()> moved)
{
    if (!moved) {
        return {};
    }
    return std::make_unique<CoreAudioWatch>(std::move(moved));
}

}
