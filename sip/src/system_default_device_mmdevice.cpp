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

#include <windows.h>

#include <mmdeviceapi.h>

#include <functiondiscoverykeys_devpkey.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>

namespace softphone::sip {
namespace {

[[nodiscard]] std::string narrow(const wchar_t *text)
{
    if (text == nullptr || *text == L'\0') {
        return {};
    }

    const int needed = ::WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (needed <= 1) {
        return {};
    }

    std::string narrowed(static_cast<std::size_t>(needed - 1), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, text, -1, narrowed.data(), needed - 1, nullptr, nullptr);
    return narrowed;
}

class ComScope {
public:
    ComScope()
        : ours_{[] {
              const HRESULT result = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);

              return result == S_OK || result == S_FALSE;
          }()}
    {}

    ComScope(const ComScope &) = delete;
    ComScope &operator=(const ComScope &) = delete;
    ComScope(ComScope &&) = delete;
    ComScope &operator=(ComScope &&) = delete;

    ~ComScope()
    {
        if (ours_) {
            ::CoUninitialize();
        }
    }

private:
    bool ours_;
};

template<typename Interface>
struct ComDeleter {
    void operator()(Interface *pointer) const noexcept
    {
        if (pointer != nullptr) {
            pointer->Release();
        }
    }
};

template<typename Interface>
using ComPtr = std::unique_ptr<Interface, ComDeleter<Interface>>;

constexpr auto kSettling = std::chrono::milliseconds{400};

constexpr ERole kWhichDefault = eConsole;

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

}

std::optional<std::string> systemDefaultDeviceName(core::DeviceRole role)
{
    const ComScope com;

    IMMDeviceEnumerator *rawEnumerator = nullptr;
    if (FAILED(::CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(&rawEnumerator)))) {
        return std::nullopt;
    }
    const ComPtr<IMMDeviceEnumerator> enumerator{rawEnumerator};

    IMMDevice *rawDevice = nullptr;
    const EDataFlow flow = role == core::DeviceRole::Capture ? eCapture : eRender;
    if (FAILED(enumerator->GetDefaultAudioEndpoint(flow, kWhichDefault, &rawDevice)) ||
        rawDevice == nullptr) {

        return std::nullopt;
    }
    const ComPtr<IMMDevice> device{rawDevice};

    auto name = friendlyName(*device);
    if (name.empty()) {
        return std::nullopt;
    }
    return name;
}

namespace {

struct Waking {
    std::atomic<bool> watching{true};
    std::atomic<bool> something{false};
    std::mutex sleeping;
    std::condition_variable wake;
};

class Nudge final : public IMMNotificationClient {
public:
    explicit Nudge(std::shared_ptr<Waking> waking) : waking_{std::move(waking)} {}

    Nudge(const Nudge &) = delete;
    Nudge &operator=(const Nudge &) = delete;
    Nudge(Nudge &&) = delete;
    Nudge &operator=(Nudge &&) = delete;

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return refs_.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const auto left = refs_.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (left == 0) {
            delete this;
        }
        return left;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID asked, void **answer) override
    {
        if (answer == nullptr) {
            return E_POINTER;
        }

        if (::IsEqualGUID(asked, __uuidof(IUnknown)) ||
            ::IsEqualGUID(asked, __uuidof(IMMNotificationClient))) {
            *answer = static_cast<IMMNotificationClient *>(this);
            AddRef();
            return S_OK;
        }
        *answer = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow , ERole ,
                                                     LPCWSTR ) override
    {
        moved();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR , DWORD ) override
    {
        moved();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR ) override
    {
        moved();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR ) override
    {
        moved();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR ,
                                                     const PROPERTYKEY ) override
    {
        return S_OK;
    }

private:

    void moved() noexcept
    {
        waking_->something.store(true, std::memory_order_release);
        waking_->wake.notify_one();
    }

    ~Nudge() = default;

    std::atomic<ULONG> refs_{1};
    std::shared_ptr<Waking> waking_;
};

class EndpointWatch final : public DeviceWatch {
public:
    explicit EndpointWatch(std::function<void()> moved)
        : moved_{std::move(moved)}, waking_{std::make_shared<Waking>()}
    {
        settling_ = std::thread{[this]() { listen(); }};
    }

    EndpointWatch(const EndpointWatch &) = delete;
    EndpointWatch &operator=(const EndpointWatch &) = delete;
    EndpointWatch(EndpointWatch &&) = delete;
    EndpointWatch &operator=(EndpointWatch &&) = delete;

    ~EndpointWatch() override
    {
        waking_->watching.store(false, std::memory_order_release);
        waking_->wake.notify_all();
        if (settling_.joinable()) {
            settling_.join();
        }
    }

private:

    void listen()
    {
        const ComScope com;

        IMMDeviceEnumerator *rawEnumerator = nullptr;
        if (FAILED(::CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                      IID_PPV_ARGS(&rawEnumerator)))) {

            settle();
            return;
        }
        const ComPtr<IMMDeviceEnumerator> enumerator{rawEnumerator};

        const ComPtr<Nudge> nudge{new Nudge{waking_}};
        if (FAILED(enumerator->RegisterEndpointNotificationCallback(nudge.get()))) {
            settle();
            return;
        }

        settle();

        static_cast<void>(enumerator->UnregisterEndpointNotificationCallback(nudge.get()));
    }

    void settle()
    {
        while (waking_->watching.load(std::memory_order_acquire)) {
            {
                std::unique_lock sleeping{waking_->sleeping};
                waking_->wake.wait(sleeping, [this] {
                    return waking_->something.load(std::memory_order_acquire) ||
                           !waking_->watching.load(std::memory_order_acquire);
                });
            }
            if (!waking_->watching.load(std::memory_order_acquire)) {
                return;
            }

            std::this_thread::sleep_for(kSettling);
            waking_->something.store(false, std::memory_order_release);
            if (waking_->watching.load(std::memory_order_acquire) && moved_) {
                moved_();
            }
        }
    }

    std::function<void()> moved_;
    std::shared_ptr<Waking> waking_;
    std::thread settling_;
};

}

std::unique_ptr<DeviceWatch> watchTheMachinesDevices(std::function<void()> moved)
{
    if (!moved) {
        return {};
    }
    return std::make_unique<EndpointWatch>(std::move(moved));
}

}
