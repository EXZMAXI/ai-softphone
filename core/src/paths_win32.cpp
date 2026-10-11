// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/paths.hpp>

#include <windows.h>

#include <aclapi.h>
#include <knownfolders.h>
#include <shlobj.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>

namespace softphone::core {
namespace {

struct ShellDeleter {
    void operator()(wchar_t *memory) const noexcept
    {
        ::CoTaskMemFree(memory);
    }
};

[[nodiscard]] std::filesystem::path knownFolder(const KNOWNFOLDERID &folder, const char *variable)
{
    wchar_t *raw = nullptr;
    if (SUCCEEDED(::SHGetKnownFolderPath(folder, KF_FLAG_CREATE, nullptr, &raw))) {
        const std::unique_ptr<wchar_t, ShellDeleter> owned{raw};
        return std::filesystem::path{owned.get()};
    }

    if (const auto *found = std::getenv(variable); found != nullptr && *found != '\0') {
        return std::filesystem::path{found};
    }

    return std::filesystem::path{"."};
}

[[nodiscard]] std::filesystem::path roaming()
{
    return knownFolder(FOLDERID_RoamingAppData, "APPDATA") / "ai-softphone";
}

[[nodiscard]] std::filesystem::path local()
{
    return knownFolder(FOLDERID_LocalAppData, "LOCALAPPDATA") / "ai-softphone";
}

[[nodiscard]] std::filesystem::path beside()
{
    std::wstring buffer(MAX_PATH, L'\0');
    for (;;) {
        const auto written =
            ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (written == 0) {
            return {};
        }
        if (written < buffer.size()) {
            buffer.resize(written);
            break;
        }

        if (buffer.size() >= 32768) {
            return {};
        }
        buffer.resize(buffer.size() * 2);
    }
    return std::filesystem::path{buffer}.parent_path();
}

[[nodiscard]] std::filesystem::path profileAsked()
{

    const std::wstring name{kProfileVariable.begin(), kProfileVariable.end()};

    if (const auto *asked = ::_wgetenv(name.c_str()); asked != nullptr && *asked != L'\0') {

        return std::filesystem::path{asked}.make_preferred();
    }
    return {};
}

[[nodiscard]] const ChosenLocations &chosen()
{

    if (auto wanted = profileAsked(); !wanted.empty()) {
        static ChosenLocations sandbox;
        if (sandbox.profile != wanted) {
            std::error_code failed;
            std::filesystem::create_directories(wanted, failed);
            sandbox.profile = std::filesystem::is_directory(wanted, failed)
                                  ? std::move(wanted)
                                  : std::filesystem::path{};
        }
        if (!sandbox.profile.empty()) {
            return sandbox;
        }
    }

    static const ChosenLocations found = [] {
        const auto directory = beside();
        if (directory.empty()) {
            return ChosenLocations{};
        }

        std::ifstream document{directory / std::filesystem::path{kLocationsFile}, std::ios::binary};
        if (!document.is_open()) {
            return ChosenLocations{};
        }
        std::ostringstream text;
        text << document.rdbuf();

        auto asked = locationsIn(text.str());

        const auto usable = [](const std::filesystem::path &where) {
            if (where.empty()) {
                return false;
            }
            std::error_code failed;
            std::filesystem::create_directories(where, failed);
            return std::filesystem::is_directory(where, failed);
        };

        if (!usable(asked.profile)) {
            asked.profile.clear();
        }
        if (!usable(asked.recordings)) {
            asked.recordings.clear();
        }
        return asked;
    }();
    return found;
}

[[nodiscard]] std::unique_ptr<std::byte[]> currentUser(PSID &sid)
{
    HANDLE raw = nullptr;
    if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &raw) == FALSE) {
        return nullptr;
    }

    DWORD needed = 0;
    ::GetTokenInformation(raw, TokenUser, nullptr, 0, &needed);
    if (needed == 0) {
        ::CloseHandle(raw);
        return nullptr;
    }

    auto buffer = std::make_unique<std::byte[]>(needed);
    const BOOL asked = ::GetTokenInformation(raw, TokenUser, buffer.get(), needed, &needed);
    ::CloseHandle(raw);

    if (asked == FALSE) {
        return nullptr;
    }

    sid = reinterpret_cast<TOKEN_USER *>(buffer.get())->User.Sid;
    return buffer;
}

}

std::filesystem::path configDirectory()
{

    const auto &asked = chosen();
    return asked.profile.empty() ? roaming() : asked.profile / "config";
}

std::filesystem::path dataDirectory()
{
    const auto &asked = chosen();
    return asked.profile.empty() ? local() : asked.profile / "data";
}

std::filesystem::path stateDirectory()
{

    const auto &asked = chosen();
    return asked.profile.empty() ? local() / "logs" : asked.profile / "logs";
}

std::filesystem::path cacheDirectory()
{
    const auto &asked = chosen();
    return asked.profile.empty() ? local() / "cache" : asked.profile / "cache";
}

std::filesystem::path recordingsDirectory()
{

    const auto &asked = chosen();
    return asked.recordings.empty() ? dataDirectory() / "recordings" : asked.recordings;
}

std::string forNarrowFileApi(const std::filesystem::path &file)
{
    const auto wide = file.wstring();

    if (std::all_of(wide.begin(), wide.end(), [](wchar_t letter) { return letter < 0x80; })) {
        return file.string();
    }

    if (const DWORD needed = ::GetShortPathNameW(wide.c_str(), nullptr, 0); needed > 0) {
        std::wstring shortened(needed, L'\0');
        const DWORD wrote = ::GetShortPathNameW(wide.c_str(), shortened.data(), needed);
        if (wrote > 0 && wrote < needed) {
            shortened.resize(wrote);
            if (std::all_of(shortened.begin(), shortened.end(),
                            [](wchar_t letter) { return letter < 0x80; })) {
                return std::filesystem::path{shortened}.string();
            }
        }
    }

    return file.string();
}

bool restrictToOwner(const std::filesystem::path &file)
{
    PSID user = nullptr;
    const auto token = currentUser(user);
    if (token == nullptr) {
        return false;
    }

    EXPLICIT_ACCESS_W entry{};
    entry.grfAccessPermissions = GENERIC_READ | GENERIC_WRITE | DELETE;
    entry.grfAccessMode = SET_ACCESS;
    entry.grfInheritance = NO_INHERITANCE;
    entry.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    entry.Trustee.TrusteeType = TRUSTEE_IS_USER;
    entry.Trustee.ptstrName = static_cast<LPWCH>(user);

    PACL raw = nullptr;
    if (::SetEntriesInAclW(1, &entry, nullptr, &raw) != ERROR_SUCCESS) {
        return false;
    }
    const std::unique_ptr<ACL, decltype(&::LocalFree)> list{raw, &::LocalFree};

    auto name = file.wstring();

    const DWORD wrote =
        ::SetNamedSecurityInfoW(name.data(), SE_FILE_OBJECT,
                                DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                                nullptr, nullptr, list.get(), nullptr);

    return wrote == ERROR_SUCCESS;
}

}
