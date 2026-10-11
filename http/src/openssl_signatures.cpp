// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/http/openssl_signatures.hpp>

#include <openssl/evp.h>

#include <array>
#include <cstdint>
#include <fstream>
#include <memory>
#include <optional>
#include <vector>

namespace softphone::http {
namespace {

constexpr std::size_t kKeyBytes = 32;
constexpr std::size_t kSignatureBytes = 64;

[[nodiscard]] std::optional<std::vector<unsigned char>> fromBase64(std::string_view text)
{
    constexpr std::string_view kAlphabet{
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};

    std::vector<unsigned char> bytes;
    bytes.reserve(text.size() * 3 / 4);

    std::uint32_t group = 0;
    int filled = 0;

    for (const char character : text) {
        if (character == '=' || character == '\n' || character == '\r') {
            continue;
        }

        const auto index = kAlphabet.find(character);
        if (index == std::string_view::npos) {
            return std::nullopt;
        }

        group = (group << 6) | static_cast<std::uint32_t>(index);
        filled += 6;

        if (filled >= 8) {
            filled -= 8;
            bytes.push_back(static_cast<unsigned char>((group >> filled) & 0xFFU));
        }
    }

    return bytes;
}

struct ContextDeleter {
    void operator()(EVP_MD_CTX *context) const noexcept
    {
        EVP_MD_CTX_free(context);
    }
};

struct KeyDeleter {
    void operator()(EVP_PKEY *key) const noexcept
    {
        EVP_PKEY_free(key);
    }
};

}

OpenSslSignatures::OpenSslSignatures() = default;

OpenSslSignatures::~OpenSslSignatures() = default;

bool OpenSslSignatures::verify(std::string_view document, std::string_view signature,
                               std::string_view publicKey)
{
    const auto key = fromBase64(publicKey);
    const auto seal = fromBase64(signature);

    if (!key || key->size() != kKeyBytes) {
        return false;
    }
    if (!seal || seal->size() != kSignatureBytes) {
        return false;
    }

    const std::unique_ptr<EVP_PKEY, KeyDeleter> parsed{
        EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, key->data(), key->size())};
    if (!parsed) {
        return false;
    }

    const std::unique_ptr<EVP_MD_CTX, ContextDeleter> context{EVP_MD_CTX_new()};
    if (!context) {
        return false;
    }

    if (EVP_DigestVerifyInit(context.get(), nullptr, nullptr, nullptr, parsed.get()) != 1) {
        return false;
    }

    return EVP_DigestVerify(context.get(), seal->data(), seal->size(),
                            reinterpret_cast<const unsigned char *>(document.data()),
                            document.size()) == 1;
}

std::string OpenSslSignatures::digest(const std::filesystem::path &file)
{
    std::ifstream input{file, std::ios::binary};
    if (!input) {
        return {};
    }

    const std::unique_ptr<EVP_MD_CTX, ContextDeleter> context{EVP_MD_CTX_new()};
    if (!context || EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1) {
        return {};
    }

    std::vector<char> chunk(1024 * 1024);
    while (input) {
        input.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        const auto read = static_cast<std::size_t>(input.gcount());
        if (read == 0) {
            break;
        }
        if (EVP_DigestUpdate(context.get(), chunk.data(), read) != 1) {
            return {};
        }
    }

    if (input.bad()) {
        return {};
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> raw{};
    unsigned int length = 0;
    if (EVP_DigestFinal_ex(context.get(), raw.data(), &length) != 1) {
        return {};
    }

    constexpr std::string_view kHex{"0123456789abcdef"};
    std::string hexadecimal;
    hexadecimal.reserve(static_cast<std::size_t>(length) * 2);
    for (unsigned int index = 0; index < length; ++index) {
        hexadecimal.push_back(kHex[raw[index] >> 4U]);
        hexadecimal.push_back(kHex[raw[index] & 0x0FU]);
    }
    return hexadecimal;
}

}
