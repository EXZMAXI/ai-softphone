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

#include <softphone/capture/applications.hpp>
#include <softphone/capture/devices.hpp>
#include <softphone/capture/signal_trail.hpp>

#include "com_win32.hpp"
#include "process_win32.hpp"

#include <softphone/core/logging.hpp>

#include <audiopolicy.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <psapi.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace softphone::capture {
namespace {

using win32::ComPtr;
using win32::ComScope;
using win32::imageOf;
using win32::narrow;

constexpr auto kTick = std::chrono::milliseconds{250};

constexpr auto kSweep = std::chrono::seconds{2};

[[nodiscard]] std::string idOf(IMMDevice &device)
{
    LPWSTR raw = nullptr;
    if (FAILED(device.GetId(&raw))) {
        return {};
    }
    auto id = narrow(raw);
    ::CoTaskMemFree(raw);
    return id;
}

class WasapiDetector final : public IConversationDetector {
public:
    ~WasapiDetector() override
    {
        WasapiDetector::stop();
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

    struct Session {
        ComPtr<IAudioSessionControl2> control;
        DWORD process{0};
        std::string image;

        bool ignored{false};
    };

    struct Endpoint {

        std::string id;

        ComPtr<IMMDevice> device;
        ComPtr<IAudioSessionManager2> manager;
        ComPtr<IAudioMeterInformation> meter;
        std::vector<Session> sessions;
    };

    void watch();

    void openEndpoints(IMMDeviceEnumerator &enumerator, EDataFlow flow,
                       std::vector<Endpoint> &endpoints);

    void enumerate(Endpoint &endpoint);

    [[nodiscard]] float loudest(const std::vector<Endpoint> &outputs) const;

    std::thread watcher_;
    std::atomic<bool> running_{false};
    std::mutex sleeping_;
    std::condition_variable wake_;

    DetectorSettings settings_;
    Began began_;
    Ended ended_;

    std::atomic<bool> talking_{false};

    mutable std::mutex naming_;
    Conversation seen_;

    mutable std::mutex trailing_;
    SignalTrail trail_;
    std::chrono::steady_clock::time_point watching_{};
};

void WasapiDetector::openEndpoints(IMMDeviceEnumerator &enumerator, EDataFlow flow,
                                   std::vector<Endpoint> &endpoints)
{
    IMMDeviceCollection *rawCollection = nullptr;
    if (FAILED(enumerator.EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &rawCollection))) {
        return;
    }
    const ComPtr<IMMDeviceCollection> collection{rawCollection};

    UINT count = 0;
    if (FAILED(collection->GetCount(&count))) {
        return;
    }

    std::vector<Endpoint> found;
    found.reserve(count);

    for (UINT at = 0; at < count; ++at) {
        IMMDevice *rawDevice = nullptr;
        if (FAILED(collection->Item(at, &rawDevice))) {
            continue;
        }
        ComPtr<IMMDevice> device{rawDevice};
        auto id = idOf(*device);

        const auto older = std::ranges::find_if(
            endpoints, [&id](const Endpoint &was) { return !was.id.empty() && was.id == id; });
        if (older != endpoints.end()) {
            found.push_back(std::move(*older));
            continue;
        }

        Endpoint one;
        one.id = std::move(id);
        one.device = std::move(device);

        IAudioSessionManager2 *rawManager = nullptr;
        if (SUCCEEDED(one.device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr,
                                           reinterpret_cast<void **>(&rawManager)))) {
            one.manager.reset(rawManager);
        }

        IAudioMeterInformation *rawMeter = nullptr;
        if (SUCCEEDED(one.device->Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, nullptr,
                                           reinterpret_cast<void **>(&rawMeter)))) {
            one.meter.reset(rawMeter);
        }

        found.push_back(std::move(one));
    }

    endpoints = std::move(found);
}

void WasapiDetector::enumerate(Endpoint &endpoint)
{
    IAudioSessionEnumerator *rawSessions = nullptr;
    if (endpoint.manager == nullptr ||
        FAILED(endpoint.manager->GetSessionEnumerator(&rawSessions))) {
        return;
    }
    const ComPtr<IAudioSessionEnumerator> sessions{rawSessions};

    int count = 0;
    if (FAILED(sessions->GetCount(&count))) {
        return;
    }

    std::vector<Session> found;
    found.reserve(static_cast<std::size_t>(count));

    for (int at = 0; at < count; ++at) {
        IAudioSessionControl *rawControl = nullptr;
        if (FAILED(sessions->GetSession(at, &rawControl))) {
            continue;
        }
        const ComPtr<IAudioSessionControl> control{rawControl};

        IAudioSessionControl2 *rawTwo = nullptr;
        if (FAILED(control->QueryInterface(__uuidof(IAudioSessionControl2),
                                           reinterpret_cast<void **>(&rawTwo)))) {
            continue;
        }

        Session one;
        one.control.reset(rawTwo);
        static_cast<void>(one.control->GetProcessId(&one.process));

        const auto older = std::ranges::find_if(endpoint.sessions, [&one](const Session &was) {
            return was.process == one.process && !was.image.empty();
        });
        one.image = older != endpoint.sessions.end() ? older->image : imageOf(one.process);

        const bool system = one.control->IsSystemSoundsSession() == S_OK;
        const bool ours = settings_.ownProcessId != 0 &&
                          one.process == static_cast<DWORD>(settings_.ownProcessId);
        const bool never = !one.image.empty() && neverAConversation(one.image);
        const bool excluded =
            !one.image.empty() &&
            std::ranges::find(settings_.excluded, one.image) != settings_.excluded.end();

        one.ignored = system || ours || never || excluded;
        found.push_back(std::move(one));
    }

    endpoint.sessions = std::move(found);
}

float WasapiDetector::loudest(const std::vector<Endpoint> &outputs) const
{
    float loudest = 0.0F;
    for (const Endpoint &one : outputs) {
        if (one.meter == nullptr) {
            continue;
        }
        float peak = 0.0F;
        if (SUCCEEDED(one.meter->GetPeakValue(&peak))) {
            loudest = std::max(loudest, peak);
        }
    }
    return loudest;
}

void WasapiDetector::watch()
{
    const ComScope com;

    IMMDeviceEnumerator *rawEnumerator = nullptr;
    if (FAILED(::CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(&rawEnumerator)))) {

        core::Logger::instance().log(core::LogLevel::Warning, "capture",
                                     "this machine's audio engine would not list its devices, so "
                                     "nothing here can watch for a conversation");
        return;
    }
    const ComPtr<IMMDeviceEnumerator> enumerator{rawEnumerator};

    std::vector<Endpoint> microphones;
    std::vector<Endpoint> outputs;
    openEndpoints(*enumerator, eCapture, microphones);
    openEndpoints(*enumerator, eRender, outputs);

    if (microphones.empty() && outputs.empty()) {
        core::Logger::instance().log(core::LogLevel::Warning, "capture",
                                     "the audio engine would not say who is holding a device");
    }

    ConversationGate gate{settings_};
    auto sweptAt = std::chrono::steady_clock::now() - kSweep;

    bool everSaid = false;
    bool saidHeld = false;
    bool saidAnswering = false;

    while (running_.load(std::memory_order_acquire)) {
        const auto now = std::chrono::steady_clock::now();

        if (now - sweptAt >= kSweep) {
            sweptAt = now;

            openEndpoints(*enumerator, eCapture, microphones);
            openEndpoints(*enumerator, eRender, outputs);
            for (Endpoint &one : microphones) {
                enumerate(one);
            }
            for (Endpoint &one : outputs) {
                enumerate(one);
            }
        }

        struct Holder {
            DWORD process{0};
            std::string image;
        };

        std::vector<Holder> holders;
        for (const Endpoint &microphone : microphones) {
            for (const Session &one : microphone.sessions) {
                if (one.ignored) {
                    continue;
                }
                AudioSessionState state = AudioSessionStateInactive;
                if (FAILED(one.control->GetState(&state)) || state != AudioSessionStateActive) {
                    continue;
                }
                holders.push_back(Holder{one.process, one.image});
            }
        }
        const bool held = !holders.empty();

        const bool playing = loudest(outputs) > kAudible;

        const auto playsToo = [&outputs](const Holder &candidate) {
            return std::ranges::any_of(outputs, [&candidate](const Endpoint &output) {
                return std::ranges::any_of(output.sessions, [&candidate](const Session &one) {
                    return one.process == candidate.process ||
                           (!one.image.empty() && !candidate.image.empty() &&
                            plainly(one.image) == plainly(candidate.image));
                });
            });
        };

        const auto talking = std::ranges::find_if(holders, playsToo);
        const bool duplex = talking != holders.end();

        std::string who;
        if (duplex) {
            who = talking->image;
        } else if (held) {
            who = holders.front().image;
        }

        const bool answering = duplex;

        if (!everSaid || held != saidHeld || answering != saidAnswering) {
            everSaid = true;
            saidHeld = held;
            saidAnswering = answering;
            core::Logger::instance().log(
                core::LogLevel::Debug, "capture",
                std::string{"the machine: microphone "} + (held ? "held" : "free") +
                    (who.empty() ? std::string{} : std::string{" by "} + who) + ", output " +
                    (playing ? "playing" : "quiet") + ", the holder " +
                    (duplex ? "has one open" : "has none"));
        }

        {
            const std::lock_guard guard{naming_};
            seen_.application = who;

            seen_.title.clear();
        }

        const auto move = gate.saw(held, answering, playing, now);
        talking_.store(gate.talking(), std::memory_order_release);

        {
            const std::lock_guard guard{trailing_};
            trail_.saw(
                Reading{std::chrono::duration_cast<std::chrono::milliseconds>(now - watching_),
                        held, playing, duplex, gate.talking()});
        }

        if (move == ConversationGate::Move::Began && began_) {
            Conversation what;
            {
                const std::lock_guard guard{naming_};
                what = seen_;
            }
            began_(what);
        } else if (move == ConversationGate::Move::Ended && ended_) {
            ended_();
        }

        std::unique_lock sleeping{sleeping_};
        wake_.wait_for(sleeping, kTick,
                       [this] { return !running_.load(std::memory_order_acquire); });
    }
}

CaptureError WasapiDetector::start(const DetectorSettings &settings, Began began, Ended ended)
{
    stop();

    settings_ = settings;
    began_ = std::move(began);
    ended_ = std::move(ended);
    talking_.store(false, std::memory_order_relaxed);
    {
        const std::lock_guard guard{naming_};
        seen_ = {};
    }
    {
        const std::lock_guard guard{trailing_};
        trail_.forget();
        watching_ = std::chrono::steady_clock::now();
    }

    if (watcher_.joinable()) {
        return CaptureError::Refused;
    }

    running_.store(true, std::memory_order_release);
    watcher_ = std::thread{[this] { watch(); }};
    return CaptureError::None;
}

void WasapiDetector::stop()
{
    running_.store(false, std::memory_order_release);
    wake_.notify_all();

    if (watcher_.joinable() && watcher_.get_id() != std::this_thread::get_id()) {
        watcher_.join();
    }
}

}

std::unique_ptr<IConversationDetector> makeConversationDetector()
{
    return std::make_unique<WasapiDetector>();
}

}
