// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/crash.hpp>

#include "crash_detail.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

#include <windows.h>

#include <dbghelp.h>
#include <io.h>

#include <fcntl.h>
#include <sys/stat.h>

namespace softphone::core {
namespace {

using crashDetail::destination;
using crashDetail::kPathLength;

[[nodiscard]] std::string_view nameOf(DWORD code) noexcept
{
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
        return "access violation, a read or write through a pointer that was not there";
    case EXCEPTION_STACK_OVERFLOW:
        return "stack overflow";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        return "illegal instruction";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        return "division by zero";
    case EXCEPTION_IN_PAGE_ERROR:
        return "in-page error, memory the system could not fetch";
    default:
        return "an exception this build has no name for";
    }
}

constexpr MINIDUMP_TYPE kDumpType = static_cast<MINIDUMP_TYPE>(
    MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory | MiniDumpWithThreadInfo);

void writeDump(EXCEPTION_POINTERS *exception) noexcept
{
    const HANDLE file = ::CreateFileA(destination().dump.data(), GENERIC_WRITE, 0, nullptr,
                                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }

    MINIDUMP_EXCEPTION_INFORMATION information{};
    information.ThreadId = ::GetCurrentThreadId();
    information.ExceptionPointers = exception;
    information.ClientPointers = FALSE;

    static_cast<void>(::MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), file,
                                          kDumpType, exception == nullptr ? nullptr : &information,
                                          nullptr, nullptr));
    ::CloseHandle(file);
}

std::size_t appendNumber(wchar_t *into, std::size_t at, std::size_t room,
                         unsigned long long value) noexcept
{
    std::array<wchar_t, 24> digits{};
    std::size_t count = 0;
    do {
        digits[count++] = static_cast<wchar_t>(L'0' + (value % 10));
        value /= 10;
    } while (value != 0 && count < digits.size());

    while (count > 0 && at + 1 < room) {
        into[at++] = digits[--count];
    }
    return at;
}

std::size_t appendNarrow(wchar_t *into, std::size_t at, std::size_t room,
                         std::string_view text) noexcept
{
    for (const char one : text) {
        if (at + 1 >= room) {
            break;
        }
        into[at++] = static_cast<wchar_t>(static_cast<unsigned char>(one));
    }
    return at;
}

constexpr DWORD kHandlerPatience = 10000;

constexpr DWORD kOutsidePatience = 8000;

struct Self {
    std::array<wchar_t, kPathLength> path{};
    std::size_t length{0};

    std::array<wchar_t, kPathLength> directory{};
    std::size_t directoryLength{0};
};

[[nodiscard]] Self &self() noexcept
{
    static Self only;
    return only;
}

[[nodiscard]] bool writeDumpFromOutside(EXCEPTION_POINTERS *exception) noexcept
{
    if (self().length == 0) {
        return false;
    }

    std::array<wchar_t, kPathLength * 2 + 128> line{};
    std::size_t at = 0;

    const auto put = [&line, &at](wchar_t one) noexcept {
        if (at + 1 < line.size()) {
            line[at++] = one;
        }
    };

    put(L'"');
    for (std::size_t i = 0; i < self().length; ++i) {
        put(self().path[i]);
    }
    put(L'"');
    put(L' ');
    at = appendNarrow(line.data(), at, line.size(), kWriteCrashDumpOption);
    put(L' ');
    at = appendNumber(line.data(), at, line.size(), ::GetCurrentProcessId());
    put(L' ');
    at = appendNumber(line.data(), at, line.size(), ::GetCurrentThreadId());
    put(L' ');
    at = appendNumber(line.data(), at, line.size(), reinterpret_cast<std::uintptr_t>(exception));
    put(L' ');
    put(L'"');

    for (std::size_t i = 0; i < self().directoryLength; ++i) {
        put(self().directory[i]);
    }

    const std::size_t whole = std::strlen(destination().dump.data());
    if (whole > destination().directoryLength) {
        at =
            appendNarrow(line.data(), at, line.size(),
                         std::string_view{destination().dump.data() + destination().directoryLength,
                                          whole - destination().directoryLength});
    }
    put(L'"');
    line[at] = L'\0';

    STARTUPINFOW how{};
    how.cb = sizeof how;
    PROCESS_INFORMATION started{};

    if (::CreateProcessW(nullptr, line.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                         nullptr, &how, &started) == 0) {
        return false;
    }

    const bool finished =
        ::WaitForSingleObject(started.hProcess, kOutsidePatience) == WAIT_OBJECT_0;

    DWORD answer = 1;
    if (finished) {
        static_cast<void>(::GetExitCodeProcess(started.hProcess, &answer));
    }

    ::CloseHandle(started.hThread);
    ::CloseHandle(started.hProcess);
    return finished && answer == 0;
}

void watchTheHandler() noexcept
{
    const HANDLE watchdog = ::CreateThread(
        nullptr, 0,
        [](LPVOID) -> DWORD {
            ::Sleep(kHandlerPatience);

            ::TerminateProcess(::GetCurrentProcess(), 3);
            return 0;
        },
        nullptr, 0, nullptr);
    if (watchdog != nullptr) {
        ::CloseHandle(watchdog);
    }
}

LONG WINAPI onUnhandledException(EXCEPTION_POINTERS *exception) noexcept
{
    crashDetail::nameTheFiles();

    const int descriptor =
        ::_open(destination().report.data(), _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY,
                _S_IREAD | _S_IWRITE);
    if (descriptor >= 0) {
        crashDetail::writeReport(
            descriptor, exception == nullptr ? std::string_view{"the program stopped itself"}
                                             : nameOf(exception->ExceptionRecord->ExceptionCode));
        static_cast<void>(
            ::_write(descriptor, "\nthe stacks are in the .dmp beside this file\n", 45));
        ::_close(descriptor);
    }

    watchTheHandler();

    if (!writeDumpFromOutside(exception)) {
        writeDump(exception);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

void onInvalidParameter(const wchar_t *, const wchar_t *, const wchar_t *, unsigned int,
                        uintptr_t) noexcept
{
    onUnhandledException(nullptr);
    ::TerminateProcess(::GetCurrentProcess(), 3);
}

void onTerminate() noexcept
{
    onUnhandledException(nullptr);
    ::TerminateProcess(::GetCurrentProcess(), 3);
}

}

bool writeDumpOfAnotherProcess(unsigned long processId, unsigned long threadId,
                               unsigned long long exceptionPointers,
                               const std::string &dumpFile) noexcept
{

    const HANDLE faulted = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                                         static_cast<DWORD>(processId));
    if (faulted == nullptr) {
        return false;
    }

    const HANDLE file = ::CreateFileA(dumpFile.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        ::CloseHandle(faulted);
        return false;
    }

    MINIDUMP_EXCEPTION_INFORMATION information{};
    information.ThreadId = static_cast<DWORD>(threadId);
    information.ExceptionPointers =
        reinterpret_cast<EXCEPTION_POINTERS *>(static_cast<std::uintptr_t>(exceptionPointers));

    information.ClientPointers = TRUE;

    const BOOL written =
        ::MiniDumpWriteDump(faulted, static_cast<DWORD>(processId), file, kDumpType,
                            exceptionPointers == 0 ? nullptr : &information, nullptr, nullptr);

    ::CloseHandle(file);
    ::CloseHandle(faulted);
    return written != FALSE;
}

bool installCrashHandler(const std::filesystem::path &directory)
{
    std::error_code failure;
    std::filesystem::create_directories(directory, failure);

    auto &where = destination();
    auto text = directory.string();
    if (!text.empty() && text.back() != '\\' && text.back() != '/') {
        text += '\\';
    }
    where.directoryLength = crashDetail::append(where.directory, 0, text);

    crashDetail::prepareBanner();

    {
        const DWORD length = ::GetModuleFileNameW(nullptr, self().path.data(),
                                                  static_cast<DWORD>(self().path.size()));
        self().length = length > 0 && length < self().path.size() ? length : 0;

        const auto wide = directory.wstring();
        auto &kept = self().directory;
        std::size_t at = 0;
        for (const wchar_t one : wide) {
            if (at + 2 >= kept.size()) {
                break;
            }
            kept[at++] = one;
        }
        if (at > 0 && kept[at - 1] != L'\\' && kept[at - 1] != L'/' && at + 1 < kept.size()) {
            kept[at++] = L'\\';
        }
        self().directoryLength = at;
    }

    crashDetail::rememberPreviousRun();

    const auto marker = directory / crashDetail::markerName();
    const HANDLE file = ::CreateFileW(marker.wstring().c_str(), GENERIC_WRITE, 0, nullptr,
                                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE) {
        ::CloseHandle(file);
    }

    ::SetUnhandledExceptionFilter(&onUnhandledException);
    ::_set_invalid_parameter_handler(&onInvalidParameter);
    std::set_terminate(&onTerminate);

    return true;
}

}
