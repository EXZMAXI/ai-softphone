// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/secrets/keychain_store.hpp>

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

#include <memory>
#include <type_traits>

namespace softphone::secrets {
namespace {

constexpr auto kService = "AI Softphone";

struct CoreFoundationDeleter {
    void operator()(CFTypeRef value) const noexcept
    {
        if (value != nullptr) {
            ::CFRelease(value);
        }
    }
};

template<typename T>
using Owned = std::unique_ptr<std::remove_pointer_t<T>, CoreFoundationDeleter>;

[[nodiscard]] Owned<CFDataRef> asData(const std::string &text)
{
    return Owned<CFDataRef>{::CFDataCreate(nullptr, reinterpret_cast<const UInt8 *>(text.data()),
                                           static_cast<CFIndex>(text.size()))};
}

[[nodiscard]] Owned<CFStringRef> asString(const std::string &text)
{
    return Owned<CFStringRef>{
        ::CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8 *>(text.data()),
                                  static_cast<CFIndex>(text.size()), kCFStringEncodingUTF8, false)};
}

[[nodiscard]] SecretStatus statusFrom(OSStatus status)
{
    switch (status) {
    case errSecSuccess:
        return SecretStatus::Ok;

    case errSecItemNotFound:
        return SecretStatus::NotFound;

    case errSecAuthFailed:
    case errSecInteractionNotAllowed:
    case errSecUserCanceled:
        return SecretStatus::Denied;

    default:
        return SecretStatus::Failed;
    }
}

[[nodiscard]] Owned<CFMutableDictionaryRef> queryFor(const std::string &accountId)
{
    const auto service = asString(kService);
    const auto account = asString(accountId);
    if (!service || !account) {
        return nullptr;
    }

    Owned<CFMutableDictionaryRef> query{::CFDictionaryCreateMutable(
        nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)};
    if (!query) {
        return nullptr;
    }

    ::CFDictionarySetValue(query.get(), kSecClass, kSecClassGenericPassword);
    ::CFDictionarySetValue(query.get(), kSecAttrService, service.get());
    ::CFDictionarySetValue(query.get(), kSecAttrAccount, account.get());
    return query;
}

}

std::string keychainServiceName()
{
    return kService;
}

bool KeychainStore::available() const
{

    return true;
}

SecretStatus KeychainStore::store(const std::string &accountId, const std::string &password)
{
    auto query = queryFor(accountId);
    const auto secret = asData(password);
    if (!query || !secret) {
        return SecretStatus::Failed;
    }

    Owned<CFMutableDictionaryRef> changes{::CFDictionaryCreateMutable(
        nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)};
    if (!changes) {
        return SecretStatus::Failed;
    }
    ::CFDictionarySetValue(changes.get(), kSecValueData, secret.get());

    const auto updated = ::SecItemUpdate(query.get(), changes.get());
    if (updated == errSecSuccess) {
        return SecretStatus::Ok;
    }
    if (updated != errSecItemNotFound) {
        return statusFrom(updated);
    }

    ::CFDictionarySetValue(query.get(), kSecValueData, secret.get());

    ::CFDictionarySetValue(query.get(), kSecAttrAccessible, kSecAttrAccessibleWhenUnlocked);
    ::CFDictionarySetValue(query.get(), kSecAttrSynchronizable, kCFBooleanFalse);

    return statusFrom(::SecItemAdd(query.get(), nullptr));
}

SecretStatus KeychainStore::fetch(const std::string &accountId, std::string &password) const
{
    auto query = queryFor(accountId);
    if (!query) {
        return SecretStatus::Failed;
    }

    ::CFDictionarySetValue(query.get(), kSecReturnData, kCFBooleanTrue);
    ::CFDictionarySetValue(query.get(), kSecMatchLimit, kSecMatchLimitOne);

    CFTypeRef raw = nullptr;
    const auto status = ::SecItemCopyMatching(query.get(), &raw);
    if (status != errSecSuccess) {
        return statusFrom(status);
    }

    const Owned<CFTypeRef> found{raw};
    if (raw == nullptr || ::CFGetTypeID(raw) != ::CFDataGetTypeID()) {
        return SecretStatus::Failed;
    }

    const auto *data = static_cast<CFDataRef>(raw);
    password.assign(reinterpret_cast<const char *>(::CFDataGetBytePtr(data)),
                    static_cast<std::size_t>(::CFDataGetLength(data)));
    return SecretStatus::Ok;
}

SecretStatus KeychainStore::erase(const std::string &accountId)
{
    const auto query = queryFor(accountId);
    if (!query) {
        return SecretStatus::Failed;
    }

    return statusFrom(::SecItemDelete(query.get()));
}

std::optional<std::string> KeychainStore::password(const std::string &accountId) const
{
    std::string found;
    if (fetch(accountId, found) != SecretStatus::Ok) {
        return std::nullopt;
    }
    return found;
}

std::unique_ptr<ICredentialStore> makePlatformCredentialStore()
{
    return std::make_unique<KeychainStore>();
}

}
