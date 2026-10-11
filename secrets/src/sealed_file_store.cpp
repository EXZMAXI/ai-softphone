// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/secrets/sealed_file_store.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace softphone::secrets {
namespace {

constexpr auto kArea = "secrets";

constexpr std::string_view kDomain = "ai-softphone sealed credentials v1";

constexpr std::string_view kHeader = "ai-softphone sealed 1";

using Bytes = std::vector<unsigned char>;

[[nodiscard]] std::string asHex(const unsigned char *from, std::size_t many)
{
    static constexpr std::string_view kDigits = "0123456789abcdef";
    std::string out;
    out.reserve(many * 2);
    for (std::size_t at = 0; at < many; ++at) {
        out.push_back(kDigits[from[at] >> 4U]);
        out.push_back(kDigits[from[at] & 0x0FU]);
    }
    return out;
}

[[nodiscard]] std::string asHex(const Bytes &from)
{
    return asHex(from.data(), from.size());
}

[[nodiscard]] std::string asHex(std::string_view from)
{
    return asHex(reinterpret_cast<const unsigned char *>(from.data()), from.size());
}

[[nodiscard]] bool fromHex(std::string_view text, Bytes &out)
{
    if (text.size() % 2 != 0) {
        return false;
    }
    const auto digit = [](char one) -> int {
        if (one >= '0' && one <= '9') {
            return one - '0';
        }
        if (one >= 'a' && one <= 'f') {
            return one - 'a' + 10;
        }
        return -1;
    };

    out.clear();
    out.reserve(text.size() / 2);
    for (std::size_t at = 0; at < text.size(); at += 2) {
        const int high = digit(text[at]);
        const int low = digit(text[at + 1]);
        if (high < 0 || low < 0) {
            return false;
        }
        out.push_back(static_cast<unsigned char>((high << 4) | low));
    }
    return true;
}

[[nodiscard]] Bytes sha256(std::string_view of)
{
    Bytes out(SHA256_DIGEST_LENGTH);
    SHA256(reinterpret_cast<const unsigned char *>(of.data()), of.size(), out.data());
    return out;
}

struct Sealed {
    Bytes nonce;
    Bytes body;
};

[[nodiscard]] bool seal(const Bytes &key, const std::string &name, const std::string &plain,
                        Sealed &out)
{
    out.nonce.assign(12, 0);
    if (RAND_bytes(out.nonce.data(), static_cast<int>(out.nonce.size())) != 1) {
        return false;
    }

    const std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> context{
        EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free};
    if (!context) {
        return false;
    }

    if (EVP_EncryptInit_ex(context.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_IVLEN,
                            static_cast<int>(out.nonce.size()), nullptr) != 1 ||
        EVP_EncryptInit_ex(context.get(), nullptr, nullptr, key.data(), out.nonce.data()) != 1) {
        return false;
    }

    int moved = 0;
    if (!name.empty() && EVP_EncryptUpdate(context.get(), nullptr, &moved,
                                           reinterpret_cast<const unsigned char *>(name.data()),
                                           static_cast<int>(name.size())) != 1) {
        return false;
    }

    out.body.assign(plain.size() + 16, 0);
    if (EVP_EncryptUpdate(context.get(), out.body.data(), &moved,
                          reinterpret_cast<const unsigned char *>(plain.data()),
                          static_cast<int>(plain.size())) != 1) {
        return false;
    }
    int written = moved;

    if (EVP_EncryptFinal_ex(context.get(), out.body.data() + written, &moved) != 1) {
        return false;
    }
    written += moved;

    if (EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_GET_TAG, 16, out.body.data() + written) !=
        1) {
        return false;
    }
    out.body.resize(static_cast<std::size_t>(written) + 16);
    return true;
}

[[nodiscard]] bool unseal(const Bytes &key, const std::string &name, const Sealed &from,
                          std::string &plain)
{
    if (from.body.size() < 16 || from.nonce.size() != 12) {
        return false;
    }

    const std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> context{
        EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free};
    if (!context) {
        return false;
    }

    if (EVP_DecryptInit_ex(context.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_IVLEN,
                            static_cast<int>(from.nonce.size()), nullptr) != 1 ||
        EVP_DecryptInit_ex(context.get(), nullptr, nullptr, key.data(), from.nonce.data()) != 1) {
        return false;
    }

    int moved = 0;
    if (!name.empty() && EVP_DecryptUpdate(context.get(), nullptr, &moved,
                                           reinterpret_cast<const unsigned char *>(name.data()),
                                           static_cast<int>(name.size())) != 1) {
        return false;
    }

    const auto bodySize = from.body.size() - 16;
    Bytes out(bodySize == 0 ? 1 : bodySize, 0);
    if (EVP_DecryptUpdate(context.get(), out.data(), &moved, from.body.data(),
                          static_cast<int>(bodySize)) != 1) {
        return false;
    }
    int written = moved;

    if (EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_TAG, 16,
                            const_cast<unsigned char *>(from.body.data() + bodySize)) != 1) {
        return false;
    }

    if (EVP_DecryptFinal_ex(context.get(), out.data() + written, &moved) != 1) {
        return false;
    }
    written += moved;

    plain.assign(reinterpret_cast<const char *>(out.data()), static_cast<std::size_t>(written));
    return true;
}

[[nodiscard]] std::string machineBinding()
{
    for (const auto *candidate : {"/etc/machine-id", "/var/lib/dbus/machine-id"}) {
        std::ifstream input{candidate};
        if (!input) {
            continue;
        }
        std::string line;
        std::getline(input, line);

        const auto from = line.find_first_not_of(" \t\r\n");
        const auto to = line.find_last_not_of(" \t\r\n");
        if (from != std::string::npos) {
            return line.substr(from, to - from + 1);
        }
    }
    return {};
}

class SealedFileStore final : public ICredentialStore {
public:
    SealedFileStore(std::filesystem::path file, const std::string &binding)
        : file_{std::move(file)}, key_{sha256(std::string{kDomain} + '\n' + binding)},
          fingerprint_{asHex(sha256(binding))}
    {
        load();
    }

    [[nodiscard]] bool available() const override
    {
        const std::lock_guard guard{mutex_};
        return writable_;
    }

    [[nodiscard]] SecretStatus store(const std::string &accountId,
                                     const std::string &password) override
    {
        const std::lock_guard guard{mutex_};

        Sealed sealed;
        if (!seal(key_, accountId, password, sealed)) {
            return SecretStatus::Failed;
        }

        entries_[accountId] = std::move(sealed);
        return save();
    }

    [[nodiscard]] SecretStatus erase(const std::string &accountId) override
    {
        const std::lock_guard guard{mutex_};

        if (entries_.erase(accountId) == 0) {
            return SecretStatus::NotFound;
        }
        return save();
    }

    [[nodiscard]] SecretStatus fetch(const std::string &accountId,
                                     std::string &password) const override
    {
        const std::lock_guard guard{mutex_};

        if (strangersFile_) {
            return SecretStatus::Denied;
        }

        const auto found = entries_.find(accountId);
        if (found == entries_.end()) {
            return SecretStatus::NotFound;
        }
        if (!unseal(key_, accountId, found->second, password)) {
            return SecretStatus::Denied;
        }
        return SecretStatus::Ok;
    }

    [[nodiscard]] std::optional<std::string> password(const std::string &accountId) const override
    {
        std::string found;
        if (fetch(accountId, found) != SecretStatus::Ok) {
            return std::nullopt;
        }
        return found;
    }

private:
    void load()
    {
        std::ifstream input{file_};
        if (!input) {

            return;
        }

        std::string line;
        if (!std::getline(input, line) || line != kHeader) {
            core::Logger::instance().log(core::LogLevel::Warning, kArea,
                                         "the sealed credential file is not one of ours");
            strangersFile_ = true;
            return;
        }

        while (std::getline(input, line)) {
            std::istringstream fields{line};
            std::string what;
            fields >> what;

            if (what == "binding") {
                std::string said;
                fields >> said;
                if (said != fingerprint_) {

                    core::Logger::instance().log(
                        core::LogLevel::Warning, kArea,
                        "the sealed credential file belongs to another installation");
                    strangersFile_ = true;
                    return;
                }
                continue;
            }

            if (what != "entry") {
                continue;
            }

            std::string name;
            std::string nonce;
            std::string body;
            fields >> name >> nonce >> body;

            Bytes nameBytes;
            Sealed sealed;
            if (!fromHex(name, nameBytes) || !fromHex(nonce, sealed.nonce) ||
                !fromHex(body, sealed.body)) {
                continue;
            }
            entries_.emplace(std::string{nameBytes.begin(), nameBytes.end()}, std::move(sealed));
        }
    }

    [[nodiscard]] SecretStatus save()
    {
        std::error_code ignored;
        std::filesystem::create_directories(file_.parent_path(), ignored);

        auto temporary = file_;
        temporary += ".writing";

        {
            std::ofstream output{temporary, std::ios::trunc};
            if (!output) {
                writable_ = false;
                return SecretStatus::Failed;
            }

            output << kHeader << '\n';
            output << "binding " << fingerprint_ << '\n';
            for (const auto &[name, sealed] : entries_) {
                output << "entry " << asHex(name) << ' ' << asHex(sealed.nonce) << ' '
                       << asHex(sealed.body) << '\n';
            }
            if (!output) {
                writable_ = false;
                return SecretStatus::Failed;
            }
        }

        if (!core::restrictToOwner(temporary)) {
            std::filesystem::remove(temporary, ignored);
            writable_ = false;
            return SecretStatus::Failed;
        }

        std::error_code moved;
        std::filesystem::rename(temporary, file_, moved);
        if (moved) {
            std::filesystem::remove(temporary, ignored);
            writable_ = false;
            return SecretStatus::Failed;
        }

        writable_ = true;
        strangersFile_ = false;
        return SecretStatus::Ok;
    }

    std::filesystem::path file_;
    Bytes key_;
    std::string fingerprint_;

    mutable std::mutex mutex_;
    std::map<std::string, Sealed> entries_;

    bool writable_{true};

    bool strangersFile_{false};
};

}

std::string sealedBinding(std::string_view installationId)
{
    return std::string{installationId} + '\n' + machineBinding();
}

std::unique_ptr<ICredentialStore> makeSealedFileStore(std::filesystem::path file,
                                                      std::string binding)
{
    return std::make_unique<SealedFileStore>(std::move(file), binding);
}

}
