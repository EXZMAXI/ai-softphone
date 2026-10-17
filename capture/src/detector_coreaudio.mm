// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/capture/detector.hpp>

#include <softphone/capture/signal_trail.hpp>

#include <softphone/core/logging.hpp>

#import <CoreAudio/CoreAudio.h>
#import <Foundation/Foundation.h>

#include <dlfcn.h>
#include <libproc.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace softphone::capture {
namespace {

constexpr auto kTick = std::chrono::milliseconds{250};

template <typename Value>
[[nodiscard]] bool readProperty(AudioObjectID object, AudioObjectPropertySelector selector,
                                Value &into)
{
    const AudioObjectPropertyAddress where{selector, kAudioObjectPropertyScopeGlobal,
                                           kAudioObjectPropertyElementMain};
    UInt32 size = sizeof(Value);
    return AudioObjectGetPropertyData(object, &where, 0, nullptr, &size, &into) == noErr;
}

[[nodiscard]] std::vector<AudioObjectID> audioProcesses()
{
    const AudioObjectPropertyAddress where{kAudioHardwarePropertyProcessObjectList,
                                           kAudioObjectPropertyScopeGlobal,
                                           kAudioObjectPropertyElementMain};
    UInt32 size = 0;
    if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &where, 0, nullptr, &size) !=
            noErr ||
        size == 0) {
        return {};
    }

    std::vector<AudioObjectID> processes(size / sizeof(AudioObjectID));
    if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &where, 0, nullptr, &size,
                                   processes.data()) != noErr) {
        return {};
    }
    return processes;
}

[[nodiscard]] std::string binaryOf(pid_t process)
{
    std::array<char, 2 * MAXCOMLEN + 1> room{};
    if (::proc_name(process, room.data(), static_cast<std::uint32_t>(room.size())) <= 0) {
        return {};
    }
    return std::string{room.data()};
}

[[nodiscard]] std::string titleOf(pid_t process)
{
    std::array<char, PROC_PIDPATHINFO_MAXSIZE> room{};
    if (::proc_pidpath(process, room.data(), static_cast<std::uint32_t>(room.size())) <= 0) {
        return {};
    }

    const std::string executable{room.data()};
    const auto ends = executable.find(".app/Contents/");
    if (ends == std::string::npos) {
        return {};
    }

    NSBundle *const bundle =
        [NSBundle bundleWithPath:@(executable.substr(0, ends + 4).c_str())];
    if (bundle == nil) {
        return {};
    }

    for (NSString *const key in @[ @"CFBundleDisplayName", @"CFBundleName" ]) {
        id named = [bundle objectForInfoDictionaryKey:key];
        if ([named isKindOfClass:[NSString class]]) {
            const auto *const bytes = [static_cast<NSString *>(named) UTF8String];
            if (bytes != nullptr) {
                return std::string{bytes};
            }
        }
    }
    return {};
}

[[nodiscard]] pid_t responsibleFor(pid_t process)
{
    using Asks = pid_t (*)(pid_t);
    static const auto ask =
        reinterpret_cast<Asks>(::dlsym(RTLD_DEFAULT, "responsibility_get_pid_responsible_for_pid"));
    if (ask == nullptr) {
        return process;
    }
    const auto answer = ask(process);
    return answer > 0 ? answer : process;
}

[[nodiscard]] Conversation whoIsRunning(pid_t process)
{
    if (auto named = titleOf(process); !named.empty()) {
        return Conversation{binaryOf(process), std::move(named)};
    }

    const auto owner = responsibleFor(process);
    if (owner != process) {
        if (auto named = titleOf(owner); !named.empty()) {
            return Conversation{binaryOf(owner), std::move(named)};
        }
    }
    return Conversation{binaryOf(process), {}};
}

class CoreAudioDetector final : public IConversationDetector {
public:
    ~CoreAudioDetector() override
    {
        CoreAudioDetector::stop();
    }

    CaptureError start(const DetectorSettings &settings, Began began, Ended ended) override;
    void stop() override;

    [[nodiscard]] bool talking() const override
    {
        return talking_.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool available() const override
    {
        return true;
    }

    [[nodiscard]] std::string trail() const override
    {
        const std::lock_guard guard{trailing_};
        return trail_.render(settings_.settle, settings_.holdOff);
    }

private:

    struct Seen {
        bool held{false};
        bool playing{false};

        bool duplex{false};

        Conversation who;
    };

    [[nodiscard]] Seen look() const;
    void tick();

    DetectorSettings settings_;
    Began began_;
    Ended ended_;

    std::thread ticker_;
    std::atomic<bool> running_{false};
    std::mutex sleeping_;
    std::condition_variable wake_;
    std::atomic<bool> talking_{false};

    bool everSaid_{false};
    bool saidHeld_{false};
    bool saidPlaying_{false};

    mutable std::mutex trailing_;
    SignalTrail trail_;
    std::chrono::steady_clock::time_point watching_{};
};

CoreAudioDetector::Seen CoreAudioDetector::look() const
{
    Seen reading;

    for (const auto object : audioProcesses()) {
        pid_t process = 0;
        if (!readProperty(object, kAudioProcessPropertyPID, process) || process <= 0) {
            continue;
        }
        if (static_cast<std::uint32_t>(process) == settings_.ownProcessId) {
            continue;
        }

        UInt32 input = 0;
        UInt32 output = 0;
        const auto readsInput =
            readProperty(object, kAudioProcessPropertyIsRunningInput, input) && input != 0;
        const auto writesOutput =
            readProperty(object, kAudioProcessPropertyIsRunningOutput, output) && output != 0;
        if (!readsInput && !writesOutput) {
            continue;
        }

        const auto who = whoIsRunning(process);
        if (std::ranges::find(settings_.excluded, who.application) != settings_.excluded.end()) {

            continue;
        }

        reading.playing = reading.playing || writesOutput;

        if (readsInput && writesOutput && !reading.duplex) {
            reading.held = true;
            reading.duplex = true;
            reading.who = who;
        } else if (readsInput && !reading.held) {
            reading.held = true;
            reading.who = who;
        }
    }

    return reading;
}

void CoreAudioDetector::tick()
{
    ConversationGate gate{settings_};

    while (running_.load(std::memory_order_acquire)) {
        {
            std::unique_lock sleeping{sleeping_};
            wake_.wait_for(sleeping, kTick,
                           [this] { return !running_.load(std::memory_order_acquire); });
        }
        if (!running_.load(std::memory_order_acquire)) {
            return;
        }

        const auto reading = look();

        if (!everSaid_ || reading.held != saidHeld_ || reading.playing != saidPlaying_) {
            everSaid_ = true;
            saidHeld_ = reading.held;
            saidPlaying_ = reading.playing;
            core::Logger::instance().log(
                core::LogLevel::Debug, "capture",
                std::string{"the machine: microphone "} + (reading.held ? "held" : "free") +
                    (reading.who.application.empty() ? std::string{}
                                                     : " by " + reading.who.application) +
                    ", output " + (reading.playing ? "playing" : "quiet"));
        }

        const auto at = std::chrono::steady_clock::now();

        const auto move = gate.saw(reading.held, reading.duplex, reading.playing, at);
        talking_.store(gate.talking(), std::memory_order_release);

        {
            const std::lock_guard guard{trailing_};
            trail_.saw(Reading{std::chrono::duration_cast<std::chrono::milliseconds>(
                                   at - watching_),
                               reading.held, reading.playing, reading.duplex, gate.talking()});
        }

        if (move == ConversationGate::Move::Began && began_) {
            began_(reading.who);
        } else if (move == ConversationGate::Move::Ended && ended_) {
            ended_();
        }
    }
}

CaptureError CoreAudioDetector::start(const DetectorSettings &settings, Began began, Ended ended)
{
    stop();

    settings_ = settings;
    began_ = std::move(began);
    ended_ = std::move(ended);
    talking_.store(false, std::memory_order_relaxed);
    everSaid_ = false;
    {
        const std::lock_guard guard{trailing_};
        trail_.forget();
        watching_ = std::chrono::steady_clock::now();
    }

    if (audioProcesses().empty()) {
        return CaptureError::NoServer;
    }

    running_.store(true, std::memory_order_release);
    ticker_ = std::thread{[this] { tick(); }};
    return CaptureError::None;
}

void CoreAudioDetector::stop()
{
    running_.store(false, std::memory_order_release);
    wake_.notify_all();

    if (ticker_.get_id() == std::this_thread::get_id()) {
        return;
    }
    if (ticker_.joinable()) {
        ticker_.join();
    }
}

}

std::unique_ptr<IConversationDetector> makeConversationDetector()
{
    return std::make_unique<CoreAudioDetector>();
}

}
