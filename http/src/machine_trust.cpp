// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#if defined(_WIN32)
#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS
#endif

#include "machine_trust.hpp"

#include <httplib.h>

#if defined(_WIN32) && defined(CPPHTTPLIB_OPENSSL_SUPPORT)

#include <softphone/core/logging.hpp>

#include <windows.h>

#include <wincrypt.h>

#undef X509_NAME
#undef X509_EXTENSIONS
#undef PKCS7_SIGNER_INFO
#undef OCSP_REQUEST
#undef OCSP_RESPONSE

#include <openssl/pem.h>
#include <openssl/x509.h>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace softphone::http {
namespace {

struct Store {
    HCERTSTORE handle{nullptr};

    ~Store()
    {
        if (handle != nullptr) {
            CertCloseStore(handle, 0);
        }
    }
};

struct Chain {
    PCCERT_CHAIN_CONTEXT context{nullptr};

    ~Chain()
    {
        if (context != nullptr) {
            CertFreeCertificateChain(context);
        }
    }
};

struct Leaf {
    PCCERT_CONTEXT context{nullptr};

    ~Leaf()
    {
        if (context != nullptr) {
            CertFreeCertificateContext(context);
        }
    }
};

[[nodiscard]] std::string asHex(DWORD code)
{
    std::string digits(8, '0');
    for (auto place = digits.size(); place > 0; --place) {
        digits[place - 1] = "0123456789abcdef"[code & 0xF];
        code >>= 4;
    }
    return digits;
}

[[nodiscard]] std::wstring wide(const std::string &text)
{
    if (text.empty()) {
        return {};
    }
    const auto length =
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0) {
        return {};
    }
    std::wstring wider(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), wider.data(),
                        length);
    return wider;
}

[[nodiscard]] bool addTo(HCERTSTORE store, X509 *certificate, PCCERT_CONTEXT *kept)
{
    unsigned char *der = nullptr;
    const auto length = i2d_X509(certificate, &der);
    if (length <= 0 || der == nullptr) {
        return false;
    }

    const auto added =
        CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, der,
                                         static_cast<DWORD>(length), CERT_STORE_ADD_ALWAYS, kept);
    OPENSSL_free(der);
    return added != FALSE;
}

[[nodiscard]] bool windowsBelieves(X509 *peer, const STACK_OF(X509) * sent,
                                   const std::wstring &host)
{
    if (peer == nullptr) {
        core::Logger::instance().log(core::LogLevel::Debug, "http",
                                     "the far end sent no certificate at all");
        return false;
    }

    Store store;
    store.handle = CertOpenStore(CERT_STORE_PROV_MEMORY, X509_ASN_ENCODING, 0,
                                 CERT_STORE_CREATE_NEW_FLAG, nullptr);
    if (store.handle == nullptr) {
        core::Logger::instance().log(core::LogLevel::Debug, "http",
                                     "this machine would not open a store to verify in");
        return false;
    }

    if (sent != nullptr) {
        for (int index = 0; index < sk_X509_num(sent); ++index) {
            static_cast<void>(addTo(store.handle, sk_X509_value(sent, index), nullptr));
        }
    }

    Leaf leaf;
    if (!addTo(store.handle, peer, &leaf.context) || leaf.context == nullptr) {
        core::Logger::instance().log(core::LogLevel::Debug, "http",
                                     "the certificate the far end sent could not be read");
        return false;
    }

    auto usage = const_cast<LPSTR>(szOID_PKIX_KP_SERVER_AUTH);

    CERT_CHAIN_PARA parameters{};
    parameters.cbSize = sizeof(parameters);
    parameters.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
    parameters.RequestedUsage.Usage.cUsageIdentifier = 1;
    parameters.RequestedUsage.Usage.rgpszUsageIdentifier = &usage;

    parameters.dwUrlRetrievalTimeout = 5000;

    Chain chain;
    if (CertGetCertificateChain(nullptr, leaf.context, nullptr, store.handle, &parameters,
                                CERT_CHAIN_CACHE_END_CERT, nullptr, &chain.context) == FALSE ||
        chain.context == nullptr) {
        core::Logger::instance().log(core::LogLevel::Debug, "http",
                                     "the certificate chain could not be built");
        return false;
    }

    SSL_EXTRA_CERT_CHAIN_POLICY_PARA forTls{};
    forTls.cbSize = sizeof(forTls);
    forTls.dwAuthType = AUTHTYPE_SERVER;
    forTls.pwszServerName = const_cast<LPWSTR>(host.c_str());

    CERT_CHAIN_POLICY_PARA policy{};
    policy.cbSize = sizeof(policy);
    policy.pvExtraPolicyPara = &forTls;

    CERT_CHAIN_POLICY_STATUS answer{};
    answer.cbSize = sizeof(answer);

    if (CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, chain.context, &policy, &answer) ==
        FALSE) {
        core::Logger::instance().log(core::LogLevel::Debug, "http",
                                     "this machine would not judge the certificate chain");
        return false;
    }

    if (answer.dwError != 0) {

        core::Logger::instance().log(core::LogLevel::Warning, "http",
                                     "the certificate was refused by this machine: error 0x" +
                                         asHex(answer.dwError));
        return false;
    }

    return true;
}

[[nodiscard]] bool windowsBelieves(SSL *ssl, const std::wstring &host)
{
    auto *peer = ssl == nullptr ? nullptr : SSL_get1_peer_certificate(ssl);
    const auto answer =
        windowsBelieves(peer, ssl == nullptr ? nullptr : SSL_get_peer_cert_chain(ssl), host);
    if (peer != nullptr) {
        X509_free(peer);
    }
    return answer;
}

[[nodiscard]] bool thisMachineBelieves(X509 *peer, const STACK_OF(X509) * sent,
                                       const std::string &host)
{
    return windowsBelieves(peer, sent, wide(host));
}

}

void trustTheMachine(httplib::Client &client, const std::string &host)
{
    client.set_server_certificate_verifier(
        [wanted = wide(host)](SSL *ssl) { return windowsBelieves(ssl, wanted); });

    client.enable_server_certificate_verification(true);
}

std::string trustStoreForWebSockets()
{
    static constexpr std::array<const wchar_t *, 4> kStores{L"ROOT", L"CA", L"ROOT", L"CA"};
    static constexpr std::array<DWORD, 4> kWhose{
        CERT_SYSTEM_STORE_LOCAL_MACHINE, CERT_SYSTEM_STORE_LOCAL_MACHINE,
        CERT_SYSTEM_STORE_CURRENT_USER, CERT_SYSTEM_STORE_CURRENT_USER};

    std::string bundle;
    for (std::size_t which = 0; which < kStores.size(); ++which) {
        const Store store{CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0,
                                        CERT_STORE_READONLY_FLAG | kWhose[which], kStores[which])};
        if (store.handle == nullptr) {
            continue;
        }

        PCCERT_CONTEXT found = nullptr;
        while ((found = CertEnumCertificatesInStore(store.handle, found)) != nullptr) {
            const auto *der = static_cast<const unsigned char *>(found->pbCertEncoded);
            X509 *certificate = d2i_X509(nullptr, &der, found->cbCertEncoded);
            if (certificate == nullptr) {
                continue;
            }

            BIO *text = BIO_new(BIO_s_mem());
            if (text != nullptr) {
                if (PEM_write_bio_X509(text, certificate) == 1) {
                    char *bytes = nullptr;
                    const auto length = BIO_get_mem_data(text, &bytes);
                    if (bytes != nullptr && length > 0) {
                        bundle.append(bytes, static_cast<std::size_t>(length));
                    }
                }
                BIO_free(text);
            }
            X509_free(certificate);
        }
    }

    if (bundle.empty()) {

        core::Logger::instance().log(core::LogLevel::Warning, "websocket",
                                     "this machine's certificate stores held nothing to trust");
        return "SYSTEM";
    }
    return bundle;
}

}

#elif defined(__APPLE__) && defined(CPPHTTPLIB_OPENSSL_SUPPORT)

#include <softphone/core/logging.hpp>

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace softphone::http {
namespace {

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

[[nodiscard]] Owned<SecCertificateRef> asSecurityCertificate(X509 *certificate)
{
    unsigned char *der = nullptr;
    const auto length = ::i2d_X509(certificate, &der);
    if (length <= 0 || der == nullptr) {
        return nullptr;
    }

    const Owned<CFDataRef> bytes{::CFDataCreate(nullptr, der, static_cast<CFIndex>(length))};
    ::OPENSSL_free(der);
    if (!bytes) {
        return nullptr;
    }

    return Owned<SecCertificateRef>{::SecCertificateCreateWithData(nullptr, bytes.get())};
}

[[nodiscard]] bool appleBelieves(X509 *peer, const STACK_OF(X509) * sent, const std::string &host)
{
    if (peer == nullptr) {
        core::Logger::instance().log(core::LogLevel::Debug, "http",
                                     "the far end sent no certificate at all");
        return false;
    }

    const Owned<CFMutableArrayRef> chain{
        ::CFArrayCreateMutable(nullptr, 0, &kCFTypeArrayCallBacks)};
    if (!chain) {
        return false;
    }

    {
        const auto wrapped = asSecurityCertificate(peer);
        if (!wrapped) {
            return false;
        }
        ::CFArrayAppendValue(chain.get(), wrapped.get());
    }

    if (sent != nullptr) {
        for (int at = 0; at < sk_X509_num(sent); ++at) {
            auto *certificate = sk_X509_value(sent, at);
            if (certificate == nullptr) {
                continue;
            }

            if (const auto wrapped = asSecurityCertificate(certificate)) {
                ::CFArrayAppendValue(chain.get(), wrapped.get());
            }
        }
    }

    if (::CFArrayGetCount(chain.get()) == 0) {
        return false;
    }

    const Owned<CFStringRef> wanted{
        ::CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8 *>(host.data()),
                                  static_cast<CFIndex>(host.size()), kCFStringEncodingUTF8, false)};
    if (!wanted) {
        return false;
    }

    const Owned<SecPolicyRef> policy{::SecPolicyCreateSSL(true, wanted.get())};
    if (!policy) {
        return false;
    }

    SecTrustRef raw = nullptr;
    if (::SecTrustCreateWithCertificates(chain.get(), policy.get(), &raw) != errSecSuccess ||
        raw == nullptr) {
        return false;
    }
    const Owned<SecTrustRef> trust{raw};

    CFErrorRef why = nullptr;
    if (::SecTrustEvaluateWithError(trust.get(), &why)) {
        return true;
    }

    const Owned<CFErrorRef> refusal{why};
    std::string said;
    if (refusal) {
        if (const Owned<CFStringRef> text{::CFErrorCopyDescription(refusal.get())}) {
            std::array<char, 256> buffer{};
            if (::CFStringGetCString(text.get(), buffer.data(), static_cast<CFIndex>(buffer.size()),
                                     kCFStringEncodingUTF8)) {
                said = buffer.data();
            }
        }
    }

    core::Logger::instance().log(core::LogLevel::Warning, "http",
                                 "the system would not verify " + host +
                                     (said.empty() ? std::string{} : ": " + said));
    return false;
}

[[nodiscard]] bool appleBelieves(SSL *ssl, const std::string &host)
{
    auto *peer = ssl == nullptr ? nullptr : ::SSL_get1_peer_certificate(ssl);
    const auto answer =
        appleBelieves(peer, ssl == nullptr ? nullptr : ::SSL_get_peer_cert_chain(ssl), host);
    if (peer != nullptr) {
        ::X509_free(peer);
    }
    return answer;
}

[[nodiscard]] bool thisMachineBelieves(X509 *peer, const STACK_OF(X509) * sent,
                                       const std::string &host)
{
    return appleBelieves(peer, sent, host);
}

}

void trustTheMachine(httplib::Client &client, const std::string &host)
{
    client.set_server_certificate_verifier(
        [wanted = host](SSL *ssl) { return appleBelieves(ssl, wanted); });

    client.enable_server_certificate_verification(true);
}

std::string trustStoreForWebSockets()
{
    CFArrayRef raw = nullptr;
    if (::SecTrustCopyAnchorCertificates(&raw) != errSecSuccess || raw == nullptr) {
        core::Logger::instance().log(core::LogLevel::Warning, "websocket",
                                     "this machine would not hand over its trust anchors");
        return "SYSTEM";
    }
    const Owned<CFArrayRef> anchors{raw};

    std::string bundle;
    const auto many = ::CFArrayGetCount(anchors.get());
    for (CFIndex which = 0; which < many; ++which) {
        auto *const anchor = static_cast<SecCertificateRef>(
            const_cast<void *>(::CFArrayGetValueAtIndex(anchors.get(), which)));
        if (anchor == nullptr) {
            continue;
        }

        const Owned<CFDataRef> der{::SecCertificateCopyData(anchor)};
        if (!der) {
            continue;
        }

        const auto *bytes = ::CFDataGetBytePtr(der.get());
        auto length = ::CFDataGetLength(der.get());
        X509 *certificate = ::d2i_X509(nullptr, &bytes, length);
        if (certificate == nullptr) {
            continue;
        }

        BIO *text = ::BIO_new(::BIO_s_mem());
        if (text != nullptr) {
            if (::PEM_write_bio_X509(text, certificate) == 1) {
                char *written = nullptr;
                const auto wrote = ::BIO_get_mem_data(text, &written);
                if (written != nullptr && wrote > 0) {
                    bundle.append(written, static_cast<std::size_t>(wrote));
                }
            }
            ::BIO_free(text);
        }
        ::X509_free(certificate);
    }

    if (bundle.empty()) {
        return "SYSTEM";
    }
    return bundle;
}

}

#else

namespace softphone::http {

void trustTheMachine(httplib::Client &, const std::string &)
{

}

std::string trustStoreForWebSockets()
{

    return "SYSTEM";
}

}

#endif

#if (defined(_WIN32) || defined(__APPLE__)) && defined(CPPHTTPLIB_OPENSSL_SUPPORT)

#include <softphone/core/logging.hpp>

#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>

namespace softphone::http {
namespace {

void forgetTheHost(void *, void *held, CRYPTO_EX_DATA *, int, long, void *)
{
    delete static_cast<std::string *>(held);
}

[[nodiscard]] int whereTheHostIs()
{
    static const int index = CRYPTO_get_ex_new_index(CRYPTO_EX_INDEX_SSL_CTX, 0, nullptr, nullptr,
                                                     nullptr, &forgetTheHost);
    return index;
}

int askThisMachine(X509_STORE_CTX *store, void *)
{
    auto *ssl =
        static_cast<SSL *>(X509_STORE_CTX_get_ex_data(store, SSL_get_ex_data_X509_STORE_CTX_idx()));
    const auto *host = ssl == nullptr ? nullptr
                                      : static_cast<const std::string *>(SSL_CTX_get_ex_data(
                                            SSL_get_SSL_CTX(ssl), whereTheHostIs()));
    if (host == nullptr) {
        core::Logger::instance().log(
            core::LogLevel::Debug, "http",
            "this handshake carried no name to hold a certificate to, so it is refused");
        X509_STORE_CTX_set_error(store, X509_V_ERR_UNSPECIFIED);
        return 0;
    }

    if (thisMachineBelieves(X509_STORE_CTX_get0_cert(store), X509_STORE_CTX_get0_untrusted(store),
                            *host)) {
        X509_STORE_CTX_set_error(store, X509_V_OK);
        return 1;
    }

    X509_STORE_CTX_set_error(store, X509_V_ERR_CERT_UNTRUSTED);
    return 0;
}

}

std::string trustTheMachine(SSL_CTX *context, const std::string &host)
{
    if (context == nullptr) {
        return "there is no TLS context to tell about this machine's certificates";
    }

    auto *kept = new std::string{host};
    if (SSL_CTX_set_ex_data(context, whereTheHostIs(), kept) != 1) {
        delete kept;
        return "this machine could not be asked about certificates, so every one will be refused";
    }
    SSL_CTX_set_cert_verify_callback(context, &askThisMachine, nullptr);

    return "asking this machine about every certificate";
}

}

#else

namespace softphone::http {

std::string trustTheMachine(SSL_CTX *context, const std::string &)
{
    if (context == nullptr) {
        return "there is no TLS context to tell about this machine's certificates";
    }

    if (SSL_CTX_set_default_verify_paths(context) != 1) {
        return "this machine's certificate store could not be read, so every certificate will be "
               "refused";
    }
    return "trusting what this machine's own OpenSSL trusts";
}

}

#endif
