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
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <system_error>
#include <unistd.h>

#if defined(__GLIBC__)
#include <execinfo.h>
#endif

namespace softphone::core {
namespace {

using crashDetail::destination;

constexpr std::array kSignals{SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT};

[[nodiscard]] std::string_view nameOf(int signalNumber) noexcept
{
    switch (signalNumber) {
    case SIGSEGV:
        return "SIGSEGV, a read or write through a pointer that was not there";
    case SIGBUS:
        return "SIGBUS, an address the hardware would not accept";
    case SIGILL:
        return "SIGILL, an instruction that is not one";
    case SIGFPE:
        return "SIGFPE, arithmetic that has no answer";
    case SIGABRT:
        return "SIGABRT, the program stopped itself — an assertion, or an "
               "exception nobody caught";
    default:
        return "a signal this build has no name for";
    }
}

extern "C" void onFatalSignal(int signalNumber)
{
    crashDetail::nameTheFiles();

    const int descriptor = ::open(destination().report.data(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (descriptor >= 0) {
        crashDetail::writeReport(descriptor, nameOf(signalNumber));

#if defined(__GLIBC__)

        static_cast<void>(::write(descriptor, "\nthe stack, newest frame first:\n", 32));
        std::array<void *, 64> frames{};
        const auto depth = ::backtrace(frames.data(), static_cast<int>(frames.size()));
        ::backtrace_symbols_fd(frames.data(), depth, descriptor);
#endif

        ::close(descriptor);
    }

    ::signal(signalNumber, SIG_DFL);
    ::raise(signalNumber);
}

}

void reportAndEnd(std::string_view reason) noexcept
{

    crashDetail::nameTheFiles();

    const int descriptor = ::open(destination().report.data(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (descriptor >= 0) {
        crashDetail::writeReport(descriptor, reason);

#if defined(__GLIBC__)
        static_cast<void>(::write(descriptor, "\nthe stack, newest frame first:\n", 32));
        std::array<void *, 64> frames{};
        const auto depth = ::backtrace(frames.data(), static_cast<int>(frames.size()));
        ::backtrace_symbols_fd(frames.data(), depth, descriptor);
#endif

        ::close(descriptor);
    }

    ::_exit(3);
}

bool writeDumpOfAnotherProcess(unsigned long, unsigned long, unsigned long long,
                               const std::string &) noexcept
{

    return false;
}

bool installCrashHandler(const std::filesystem::path &directory)
{
    std::error_code failure;
    std::filesystem::create_directories(directory, failure);

    auto &where = destination();
    auto text = directory.string();
    if (!text.empty() && text.back() != '/') {
        text += '/';
    }
    where.directoryLength = crashDetail::append(where.directory, 0, text);

    crashDetail::prepareBanner();

    crashDetail::rememberPreviousRun();

    const auto marker = directory / crashDetail::markerName();
    const int descriptor = ::open(marker.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (descriptor >= 0) {
        ::close(descriptor);
    }

    struct sigaction action {};

    action.sa_handler = &onFatalSignal;
    sigemptyset(&action.sa_mask);

    action.sa_flags = SA_RESTART | SA_ONSTACK;

    static constexpr std::size_t kAlternateStackSize = 64 * 1024;
    static std::array<char, kAlternateStackSize> alternateStack{};
    stack_t stack{};
    stack.ss_sp = alternateStack.data();
    stack.ss_size = alternateStack.size();
    stack.ss_flags = 0;
    static_cast<void>(::sigaltstack(&stack, nullptr));

    for (const int signalNumber : kSignals) {
        static_cast<void>(::sigaction(signalNumber, &action, nullptr));
    }

    return true;
}

}
