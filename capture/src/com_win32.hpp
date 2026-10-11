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

#include <windows.h>

#include <memory>
#include <string>

namespace softphone::capture::win32 {

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

[[nodiscard]] inline std::string narrow(const wchar_t *text)
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

}
