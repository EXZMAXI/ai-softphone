// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/secrets/credential_manager_store.hpp>

#include <windows.h>

#include <wincred.h>

#include <memory>
#include <vector>

namespace softphone::secrets {
namespace {

constexpr auto kTargetPrefix = "AI Softphone: ";

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

[[nodiscard]] SecretStatus statusFromLastError()
{
    switch (::GetLastError()) {
    case ERROR_NOT_FOUND:
        return SecretStatus::NotFound;

    case ERROR_ACCESS_DENIED:
    case ERROR_PRIVILEGE_NOT_HELD:
        return SecretStatus::Denied;

    default:
        return SecretStatus::Failed;
    }
}

struct CredentialDeleter {
    void operator()(CREDENTIALW *credential) const noexcept
    {
        if (credential != nullptr) {
            ::CredFree(credential);
        }
    }
};

using CredentialHandle = std::unique_ptr<CREDENTIALW, CredentialDeleter>;

}

std::string credentialTargetFor(const std::string &accountId)
{
    return kTargetPrefix + accountId;
}

bool CredentialManagerStore::available() const
{

    return true;
}

SecretStatus CredentialManagerStore::store(const std::string &accountId,
                                           const std::string &password)
{
    auto target = widen(credentialTargetFor(accountId));
    if (target.empty()) {
        return SecretStatus::Failed;
    }

    std::vector<BYTE> blob(password.begin(), password.end());

    auto userName = widen(accountId);

    CREDENTIALW credential{};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = target.data();
    credential.CredentialBlobSize = static_cast<DWORD>(blob.size());
    credential.CredentialBlob = blob.empty() ? nullptr : blob.data();
    credential.UserName = userName.empty() ? nullptr : userName.data();

    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;

    if (::CredWriteW(&credential, 0) == FALSE) {
        return statusFromLastError();
    }

    return SecretStatus::Ok;
}

SecretStatus CredentialManagerStore::fetch(const std::string &accountId,
                                           std::string &password) const
{
    const auto target = widen(credentialTargetFor(accountId));
    if (target.empty()) {
        return SecretStatus::Failed;
    }

    CREDENTIALW *raw = nullptr;
    if (::CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &raw) == FALSE) {
        return statusFromLastError();
    }

    const CredentialHandle credential{raw};

    password.assign(reinterpret_cast<const char *>(credential->CredentialBlob),
                    credential->CredentialBlobSize);
    return SecretStatus::Ok;
}

SecretStatus CredentialManagerStore::erase(const std::string &accountId)
{
    const auto target = widen(credentialTargetFor(accountId));
    if (target.empty()) {
        return SecretStatus::Failed;
    }

    if (::CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0) == FALSE) {
        return statusFromLastError();
    }

    return SecretStatus::Ok;
}

std::optional<std::string> CredentialManagerStore::password(const std::string &accountId) const
{
    std::string found;
    if (fetch(accountId, found) != SecretStatus::Ok) {
        return std::nullopt;
    }
    return found;
}

std::unique_ptr<ICredentialStore> makePlatformCredentialStore()
{
    return std::make_unique<CredentialManagerStore>();
}

}
