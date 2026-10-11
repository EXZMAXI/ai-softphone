// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/updates.hpp>

#include <windows.h>

#include <string>
#include <vector>

namespace softphone::core {
namespace {

[[nodiscard]] std::string explain(DWORD error)
{
    LPWSTR buffer = nullptr;
    const DWORD length = ::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);

    if (length == 0 || buffer == nullptr) {
        return "error " + std::to_string(error);
    }

    std::wstring wide{buffer, length};
    ::LocalFree(buffer);

    while (!wide.empty() && (wide.back() == L'\r' || wide.back() == L'\n')) {
        wide.pop_back();
    }

    const int needed = ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                                             nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return "error " + std::to_string(error);
    }

    std::string narrow(static_cast<std::size_t>(needed), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), narrow.data(),
                          needed, nullptr, nullptr);
    return narrow;
}

}

bool runInstaller(const std::filesystem::path &installer, std::string &complaint)
{
    complaint.clear();

    std::error_code failure;
    if (!std::filesystem::exists(installer, failure) || failure) {
        complaint = "the downloaded installer is no longer there";
        return false;
    }

    const std::wstring command =
        L"\"" + installer.wstring() + L"\" /SILENT /CLOSEAPPLICATIONS /NORESTART /RELAUNCH=1";

    std::vector<wchar_t> mutableCommand{command.begin(), command.end()};
    mutableCommand.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};

    const auto directory = installer.parent_path().wstring();

    if (::CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE, 0, nullptr,
                         directory.empty() ? nullptr : directory.c_str(), &startup,
                         &process) == FALSE) {
        complaint = "the installer would not start: " + explain(::GetLastError());
        return false;
    }

    ::CloseHandle(process.hThread);
    ::CloseHandle(process.hProcess);
    return true;
}

}
