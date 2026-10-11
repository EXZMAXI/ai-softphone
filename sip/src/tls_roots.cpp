// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "tls_roots.hpp"

#include <softphone/sip/pjsip_backend.hpp>

#include <pjsua2.hpp>

#if PJ_SSL_SOCK_IMP == PJ_SSL_SOCK_IMP_OPENSSL && defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#elif PJ_SSL_SOCK_IMP == PJ_SSL_SOCK_IMP_OPENSSL
#include <openssl/x509.h>
#endif

#include <array>
#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace softphone::sip {
namespace {

#if PJ_SSL_SOCK_IMP == PJ_SSL_SOCK_IMP_OPENSSL && defined(__APPLE__)

[[nodiscard]] std::string anchorsAsPem()
{
    CFArrayRef anchors = nullptr;
    if (SecTrustCopyAnchorCertificates(&anchors) != errSecSuccess || anchors == nullptr) {
        return {};
    }

    CFDataRef exported = nullptr;
    const auto status =
        SecItemExport(anchors, kSecFormatPEMSequence, kSecItemPemArmour, nullptr, &exported);
    CFRelease(anchors);
    if (status != errSecSuccess || exported == nullptr) {
        return {};
    }

    std::string pem{reinterpret_cast<const char *>(CFDataGetBytePtr(exported)),
                    static_cast<std::size_t>(CFDataGetLength(exported))};
    CFRelease(exported);
    return pem;
}

#elif PJ_SSL_SOCK_IMP == PJ_SSL_SOCK_IMP_OPENSSL

[[nodiscard]] bool isAFile(const std::string &path)
{
    std::error_code ignored;
    return !path.empty() && std::filesystem::is_regular_file(path, ignored);
}

[[nodiscard]] std::string systemBundle()
{
    if (const char *named = std::getenv(X509_get_default_cert_file_env());
        named != nullptr && isAFile(named)) {
        return named;
    }

    if (const std::string compiledIn = X509_get_default_cert_file(); isAFile(compiledIn)) {
        return compiledIn;
    }

    constexpr std::array kBundles{
        "/etc/ssl/certs/ca-certificates.crt",
        "/etc/pki/tls/certs/ca-bundle.crt",
        "/etc/ssl/ca-bundle.pem",
        "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",
        "/etc/ssl/cert.pem",
    };
    for (const auto *const bundle : kBundles) {
        if (isAFile(bundle)) {
            return bundle;
        }
    }
    return {};
}

#endif

}

std::string_view tlsImplementation() noexcept
{
#if PJ_HAS_SSL_SOCK == 0
    return "none";
#elif PJ_SSL_SOCK_IMP == PJ_SSL_SOCK_IMP_OPENSSL
    return "openssl";
#elif PJ_SSL_SOCK_IMP == PJ_SSL_SOCK_IMP_SCHANNEL
    return "schannel";
#elif PJ_SSL_SOCK_IMP == PJ_SSL_SOCK_IMP_DARWIN
    return "darwin";
#elif PJ_SSL_SOCK_IMP == PJ_SSL_SOCK_IMP_APPLE
    return "apple";
#else
    return "another";
#endif
}

std::string trustTheMachine(pj::TlsConfig &config, const std::string &authority)
{
    if (!authority.empty()) {
#if PJ_SSL_SOCK_IMP == PJ_SSL_SOCK_IMP_SCHANNEL

        (void)config;
        return "trusting what the operating system trusts; the authority at " + authority +
               " cannot be given to schannel, which reads no certificate files";
#else
        config.CaListFile = authority;
        return "trusting only " + authority;
#endif
    }

#if PJ_SSL_SOCK_IMP == PJ_SSL_SOCK_IMP_OPENSSL && defined(__APPLE__)
    config.CaBuf = anchorsAsPem();
    return config.CaBuf.empty() ? "the system's anchors could not be read; every certificate "
                                  "will be refused"
                                : "trusting the system's anchors";
#elif PJ_SSL_SOCK_IMP == PJ_SSL_SOCK_IMP_OPENSSL
    config.CaListFile = systemBundle();
    return config.CaListFile.empty()
               ? "no certificate bundle on this machine; every certificate will be refused"
               : "trusting " + config.CaListFile;
#else
    return "trusting what the operating system trusts";
#endif
}

}
