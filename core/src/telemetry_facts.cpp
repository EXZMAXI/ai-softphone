// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/telemetry_service.hpp>

#include <softphone/core/countries.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/utsname.h>
#else
#include <sys/utsname.h>
#endif

namespace softphone::core {
namespace {

constexpr std::size_t kLongest = 64;

[[nodiscard]] std::string tidied(std::string_view text)
{
    std::string out;
    out.reserve(std::min(text.size(), kLongest));
    for (const char letter : text) {
        if (out.size() >= kLongest) {
            break;
        }
        const auto one = static_cast<unsigned char>(letter);
        if (std::isalnum(one) != 0 || letter == '.' || letter == '_' || letter == '-' ||
            letter == '/' || letter == ' ') {
            out.push_back(letter);
        }
    }

    while (!out.empty() && out.back() == ' ') {
        out.pop_back();
    }
    while (!out.empty() && out.front() == ' ') {
        out.erase(out.begin());
    }
    return out;
}

[[nodiscard]] std::string architecture()
{

#if defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#elif defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#else
    return "";
#endif
}

[[nodiscard]] std::string asLocale(std::string_view raw)
{
    std::string out;
    for (const char letter : raw) {
        if (letter == '.' || letter == '@') {
            break;
        }
        out.push_back(letter == '-' ? '_' : letter);
    }
    return tidied(out);
}

[[nodiscard]] std::string timezone()
{

#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
    try {
        return tidied(std::chrono::current_zone()->name());
    } catch (...) {
    }
#endif

#ifndef _WIN32

    std::error_code error;
    const auto target = std::filesystem::read_symlink("/etc/localtime", error);
    if (!error) {
        const auto text = target.generic_string();
        if (const auto at = text.find("/zoneinfo/"); at != std::string::npos) {
            return tidied(text.substr(at + 10));
        }
    }
#endif
    return {};
}

#ifdef _WIN32

[[nodiscard]] std::string systemVersion()
{
    using Asking = LONG(WINAPI *)(PRTL_OSVERSIONINFOW);
    const HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
    if (ntdll == nullptr) {
        return {};
    }
    const auto ask = reinterpret_cast<Asking>(
        reinterpret_cast<void *>(::GetProcAddress(ntdll, "RtlGetVersion")));
    if (ask == nullptr) {
        return {};
    }

    RTL_OSVERSIONINFOW said{};
    said.dwOSVersionInfoSize = sizeof(said);
    if (ask(&said) != 0) {
        return {};
    }
    return std::to_string(said.dwMajorVersion) + "." + std::to_string(said.dwMinorVersion) + "." +
           std::to_string(said.dwBuildNumber);
}

[[nodiscard]] std::string systemLocale()
{
    wchar_t name[LOCALE_NAME_MAX_LENGTH]{};
    if (::GetUserDefaultLocaleName(name, LOCALE_NAME_MAX_LENGTH) == 0) {
        return {};
    }

    std::string out;
    for (const wchar_t letter : std::wstring_view{name}) {
        if (letter == L'\0') {
            break;
        }
        out.push_back(static_cast<char>(letter));
    }
    return asLocale(out);
}

#else

[[nodiscard]] std::string systemVersion()
{
#ifdef __APPLE__
    std::string answer(64, '\0');
    std::size_t size = answer.size();
    if (::sysctlbyname("kern.osproductversion", answer.data(), &size, nullptr, 0) == 0) {
        answer.resize(size == 0 ? 0 : size - 1);
        return tidied(answer);
    }
#else

    std::ifstream release{"/etc/os-release"};
    std::string line;
    std::string id;
    std::string version;
    while (std::getline(release, line)) {
        const auto value = [&line](std::string_view prefix) {
            auto rest = std::string_view{line}.substr(prefix.size());
            if (!rest.empty() && rest.front() == '"') {
                rest.remove_prefix(1);
            }
            if (!rest.empty() && rest.back() == '"') {
                rest.remove_suffix(1);
            }
            return std::string{rest};
        };
        if (line.starts_with("ID=")) {
            id = value("ID=");
        } else if (line.starts_with("VERSION_ID=")) {
            version = value("VERSION_ID=");
        }
    }
    if (!id.empty()) {
        return tidied(version.empty() ? id : id + " " + version);
    }
#endif

    ::utsname machine{};
    if (::uname(&machine) == 0) {
        return tidied(machine.release);
    }
    return {};
}

[[nodiscard]] std::string systemLocale()
{

    for (const char *name : {"LC_ALL", "LC_MESSAGES", "LANG"}) {
        if (const char *told = std::getenv(name); told != nullptr && *told != '\0') {
            const auto locale = asLocale(told);

            if (locale != "C" && locale != "POSIX" && !locale.empty()) {
                return locale;
            }
        }
    }
    return {};
}

#endif

}

TelemetryService::Facts machineFacts()
{
    TelemetryService::Facts facts;
#ifdef _WIN32
    facts.osFamily = "windows";
#elif defined(__APPLE__)
    facts.osFamily = "macos";
#else
    facts.osFamily = "linux";
#endif
    facts.osVersion = systemVersion();
    facts.osArch = architecture();
    facts.osLocale = systemLocale();

    facts.osCountry = machineCountry();
    facts.osTimezone = timezone();
    return facts;
}

}
