// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "autostart_win32.hpp"

#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>

#include <windows.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace softphone::ui {
namespace {

[[nodiscard]] std::wstring widen(const std::string &text)
{
    if (text.empty()) {
        return {};
    }

    const int needed =
        ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (needed <= 0) {
        return {};
    }

    std::wstring wide(static_cast<std::size_t>(needed), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(),
                          needed);
    return wide;
}

[[nodiscard]] std::string narrow(const std::wstring &text)
{
    if (text.empty()) {
        return {};
    }

    const int needed = ::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                                             nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return {};
    }

    std::string narrowed(static_cast<std::size_t>(needed), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), narrowed.data(),
                          needed, nullptr, nullptr);
    return narrowed;
}

struct KeyCloser {
    void operator()(HKEY key) const noexcept
    {
        if (key != nullptr) {
            ::RegCloseKey(key);
        }
    }
};

using Key = std::unique_ptr<std::remove_pointer_t<HKEY>, KeyCloser>;

[[nodiscard]] Key openForReading(const std::string &path)
{
    HKEY raw = nullptr;
    if (::RegOpenKeyExW(HKEY_CURRENT_USER, widen(path).c_str(), 0, KEY_QUERY_VALUE, &raw) !=
        ERROR_SUCCESS) {
        return nullptr;
    }
    return Key{raw};
}

[[nodiscard]] Key openForWriting(const std::string &path)
{
    HKEY raw = nullptr;
    if (::RegCreateKeyExW(HKEY_CURRENT_USER, widen(path).c_str(), 0, nullptr,
                          REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &raw,
                          nullptr) != ERROR_SUCCESS) {
        return nullptr;
    }
    return Key{raw};
}

[[nodiscard]] std::string readString(const std::string &path, const char *name)
{
    const auto key = openForReading(path);
    if (key == nullptr) {
        return {};
    }

    const auto wideName = widen(name);
    DWORD bytes = 0;
    DWORD type = 0;
    if (::RegQueryValueExW(key.get(), wideName.c_str(), nullptr, &type, nullptr, &bytes) !=
            ERROR_SUCCESS ||
        (type != REG_SZ && type != REG_EXPAND_SZ) || bytes == 0) {
        return {};
    }

    std::wstring value(bytes / sizeof(wchar_t), L'\0');
    if (::RegQueryValueExW(key.get(), wideName.c_str(), nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(value.data()), &bytes) != ERROR_SUCCESS) {
        return {};
    }

    while (!value.empty() && value.back() == L'\0') {
        value.pop_back();
    }
    return narrow(value);
}

[[nodiscard]] bool approvedBySession(const std::string &approvedKey, const char *name)
{
    const auto key = openForReading(approvedKey);
    if (key == nullptr) {
        return true;
    }

    const auto wideName = widen(name);
    std::array<BYTE, 32> flags{};
    DWORD bytes = static_cast<DWORD>(flags.size());
    if (::RegQueryValueExW(key.get(), wideName.c_str(), nullptr, nullptr, flags.data(), &bytes) !=
            ERROR_SUCCESS ||
        bytes == 0) {
        return true;
    }

    return (flags[0] & 1U) == 0U;
}

}

std::filesystem::path runningBinary()
{

    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;) {
        const DWORD wrote =
            ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (wrote == 0) {
            return {};
        }
        if (wrote < buffer.size()) {
            return std::filesystem::path{std::wstring{buffer.data(), wrote}};
        }
        buffer.resize(buffer.size() * 2);
    }
}

RunKeyAutostart::RunKeyAutostart(std::string key, std::filesystem::path binary)
    : key_{std::move(key)}, binary_{std::move(binary)}
{}

std::string RunKeyAutostart::command() const
{
    return readString(key_, kRunValue);
}

AutostartState RunKeyAutostart::state() const
{
    auto state = readRunCommand(command());
    if (state.present) {
        state.enabled = approvedBySession(kApprovedKey, kRunValue);
    }
    return state;
}

bool RunKeyAutostart::enable(bool startMinimised)
{

    if (binary_.empty()) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot ask the session to start a binary it cannot name");
        return false;
    }

    const auto key = openForWriting(key_);
    if (key == nullptr) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot open " + key_ + " to write the autostart entry");
        return false;
    }

    const auto value = widen(runCommand(core::asUtf8(binary_), startMinimised));
    const auto bytes = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));

    if (::RegSetValueExW(key.get(), widen(kRunValue).c_str(), 0, REG_SZ,
                         reinterpret_cast<const BYTE *>(value.c_str()), bytes) != ERROR_SUCCESS) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot write the autostart entry under " + key_);
        return false;
    }

    core::Logger::instance().log(core::LogLevel::Info, "ui",
                                 "the session will start " + core::asUtf8(binary_) +
                                     (startMinimised ? " minimised" : ""));
    return true;
}

bool RunKeyAutostart::disable()
{
    const auto key = openForWriting(key_);
    if (key == nullptr) {

        return true;
    }

    const auto removed = ::RegDeleteValueW(key.get(), widen(kRunValue).c_str());

    if (removed != ERROR_SUCCESS && removed != ERROR_FILE_NOT_FOUND) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot remove the autostart entry under " + key_);
        return false;
    }

    return true;
}

std::unique_ptr<IAutostart> makeAutostart()
{
    return std::make_unique<RunKeyAutostart>();
}

}
