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

#include "com_win32.hpp"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace softphone::capture::win32 {

[[nodiscard]] inline std::string imageOf(DWORD process)
{
    if (process == 0) {
        return {};
    }

    const HANDLE opened = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process);
    if (opened == nullptr) {
        return {};
    }

    std::wstring path(MAX_PATH, wchar_t{0});
    auto size = static_cast<DWORD>(path.size());
    const bool told = ::QueryFullProcessImageNameW(opened, 0, path.data(), &size) != 0;
    ::CloseHandle(opened);
    if (!told) {
        return {};
    }

    path.resize(size);
    auto name = narrow(path.c_str());
    if (const auto slash = name.find_last_of("\\/"); slash != std::string::npos) {
        name = name.substr(slash + 1);
    }
    std::ranges::transform(name, name.begin(), [](unsigned char letter) {
        return static_cast<char>(std::tolower(letter));
    });
    return name;
}

}
