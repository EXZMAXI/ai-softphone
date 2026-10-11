// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/capture/system_capture.hpp>

#include <softphone/core/logging.hpp>

#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudio/AudioHardwareTapping.h>
#import <CoreAudio/CATapDescription.h>
#import <CoreAudio/CoreAudio.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include <cstddef>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace softphone::capture {
namespace {

template <typename Value>
[[nodiscard]] bool readProperty(AudioObjectID object, AudioObjectPropertySelector selector,
                                AudioObjectPropertyScope scope, Value &into)
{
    const AudioObjectPropertyAddress where{selector, scope, kAudioObjectPropertyElementMain};
    UInt32 size = sizeof(Value);
    return AudioObjectGetPropertyData(object, &where, 0, nullptr, &size, &into) == noErr;
}

[[nodiscard]] std::string readText(AudioObjectID object, AudioObjectPropertySelector selector)
{
    CFStringRef text = nullptr;
    if (!readProperty(object, selector, kAudioObjectPropertyScopeGlobal, text) || text == nullptr) {
        return {};
    }

    std::string answer;
    if (const auto *bytes = [(__bridge NSString *)text UTF8String]; bytes != nullptr) {
        answer = bytes;
    }
    CFRelease(text);
    return answer;
}

[[nodiscard]] std::string nameOf(AudioObjectID device)
{
    return readText(device, kAudioObjectPropertyName);
}

[[nodiscard]] std::string uidOf(AudioObjectID device)
{
    return readText(device, kAudioDevicePropertyDeviceUID);
}

[[nodiscard]] unsigned channelsOn(AudioObjectID device, AudioObjectPropertyScope scope)
{
    const AudioObjectPropertyAddress where{kAudioDevicePropertyStreamConfiguration, scope,
                                           kAudioObjectPropertyElementMain};
    UInt32 size = 0;
    if (AudioObjectGetPropertyDataSize(device, &where, 0, nullptr, &size) != noErr || size == 0) {
        return 0;
    }

    std::vector<std::byte> room(size);
    auto *const list = reinterpret_cast<AudioBufferList *>(room.data());
    if (AudioObjectGetPropertyData(device, &where, 0, nullptr, &size, list) != noErr) {
        return 0;
    }

    unsigned channels = 0;
    for (UInt32 buffer = 0; buffer < list->mNumberBuffers; ++buffer) {
        channels += list->mBuffers[buffer].mNumberChannels;
    }
    return channels;
}

[[nodiscard]] AudioObjectID defaultDevice(AudioObjectPropertySelector which)
{
    AudioObjectID device = kAudioObjectUnknown;
    if (!readProperty(kAudioObjectSystemObject, which, kAudioObjectPropertyScopeGlobal, device)) {
        return kAudioObjectUnknown;
    }
    return device;
}

[[nodiscard]] std::vector<AudioObjectID> allDevices()
{
    const AudioObjectPropertyAddress where{kAudioHardwarePropertyDevices,
                                           kAudioObjectPropertyScopeGlobal,
                                           kAudioObjectPropertyElementMain};
    UInt32 size = 0;
    if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &where, 0, nullptr, &size) !=
        noErr) {
        return {};
    }

    std::vector<AudioObjectID> devices(size / sizeof(AudioObjectID));
    if (devices.empty()) {
        return {};
    }
    if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &where, 0, nullptr, &size,
                                   devices.data()) != noErr) {
        return {};
    }
    return devices;
}

[[nodiscard]] AudioObjectID deviceNamed(const std::string &wanted, AudioObjectPropertyScope scope)
{
    if (wanted.empty()) {
        return defaultDevice(scope == kAudioObjectPropertyScopeInput
                                 ? kAudioHardwarePropertyDefaultInputDevice
                                 : kAudioHardwarePropertyDefaultOutputDevice);
    }

    for (const auto device : allDevices()) {
        if (channelsOn(device, scope) > 0 && nameOf(device) == wanted) {
            return device;
        }
    }
    return kAudioObjectUnknown;
}

[[nodiscard]] double rateOf(AudioObjectID device)
{
    AudioStreamBasicDescription had{};
    if (device == kAudioObjectUnknown ||
        !readProperty(device, kAudioDevicePropertyStreamFormat, kAudioObjectPropertyScopeInput,
                      had)) {
        return 0.0;
    }
    return had.mSampleRate;
}

struct Wire {

    explicit Wire(Side which) noexcept : side{which} {}

    Side side;

    AudioObjectID device{kAudioObjectUnknown};
    AudioDeviceIOProcID proc{nullptr};
    bool started{false};

    AudioObjectID tap{kAudioObjectUnknown};
    AudioObjectID aggregate{kAudioObjectUnknown};

    AudioObjectID tapped{kAudioObjectUnknown};

    AudioConverterRef converter{nullptr};
    double deviceRate{0.0};

    bool listening{false};

    std::vector<float> mono;

    std::vector<std::int16_t> converted;

    const float *feeding{nullptr};
    std::size_t left{0};

    void *owner{nullptr};
};

constexpr std::size_t kMostFramesInOneCall = 16384;

class CoreAudioCapture final : public ISystemCapture {
public:
    ~CoreAudioCapture() override
    {
        CoreAudioCapture::stop();
    }

    CaptureError start(const CaptureRequest &request, FrameSink sink) override;
    void stop() override;
    [[nodiscard]] CaptureStatus status() const override;

    [[nodiscard]] Yields mustYieldTheSoundCard() const override
    {
        return Yields::WhatTheMachinePlays;
    }

    void holdOffWhatTheMachinePlays(bool held) override;

private:
    static OSStatus onAudio(AudioObjectID device, const AudioTimeStamp *now,
                            const AudioBufferList *input, const AudioTimeStamp *when,
                            AudioBufferList *output, const AudioTimeStamp *by,
                            void *wire) noexcept;

    static OSStatus onWanted(AudioConverterRef converter, UInt32 *packets, AudioBufferList *into,
                             AudioStreamPacketDescription **descriptions, void *wire) noexcept;

    [[nodiscard]] bool stopTheDevice(const std::shared_ptr<Wire> &wire) noexcept;

    [[nodiscard]] CaptureError openTap(Wire &wire, const std::string &output);
    [[nodiscard]] CaptureError openWire(Wire &wire, unsigned rate);
    void closeWire(std::shared_ptr<Wire> &wire) noexcept;

    void deliver(Side side, const std::int16_t *samples, std::size_t count) noexcept;

    [[nodiscard]] bool theDevicesHaveMoved() const;

    [[nodiscard]] bool theRateHasMoved() const;

    void watchTheRate(Wire &wire);
    void stopWatchingTheRate(Wire &wire) noexcept;

    static OSStatus onDevicesMoved(AudioObjectID object, UInt32 count,
                                   const AudioObjectPropertyAddress *what, void *owner) noexcept;

    void watchTheDevices();
    void stopWatching() noexcept;
    void followTheDevices();
    [[nodiscard]] CaptureError openTheWires();

    std::shared_ptr<Wire> near_{std::make_shared<Wire>(Side::Near)};
    std::shared_ptr<Wire> far_{std::make_shared<Wire>(Side::Far)};

    CaptureRequest request_;

    std::atomic<bool> farHeldOff_{false};

    std::mutex wires_;

    std::thread following_;
    std::mutex waking_;
    std::condition_variable wake_;
    std::atomic<bool> moved_{false};
    std::atomic<bool> watching_{false};

    FrameSink sink_;

    mutable std::mutex mutex_;
    CaptureStatus status_;
};

OSStatus CoreAudioCapture::onWanted(AudioConverterRef , UInt32 *packets,
                                    AudioBufferList *into,
                                    AudioStreamPacketDescription **descriptions,
                                    void *wire) noexcept
{
    auto *const it = static_cast<Wire *>(wire);

    if (descriptions != nullptr) {
        *descriptions = nullptr;
    }

    const auto take = std::min<std::size_t>(*packets, it->left);
    into->mNumberBuffers = 1;
    into->mBuffers[0].mNumberChannels = 1;
    into->mBuffers[0].mData = take > 0 ? const_cast<float *>(it->feeding) : nullptr;
    into->mBuffers[0].mDataByteSize = static_cast<UInt32>(take * sizeof(float));

    it->feeding += take;
    it->left -= take;
    *packets = static_cast<UInt32>(take);
    return noErr;
}

OSStatus CoreAudioCapture::onAudio(AudioObjectID , const AudioTimeStamp * ,
                                   const AudioBufferList *input, const AudioTimeStamp * ,
                                   AudioBufferList * , const AudioTimeStamp * ,
                                   void *wire) noexcept
{
    auto *const it = static_cast<Wire *>(wire);
    auto *const owner = static_cast<CoreAudioCapture *>(it->owner);
    if (input == nullptr || input->mNumberBuffers == 0 || owner == nullptr) {
        return noErr;
    }

    const auto &buffer = input->mBuffers[0];
    const auto channels = std::max<UInt32>(1, buffer.mNumberChannels);
    const auto samples = buffer.mDataByteSize / sizeof(float);
    const auto frames = samples / channels;
    if (buffer.mData == nullptr || frames == 0) {
        return noErr;
    }

    if (frames > kMostFramesInOneCall) {
        const std::lock_guard guard{owner->mutex_};
        ++owner->status_.dropped;
        return noErr;
    }

    const auto *const raw = static_cast<const float *>(buffer.mData);
    it->mono.resize(frames);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        float sum = 0.0F;
        for (UInt32 channel = 0; channel < channels; ++channel) {
            sum += raw[(frame * channels) + channel];
        }
        it->mono[frame] = sum / static_cast<float>(channels);
    }

    it->feeding = it->mono.data();
    it->left = frames;

    while (it->left > 0) {
        AudioBufferList out{};
        out.mNumberBuffers = 1;
        out.mBuffers[0].mNumberChannels = 1;
        out.mBuffers[0].mData = it->converted.data();
        out.mBuffers[0].mDataByteSize =
            static_cast<UInt32>(it->converted.size() * sizeof(std::int16_t));

        auto wanted = static_cast<UInt32>(it->converted.size());
        const auto converted =
            AudioConverterFillComplexBuffer(it->converter, onWanted, it, &wanted, &out, nullptr);
        if (converted != noErr || wanted == 0) {
            break;
        }

        owner->deliver(it->side, it->converted.data(), wanted);
    }

    return noErr;
}

void CoreAudioCapture::deliver(Side side, const std::int16_t *samples, std::size_t count) noexcept
{
    if (!sink_) {
        const std::lock_guard guard{mutex_};
        ++status_.dropped;
        return;
    }

    const auto at = std::chrono::steady_clock::now();
    const auto limit = static_cast<std::size_t>(request_.frameSamples);

    std::size_t done = 0;
    while (done < count) {
        const auto take = std::min(limit, count - done);
        sink_(side, std::span<const std::int16_t>{samples + done, take}, at);
        done += take;
    }

    const std::lock_guard guard{mutex_};
    (side == Side::Near ? status_.nearFrames : status_.farFrames) += 1;
}

constexpr AudioObjectPropertyAddress kWhatMoves[] = {
    {kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain},
    {kAudioHardwarePropertyDefaultInputDevice, kAudioObjectPropertyScopeGlobal,
     kAudioObjectPropertyElementMain},
    {kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal,
     kAudioObjectPropertyElementMain},
};

constexpr AudioObjectPropertyAddress kRateMoves{kAudioDevicePropertyNominalSampleRate,
                                                kAudioObjectPropertyScopeGlobal,
                                                kAudioObjectPropertyElementMain};

constexpr auto kSettling = std::chrono::milliseconds{400};

OSStatus CoreAudioCapture::onDevicesMoved(AudioObjectID , UInt32 ,
                                          const AudioObjectPropertyAddress * ,
                                          void *owner) noexcept
{

    auto *const self = static_cast<CoreAudioCapture *>(owner);
    self->moved_.store(true, std::memory_order_release);
    self->wake_.notify_one();
    return noErr;
}

void CoreAudioCapture::watchTheDevices()
{
    if (watching_.exchange(true)) {
        return;
    }

    for (const auto &what : kWhatMoves) {
        static_cast<void>(
            AudioObjectAddPropertyListener(kAudioObjectSystemObject, &what, onDevicesMoved, this));
    }

    following_ = std::thread{[this]() { followTheDevices(); }};
}

void CoreAudioCapture::stopWatching() noexcept
{
    if (!watching_.exchange(false)) {
        return;
    }

    for (const auto &what : kWhatMoves) {
        static_cast<void>(AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &what,
                                                            onDevicesMoved, this));
    }

    wake_.notify_all();
    if (following_.joinable()) {
        following_.join();
    }
}

void CoreAudioCapture::watchTheRate(Wire &wire)
{
    if (wire.listening || wire.device == kAudioObjectUnknown) {
        return;
    }
    wire.listening = AudioObjectAddPropertyListener(wire.device, &kRateMoves, onDevicesMoved,
                                                    this) == noErr;
}

void CoreAudioCapture::stopWatchingTheRate(Wire &wire) noexcept
{
    if (!wire.listening) {
        return;
    }
    static_cast<void>(
        AudioObjectRemovePropertyListener(wire.device, &kRateMoves, onDevicesMoved, this));
    wire.listening = false;
}

bool CoreAudioCapture::theRateHasMoved() const
{

    const auto microphone = rateOf(near_->device);
    const auto machine = rateOf(far_->device);
    return (microphone > 0.0 && microphone != near_->deviceRate) ||
           (machine > 0.0 && machine != far_->deviceRate);
}

bool CoreAudioCapture::theDevicesHaveMoved() const
{

    if (deviceNamed(request_.inputDevice, kAudioObjectPropertyScopeInput) != near_->device) {
        return true;
    }

    if (farHeldOff_.load(std::memory_order_acquire)) {
        return false;
    }

    return deviceNamed(request_.outputDevice, kAudioObjectPropertyScopeOutput) != far_->tapped;
}

void CoreAudioCapture::followTheDevices()
{
    while (watching_.load(std::memory_order_acquire)) {
        {
            std::unique_lock sleeping{waking_};
            wake_.wait(sleeping, [this] {
                return moved_.load(std::memory_order_acquire) ||
                       !watching_.load(std::memory_order_acquire);
            });
        }
        if (!watching_.load(std::memory_order_acquire)) {
            return;
        }

        std::this_thread::sleep_for(kSettling);
        moved_.store(false, std::memory_order_release);

        const std::lock_guard guard{wires_};
        if (!watching_.load(std::memory_order_acquire)) {
            continue;
        }

        const bool devicesMoved = theDevicesHaveMoved();
        const bool rateMoved = !devicesMoved && theRateHasMoved();
        if (!devicesMoved && !rateMoved) {
            continue;
        }

        const auto was = [this]() {
            const std::lock_guard held{mutex_};
            return status_.inputDevice + "/" + status_.outputDevice;
        }();
        const auto rateWas = std::to_string(static_cast<long>(near_->deviceRate)) + "/" +
                             std::to_string(static_cast<long>(far_->deviceRate));

        closeWire(near_);
        closeWire(far_);

        if (const auto opened = openTheWires(); opened != CaptureError::None) {
            core::Logger::instance().log(core::LogLevel::Warning, "capture",
                                         "the devices moved under this recording and what it "
                                         "moved to would not open: " +
                                             std::string{toString(opened)} + ", was " + was);
            continue;
        }

        const std::lock_guard held{mutex_};
        if (rateMoved) {
            core::Logger::instance().log(
                core::LogLevel::Info, "capture",
                "the devices under this recording changed rate: " + was + " was at " + rateWas +
                    " Hz and is now at " +
                    std::to_string(static_cast<long>(near_->deviceRate)) + "/" +
                    std::to_string(static_cast<long>(far_->deviceRate)) + " Hz");
            continue;
        }
        core::Logger::instance().log(core::LogLevel::Info, "capture",
                                     "the devices moved under this recording: it was on " + was +
                                         " and is now on " + status_.inputDevice + "/" +
                                         status_.outputDevice);
    }
}

CaptureError CoreAudioCapture::openTap(Wire &wire, const std::string &output)
{
    const auto device = deviceNamed(output, kAudioObjectPropertyScopeOutput);
    if (device == kAudioObjectUnknown) {
        return CaptureError::NoDevice;
    }

    const auto uid = uidOf(device);
    if (uid.empty()) {
        return CaptureError::NoDevice;
    }
    NSString *const named = @(uid.c_str());

    CATapDescription *const description = [[CATapDescription alloc] initExcludingProcesses:@[]
                                                                              andDeviceUID:named
                                                                                withStream:0];
    description.name = @"AI Softphone";
    description.UUID = [NSUUID UUID];
    description.muteBehavior = CATapUnmuted;

    description.privateTap = YES;

    if (AudioHardwareCreateProcessTap(description, &wire.tap) != noErr ||
        wire.tap == kAudioObjectUnknown) {

        return CaptureError::Refused;
    }

    NSDictionary *const wanted = @{
        @(kAudioAggregateDeviceNameKey) : @"AI Softphone capture",
        @(kAudioAggregateDeviceUIDKey) : [[NSUUID UUID] UUIDString],
        @(kAudioAggregateDeviceIsPrivateKey) : @YES,
        @(kAudioAggregateDeviceIsStackedKey) : @NO,
        @(kAudioAggregateDeviceTapAutoStartKey) : @YES,
        @(kAudioAggregateDeviceMainSubDeviceKey) : named,
        @(kAudioAggregateDeviceSubDeviceListKey) : @[ @{@(kAudioSubDeviceUIDKey) : named} ],
        @(kAudioAggregateDeviceTapListKey) : @[ @{
            @(kAudioSubTapUIDKey) : [description.UUID UUIDString],
            @(kAudioSubTapDriftCompensationKey) : @YES
        } ],
    };

    if (AudioHardwareCreateAggregateDevice((__bridge CFDictionaryRef)wanted, &wire.aggregate) !=
            noErr ||
        wire.aggregate == kAudioObjectUnknown) {
        return CaptureError::Refused;
    }

    wire.device = wire.aggregate;
    wire.tapped = device;

    const std::lock_guard guard{mutex_};
    status_.outputDevice = nameOf(device);
    return CaptureError::None;
}

CaptureError CoreAudioCapture::openWire(Wire &wire, unsigned rate)
{
    wire.owner = this;

    const auto deviceRate = rateOf(wire.device);
    if (deviceRate <= 0.0) {
        return CaptureError::NoDevice;
    }
    wire.deviceRate = deviceRate;

    watchTheRate(wire);

    AudioStreamBasicDescription from{};
    from.mSampleRate = deviceRate;
    from.mFormatID = kAudioFormatLinearPCM;
    from.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    from.mFramesPerPacket = 1;
    from.mChannelsPerFrame = 1;
    from.mBitsPerChannel = 32;
    from.mBytesPerFrame = sizeof(float);
    from.mBytesPerPacket = sizeof(float);

    AudioStreamBasicDescription to{};
    to.mSampleRate = rate;
    to.mFormatID = kAudioFormatLinearPCM;
    to.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    to.mFramesPerPacket = 1;
    to.mChannelsPerFrame = 1;
    to.mBitsPerChannel = 16;
    to.mBytesPerFrame = sizeof(std::int16_t);
    to.mBytesPerPacket = sizeof(std::int16_t);

    if (AudioConverterNew(&from, &to, &wire.converter) != noErr || wire.converter == nullptr) {
        return CaptureError::Refused;
    }

    const auto ratio = static_cast<double>(rate) / deviceRate;
    wire.mono.reserve(kMostFramesInOneCall);
    wire.converted.resize(
        static_cast<std::size_t>((kMostFramesInOneCall * std::max(1.0, ratio)) + 64));

    if (AudioDeviceCreateIOProcID(wire.device, onAudio, &wire, &wire.proc) != noErr ||
        wire.proc == nullptr) {
        return CaptureError::Refused;
    }

    if (AudioDeviceStart(wire.device, wire.proc) != noErr) {
        return CaptureError::Refused;
    }
    wire.started = true;

    return CaptureError::None;
}

constexpr auto kStoppingBound = std::chrono::seconds{3};

bool CoreAudioCapture::stopTheDevice(const std::shared_ptr<Wire> &wire) noexcept
{
    std::promise<void> finished;
    auto waiting = finished.get_future();

    std::thread stopping{[wire, done = std::move(finished)]() mutable {
        AudioDeviceStop(wire->device, wire->proc);
        wire->started = false;
        done.set_value();
    }};

    if (waiting.wait_for(kStoppingBound) == std::future_status::ready) {
        stopping.join();
        return true;
    }

    stopping.detach();
    core::Logger::instance().log(core::LogLevel::Warning, "capture",
                                 "a sound device would not stop inside three seconds and has been "
                                 "left behind; this phone is not waiting for it");
    return false;
}

void CoreAudioCapture::closeWire(std::shared_ptr<Wire> &held) noexcept
{
    if (!held) {
        return;
    }
    Wire &wire = *held;

    stopWatchingTheRate(wire);

    wire.owner = nullptr;

    if (wire.proc != nullptr) {
        if (wire.started && !stopTheDevice(held)) {

            held = std::make_shared<Wire>(wire.side);
            return;
        }
        AudioDeviceDestroyIOProcID(wire.device, wire.proc);
        wire.proc = nullptr;
    }

    if (wire.converter != nullptr) {
        AudioConverterDispose(wire.converter);
        wire.converter = nullptr;
    }

    if (wire.aggregate != kAudioObjectUnknown) {
        AudioHardwareDestroyAggregateDevice(wire.aggregate);
        wire.aggregate = kAudioObjectUnknown;
    }
    if (wire.tap != kAudioObjectUnknown) {
        AudioHardwareDestroyProcessTap(wire.tap);
        wire.tap = kAudioObjectUnknown;
    }

    wire.device = kAudioObjectUnknown;
    wire.tapped = kAudioObjectUnknown;
    wire.deviceRate = 0.0;
    wire.feeding = nullptr;
    wire.left = 0;
}

CaptureError CoreAudioCapture::openTheWires()
{

    const bool wantsTheTap = !farHeldOff_.load(std::memory_order_acquire);

    if (wantsTheTap) {
        if (const auto opened = openTap(*far_, request_.outputDevice);
            opened != CaptureError::None) {
            closeWire(far_);
            return opened;
        }
    } else {

        {
            const std::lock_guard guard{mutex_};
            status_.outputDevice.clear();
        }
        core::Logger::instance().log(core::LogLevel::Info, "capture",
                                     "what this machine plays is not being read: the telephone "
                                     "has the sound card");
    }

    near_->device = deviceNamed(request_.inputDevice, kAudioObjectPropertyScopeInput);
    if (near_->device == kAudioObjectUnknown) {
        closeWire(far_);
        return CaptureError::NoDevice;
    }
    {
        const std::lock_guard guard{mutex_};
        status_.inputDevice = nameOf(near_->device);
    }

    if (wantsTheTap) {
        if (const auto opened = openWire(*far_, request_.sampleRate);
            opened != CaptureError::None) {
            closeWire(near_);
            closeWire(far_);
            return opened;
        }
    }
    if (const auto opened = openWire(*near_, request_.sampleRate); opened != CaptureError::None) {
        closeWire(near_);
        closeWire(far_);
        return opened;
    }

    const std::lock_guard guard{mutex_};
    status_.running = true;
    return CaptureError::None;
}

void CoreAudioCapture::holdOffWhatTheMachinePlays(bool held)
{
    if (farHeldOff_.exchange(held) == held) {
        return;
    }

    const std::lock_guard guard{wires_};

    if (held) {

        if (far_->device == kAudioObjectUnknown) {
            return;
        }

        closeWire(far_);
        {
            const std::lock_guard status{mutex_};
            status_.outputDevice.clear();
        }
        core::Logger::instance().log(
            core::LogLevel::Info, "capture",
            "the telephone has the sound card, so what this machine plays stands down; the "
            "microphone goes on being read");
        return;
    }

    if (near_->device == kAudioObjectUnknown) {
        return;
    }

    if (const auto opened = openTap(*far_, request_.outputDevice); opened != CaptureError::None) {
        closeWire(far_);
        core::Logger::instance().log(core::LogLevel::Warning, "capture",
                                     "the telephone finished with the sound card and what this "
                                     "machine plays would not open again: " +
                                         std::string{toString(opened)});
        return;
    }
    if (const auto opened = openWire(*far_, request_.sampleRate); opened != CaptureError::None) {
        closeWire(far_);
        core::Logger::instance().log(core::LogLevel::Warning, "capture",
                                     "the telephone finished with the sound card and what this "
                                     "machine plays would not open again: " +
                                         std::string{toString(opened)});
        return;
    }

    core::Logger::instance().log(core::LogLevel::Info, "capture",
                                 "the telephone finished with the sound card, so what this "
                                 "machine plays is being read again");
}

CaptureError CoreAudioCapture::start(const CaptureRequest &request, FrameSink sink)
{
    stop();

    const std::lock_guard guard{wires_};

    request_ = request;
    {
        const std::lock_guard held{mutex_};
        status_ = CaptureStatus{};
    }

    sink_ = std::move(sink);

    if (const auto opened = openTheWires(); opened != CaptureError::None) {
        sink_ = {};
        return opened;
    }

    watchTheDevices();
    return CaptureError::None;
}

void CoreAudioCapture::stop()
{

    stopWatching();

    const std::lock_guard guard{wires_};

    closeWire(near_);
    closeWire(far_);

    sink_ = {};

    const std::lock_guard held{mutex_};
    status_.running = false;
}

CaptureStatus CoreAudioCapture::status() const
{
    const std::lock_guard guard{mutex_};
    return status_;
}

}

std::unique_ptr<ISystemCapture> makeSystemCapture()
{
    return std::make_unique<CoreAudioCapture>();
}

}
