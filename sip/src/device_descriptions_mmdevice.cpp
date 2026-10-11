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

#include <windows.h>

#include <mmdeviceapi.h>

#include <functiondiscoverykeys_devpkey.h>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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

void collect(IMMDeviceEnumerator &enumerator, EDataFlow flow, core::DeviceRole role,
             std::vector<AudioEndpoint> &into)
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

    for (UINT at = 0; at < count; ++at) {
        IMMDevice *rawDevice = nullptr;
        if (FAILED(collection->Item(at, &rawDevice))) {
            continue;
        }
        const ComPtr<IMMDevice> device{rawDevice};

        if (auto name = friendlyName(*device); !name.empty()) {
            into.push_back(AudioEndpoint{std::move(name), role});
        }
    }
}

[[nodiscard]] std::vector<AudioEndpoint> endpointWords()
{
    const ComScope com;

    IMMDeviceEnumerator *rawEnumerator = nullptr;
    if (FAILED(::CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(&rawEnumerator)))) {
        return {};
    }
    const ComPtr<IMMDeviceEnumerator> enumerator{rawEnumerator};

    std::vector<AudioEndpoint> endpoints;
    collect(*enumerator, eCapture, core::DeviceRole::Capture, endpoints);
    collect(*enumerator, eRender, core::DeviceRole::Playback, endpoints);
    return endpoints;
}

}

[[nodiscard]] bool isUtf8(const std::string &text) noexcept
{
    return ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                 static_cast<int>(text.size()), nullptr, 0) > 0;
}

std::string asText(std::string reported)
{
    if (reported.empty() || isUtf8(reported)) {
        return reported;
    }

    const int wide = ::MultiByteToWideChar(CP_ACP, 0, reported.data(),
                                           static_cast<int>(reported.size()), nullptr, 0);
    if (wide <= 0) {
        return reported;
    }

    std::wstring widened(static_cast<std::size_t>(wide), L'\0');
    ::MultiByteToWideChar(CP_ACP, 0, reported.data(), static_cast<int>(reported.size()),
                          widened.data(), wide);

    const int narrow8 =
        ::WideCharToMultiByte(CP_UTF8, 0, widened.data(), wide, nullptr, 0, nullptr, nullptr);
    if (narrow8 <= 0) {
        return reported;
    }

    std::string narrowed(static_cast<std::size_t>(narrow8), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, widened.data(), wide, narrowed.data(), narrow8, nullptr,
                          nullptr);
    return narrowed;
}

std::vector<core::AudioDeviceInfo> described(std::vector<core::AudioDeviceInfo> devices)
{

    for (auto &device : devices) {
        device.name = asText(std::move(device.name));
    }

    const auto endpoints = endpointWords();
    return describedWithEndpoints(std::move(devices), endpoints);
}

}
