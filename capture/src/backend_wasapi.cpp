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

#include <softphone/capture/applications.hpp>
#include <softphone/capture/devices.hpp>

#include "com_win32.hpp"
#include "process_win32.hpp"

#include <softphone/core/logging.hpp>

#include <audioclient.h>
#include <audiopolicy.h>
#include <avrt.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>

#include <functiondiscoverykeys_devpkey.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <future>
#include <mutex>
#include <span>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace softphone::capture {
namespace {

using win32::ComPtr;
using win32::ComScope;
using win32::narrow;

constexpr ERole kDefaults[] = {eCommunications, eConsole};

constexpr REFERENCE_TIME kBuffer = 10'000'000;

constexpr DWORD kPoll = 10;

constexpr DWORD kReopen = 500;

constexpr auto kReconsider = std::chrono::seconds{1};

struct Opened {

    ComPtr<IMMDevice> device;

    ComPtr<IAudioClient> client;
    ComPtr<IAudioCaptureClient> reader;

    HANDLE ready{nullptr};

    std::string name;

    std::string id;

    void close() noexcept
    {
        if (client) {
            static_cast<void>(client->Stop());
        }
        reader.reset();
        client.reset();
        device.reset();
        if (ready != nullptr) {
            ::CloseHandle(ready);
            ready = nullptr;
        }
    }
};

[[nodiscard]] std::string friendlyName(IMMDevice &device)
{
    IPropertyStore *rawProperties = nullptr;
    if (FAILED(device.OpenPropertyStore(STGM_READ, &rawProperties))) {
        return {};
    }
    const ComPtr<IPropertyStore> properties{rawProperties};

    PROPVARIANT value;
    ::PropVariantInit(&value);
    if (FAILED(properties->GetValue(PKEY_Device_FriendlyName, &value))) {
        return {};
    }

    auto name = value.vt == VT_LPWSTR ? narrow(value.pwszVal) : std::string{};
    ::PropVariantClear(&value);
    return name;
}

[[nodiscard]] std::string endpointId(IMMDevice &device)
{
    LPWSTR raw = nullptr;
    if (FAILED(device.GetId(&raw))) {
        return {};
    }
    auto id = narrow(raw);
    ::CoTaskMemFree(raw);
    return id;
}

[[nodiscard]] std::string folded(std::string text)
{
    std::ranges::transform(text, text.begin(), [](unsigned char letter) {
        return static_cast<char>(std::tolower(letter));
    });
    return text;
}

[[nodiscard]] ComPtr<IMMDevice> endpointFor(IMMDeviceEnumerator &enumerator, EDataFlow flow,
                                            const std::string &wanted)
{
    IMMDeviceCollection *rawCollection = nullptr;
    if (FAILED(enumerator.EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &rawCollection))) {
        return {};
    }
    const ComPtr<IMMDeviceCollection> collection{rawCollection};

    UINT count = 0;
    if (FAILED(collection->GetCount(&count))) {
        return {};
    }

    const auto asked = folded(wanted);
    for (UINT at = 0; at < count; ++at) {
        IMMDevice *rawDevice = nullptr;
        if (FAILED(collection->Item(at, &rawDevice))) {
            continue;
        }
        ComPtr<IMMDevice> device{rawDevice};
        if (folded(friendlyName(*device)) == asked || endpointId(*device) == wanted) {
            return device;
        }
    }
    return {};
}

[[nodiscard]] bool heldBySomebodyElse(IMMDevice &device)
{
    IAudioSessionManager2 *rawManager = nullptr;
    if (FAILED(device.Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr,
                               reinterpret_cast<void **>(&rawManager)))) {
        return false;
    }
    const ComPtr<IAudioSessionManager2> manager{rawManager};

    IAudioSessionEnumerator *rawSessions = nullptr;
    if (FAILED(manager->GetSessionEnumerator(&rawSessions))) {
        return false;
    }
    const ComPtr<IAudioSessionEnumerator> sessions{rawSessions};

    int count = 0;
    if (FAILED(sessions->GetCount(&count))) {
        return false;
    }

    const DWORD ours = ::GetCurrentProcessId();
    for (int at = 0; at < count; ++at) {
        IAudioSessionControl *rawControl = nullptr;
        if (FAILED(sessions->GetSession(at, &rawControl))) {
            continue;
        }
        const ComPtr<IAudioSessionControl> control{rawControl};

        AudioSessionState state = AudioSessionStateInactive;
        if (FAILED(control->GetState(&state)) || state != AudioSessionStateActive) {
            continue;
        }

        IAudioSessionControl2 *rawTwo = nullptr;
        if (FAILED(control->QueryInterface(__uuidof(IAudioSessionControl2),
                                           reinterpret_cast<void **>(&rawTwo)))) {

            return true;
        }
        const ComPtr<IAudioSessionControl2> two{rawTwo};

        if (two->IsSystemSoundsSession() == S_OK) {
            continue;
        }

        DWORD process = 0;
        if (SUCCEEDED(two->GetProcessId(&process))) {
            if (process == ours) {
                continue;
            }
            const auto image = win32::imageOf(process);
            if (!image.empty() && neverAConversation(image)) {
                continue;
            }
        }
        return true;
    }
    return false;
}

[[nodiscard]] float peakOf(IMMDevice &device)
{
    IAudioMeterInformation *rawMeter = nullptr;
    if (FAILED(device.Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, nullptr,
                               reinterpret_cast<void **>(&rawMeter)))) {
        return 0.0F;
    }
    const ComPtr<IAudioMeterInformation> meter{rawMeter};

    float peak = 0.0F;
    if (FAILED(meter->GetPeakValue(&peak))) {
        return 0.0F;
    }
    return peak;
}

struct Seen {
    std::vector<ComPtr<IMMDevice>> handles;
    std::vector<Device> what;
};

[[nodiscard]] Seen lookAt(IMMDeviceEnumerator &enumerator, EDataFlow flow)
{
    Seen seen;

    std::string consoleId;
    std::string communicationsId;
    for (const ERole role : kDefaults) {
        IMMDevice *rawDefault = nullptr;
        if (SUCCEEDED(enumerator.GetDefaultAudioEndpoint(flow, role, &rawDefault))) {
            const ComPtr<IMMDevice> device{rawDefault};
            (role == eConsole ? consoleId : communicationsId) = endpointId(*device);
        }
    }

    IMMDeviceCollection *rawCollection = nullptr;
    if (FAILED(enumerator.EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &rawCollection))) {
        return seen;
    }
    const ComPtr<IMMDeviceCollection> collection{rawCollection};

    UINT count = 0;
    if (FAILED(collection->GetCount(&count))) {
        return seen;
    }

    seen.handles.reserve(count);
    seen.what.reserve(count);

    for (UINT at = 0; at < count; ++at) {
        IMMDevice *rawDevice = nullptr;
        if (FAILED(collection->Item(at, &rawDevice))) {
            continue;
        }
        ComPtr<IMMDevice> device{rawDevice};

        Device one;
        one.id = endpointId(*device);
        one.name = friendlyName(*device);
        one.console = !one.id.empty() && one.id == consoleId;
        one.communications = !one.id.empty() && one.id == communicationsId;
        one.held = heldBySomebodyElse(*device);

        one.peak = flow == eRender ? peakOf(*device) : 0.0F;

        seen.handles.push_back(std::move(device));
        seen.what.push_back(std::move(one));
    }

    return seen;
}

struct Chosen {
    ComPtr<IMMDevice> device;
    Device what;
};

[[nodiscard]] Chosen theOneToRecord(IMMDeviceEnumerator &enumerator, EDataFlow flow)
{
    auto seen = lookAt(enumerator, flow);
    const auto at =
        flow == eRender ? theOneCarryingSound(seen.what) : theOneBeingListenedTo(seen.what);
    if (at == kNoDevice) {
        return {};
    }
    return Chosen{std::move(seen.handles[at]), std::move(seen.what[at])};
}

class WasapiCapture final : public ISystemCapture {
public:
    ~WasapiCapture() override
    {
        WasapiCapture::stop();
    }

    CaptureError start(const CaptureRequest &request, FrameSink sink) override;
    void stop() override;
    [[nodiscard]] CaptureStatus status() const override;

    [[nodiscard]] Yields mustYieldTheSoundCard() const override
    {
        return Yields::Nothing;
    }

private:

    struct Wire {
        Side side{Side::Near};
        EDataFlow flow{eCapture};
        bool loopback{false};
        std::string wanted;
        std::thread thread;

        std::vector<std::int16_t> silence;
    };

    void run(Wire &wire, std::promise<CaptureError> answer) noexcept;
    [[nodiscard]] CaptureError open(IMMDeviceEnumerator &enumerator, Wire &wire,
                                    Opened &opened) const;

    void drain(IMMDeviceEnumerator &enumerator, Wire &wire, Opened &opened) noexcept;
    void deliver(Side side, const std::int16_t *samples, std::size_t count) noexcept;

    [[nodiscard]] bool waitToStop(DWORD milliseconds) const noexcept
    {
        return ::WaitForSingleObject(stop_, milliseconds) == WAIT_OBJECT_0;
    }

    [[nodiscard]] bool stopping() const noexcept
    {
        return ::WaitForSingleObject(stop_, 0) == WAIT_OBJECT_0;
    }

    HANDLE stop_{nullptr};
    Wire near_{Side::Near, eCapture, false, {}, {}, {}};
    Wire far_{Side::Far, eRender, true, {}, {}, {}};

    CaptureRequest request_;

    FrameSink sink_;

    mutable std::mutex mutex_;
    CaptureStatus status_;
};

CaptureError WasapiCapture::open(IMMDeviceEnumerator &enumerator, Wire &wire, Opened &opened) const
{

    auto device = wire.wanted.empty() ? theOneToRecord(enumerator, wire.flow).device
                                      : endpointFor(enumerator, wire.flow, wire.wanted);
    if (!device) {
        return CaptureError::NoDevice;
    }
    opened.id = endpointId(*device);

    IAudioClient *rawClient = nullptr;
    if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                reinterpret_cast<void **>(&rawClient)))) {
        return CaptureError::NoDevice;
    }
    opened.client.reset(rawClient);

    WAVEFORMATEX want{};
    want.wFormatTag = WAVE_FORMAT_PCM;
    want.nChannels = 1;
    want.nSamplesPerSec = request_.sampleRate;
    want.wBitsPerSample = 16;
    want.nBlockAlign = 2;
    want.nAvgBytesPerSec = request_.sampleRate * 2;
    want.cbSize = 0;

    DWORD flags = AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
    flags |= wire.loopback ? AUDCLNT_STREAMFLAGS_LOOPBACK : AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

    if (FAILED(opened.client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, kBuffer, 0, &want,
                                         nullptr))) {

        return CaptureError::Refused;
    }

    if (!wire.loopback) {
        opened.ready = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (opened.ready == nullptr || FAILED(opened.client->SetEventHandle(opened.ready))) {
            return CaptureError::Refused;
        }
    }

    IAudioCaptureClient *rawReader = nullptr;
    if (FAILED(opened.client->GetService(__uuidof(IAudioCaptureClient),
                                         reinterpret_cast<void **>(&rawReader)))) {
        return CaptureError::Refused;
    }
    opened.reader.reset(rawReader);

    if (FAILED(opened.client->Start())) {
        return CaptureError::Refused;
    }

    opened.name = friendlyName(*device);
    opened.device = std::move(device);
    return CaptureError::None;
}

void WasapiCapture::drain(IMMDeviceEnumerator &enumerator, Wire &wire, Opened &opened) noexcept
{

    DWORD task = 0;
    HANDLE priority = ::AvSetMmThreadCharacteristicsW(L"Capture", &task);
    const auto release = [&priority] {
        if (priority != nullptr) {
            static_cast<void>(::AvRevertMmThreadCharacteristics(priority));
            priority = nullptr;
        }
    };

    const std::array<HANDLE, 2> waited{stop_, opened.ready};

    auto heardAt = std::chrono::steady_clock::now();
    auto lookedAt = heardAt;

    while (!stopping()) {
        bool heard = false;

        if (opened.ready != nullptr) {
            if (::WaitForMultipleObjects(2, waited.data(), FALSE, kPoll) == WAIT_OBJECT_0) {
                break;
            }
        } else if (waitToStop(kPoll)) {
            break;
        }

        for (;;) {
            UINT32 available = 0;
            const HRESULT next = opened.reader->GetNextPacketSize(&available);
            if (next == AUDCLNT_E_DEVICE_INVALIDATED) {

                release();
                return;
            }
            if (FAILED(next) || available == 0) {
                break;
            }

            BYTE *bytes = nullptr;
            UINT32 frames = 0;
            DWORD given = 0;
            const HRESULT got = opened.reader->GetBuffer(&bytes, &frames, &given, nullptr, nullptr);
            if (got == AUDCLNT_E_DEVICE_INVALIDATED) {
                release();
                return;
            }
            if (FAILED(got)) {
                break;
            }

            if ((given & AUDCLNT_BUFFERFLAGS_SILENT) != 0) {

                const auto quiet = std::min<std::size_t>(frames, wire.silence.size());
                deliver(wire.side, wire.silence.data(), quiet);
            } else {
                deliver(wire.side, reinterpret_cast<const std::int16_t *>(bytes), frames);
            }

            heard = heard || frames > 0;
            static_cast<void>(opened.reader->ReleaseBuffer(frames));
        }

        if (wire.wanted.empty()) {
            const auto now = std::chrono::steady_clock::now();
            if (heard) {
                heardAt = now;
            }

            if (now - lookedAt < kReconsider) {
                continue;
            }
            lookedAt = now;

            const bool idle = wire.loopback
                                  ? now - heardAt >= kReconsider
                                  : !(opened.device && heldBySomebodyElse(*opened.device));
            if (!idle) {
                continue;
            }

            const auto seen = lookAt(enumerator, wire.flow);
            const auto at =
                wire.loopback ? theOneCarryingSound(seen.what) : theOneBeingListenedTo(seen.what);

            const auto ours = std::ranges::find_if(
                seen.what, [&opened](const Device &one) { return one.id == opened.id; });
            const bool stillInUse = ours != seen.what.end() && ours->held;

            const bool moving = at != kNoDevice && seen.what[at].id != opened.id &&
                                (seen.what[at].held || seen.what[at].peak > kAudible) &&
                                (wire.loopback || !stillInUse);

            if (moving) {
                core::Logger::instance().log(
                    core::LogLevel::Info, "capture",
                    std::string{"nobody is using "} + opened.name + " and " + seen.what[at].name +
                        " is carrying the conversation: the " +
                        (wire.side == Side::Near ? "microphone" : "far") + " wire follows it");
                release();
                return;
            }
        }
    }

    release();
}

void WasapiCapture::deliver(Side side, const std::int16_t *samples, std::size_t count) noexcept
{
    if (count == 0) {

        return;
    }
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

void WasapiCapture::run(Wire &wire, std::promise<CaptureError> answer) noexcept
{
    const ComScope com;

    bool told = false;

    const auto tell = [&answer, &told](CaptureError error) {
        if (!told) {
            told = true;
            answer.set_value(error);
        }
    };

    IMMDeviceEnumerator *rawEnumerator = nullptr;
    if (FAILED(::CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(&rawEnumerator)))) {

        tell(CaptureError::NoServer);
        return;
    }
    const ComPtr<IMMDeviceEnumerator> enumerator{rawEnumerator};

    bool everOpened = false;
    while (!stopping()) {
        Opened opened;
        const auto error = open(*enumerator, wire, opened);
        if (error != CaptureError::None) {
            opened.close();
            if (!everOpened) {

                tell(error);
                return;
            }
            if (waitToStop(kReopen)) {
                return;
            }
            continue;
        }

        everOpened = true;
        {
            const std::lock_guard guard{mutex_};
            (wire.side == Side::Near ? status_.inputDevice : status_.outputDevice) = opened.name;
        }

        core::Logger::instance().log(core::LogLevel::Info, "capture",
                                     std::string{"the "} +
                                         (wire.side == Side::Near ? "microphone" : "far") +
                                         " wire is recording " + opened.name);

        tell(CaptureError::None);

        drain(*enumerator, wire, opened);
        opened.close();
    }

    tell(CaptureError::None);
}

CaptureError WasapiCapture::start(const CaptureRequest &request, FrameSink sink)
{
    stop();

    request_ = request;
    sink_ = std::move(sink);
    {
        const std::lock_guard guard{mutex_};
        status_ = {};
    }

    near_.wanted = request.inputDevice;
    far_.wanted = request.outputDevice;
    for (Wire *wire : {&far_, &near_}) {
        wire->silence.assign(request.frameSamples, 0);
    }

    stop_ = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (stop_ == nullptr) {
        sink_ = nullptr;
        return CaptureError::NoServer;
    }

    std::promise<CaptureError> farAnswer;
    std::promise<CaptureError> nearAnswer;
    auto farOpened = farAnswer.get_future();
    auto nearOpened = nearAnswer.get_future();

    far_.thread = std::thread{&WasapiCapture::run, this, std::ref(far_), std::move(farAnswer)};
    near_.thread = std::thread{&WasapiCapture::run, this, std::ref(near_), std::move(nearAnswer)};

    const auto opened = std::array{farOpened.get(), nearOpened.get()};
    for (const auto error : opened) {
        if (error != CaptureError::None) {
            stop();
            return error;
        }
    }

    const std::lock_guard guard{mutex_};
    status_.running = true;
    return CaptureError::None;
}

void WasapiCapture::stop()
{
    if (stop_ != nullptr) {
        ::SetEvent(stop_);
    }

    for (Wire *wire : {&far_, &near_}) {
        if (wire->thread.joinable()) {
            wire->thread.join();
        }
    }

    if (stop_ != nullptr) {
        ::CloseHandle(stop_);
        stop_ = nullptr;
    }

    sink_ = nullptr;

    const std::lock_guard guard{mutex_};
    status_.running = false;
}

CaptureStatus WasapiCapture::status() const
{
    const std::lock_guard guard{mutex_};
    return status_;
}

}

std::unique_ptr<ISystemCapture> makeSystemCapture()
{
    return std::make_unique<WasapiCapture>();
}

}
