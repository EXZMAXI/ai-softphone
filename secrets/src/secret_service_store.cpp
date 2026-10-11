// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/secrets/secret_service_store.hpp>

#include <libsecret/secret.h>

#include <memory>
#include <string>

namespace softphone::secrets {
namespace {

const SecretSchema *schema()
{
    static const SecretSchema kSchema{
        "com.bpmkit.softphone.Account",
        SECRET_SCHEMA_NONE,
        {
            {"account-id", SECRET_SCHEMA_ATTRIBUTE_STRING},
            {nullptr, SecretSchemaAttributeType{}},
        },
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    };
    return &kSchema;
}

[[nodiscard]] std::string labelFor(const std::string &accountId)
{
    return "AI Softphone — " + accountId;
}

[[nodiscard]] SecretStatus statusFrom(const GError *error)
{
    if (error == nullptr) {
        return SecretStatus::Ok;
    }

    if (error->domain == G_DBUS_ERROR && (error->code == G_DBUS_ERROR_SERVICE_UNKNOWN ||
                                          error->code == G_DBUS_ERROR_NAME_HAS_NO_OWNER)) {
        return SecretStatus::ServiceUnavailable;
    }
    if (error->domain == SECRET_ERROR && error->code == SECRET_ERROR_IS_LOCKED) {
        return SecretStatus::Denied;
    }

    if (error->domain == G_IO_ERROR) {
        return SecretStatus::ServiceUnavailable;
    }

    return SecretStatus::Failed;
}

}

SecretServiceStore::SecretServiceStore() = default;

SecretServiceStore::~SecretServiceStore() = default;

bool SecretServiceStore::available() const
{

    GError *error = nullptr;

    gchar *found = secret_password_lookup_sync(schema(), nullptr, &error, "account-id",
                                               "softphone-availability-probe", nullptr);

    if (found != nullptr) {
        secret_password_free(found);
        return true;
    }

    if (error != nullptr) {
        const auto status = statusFrom(error);
        g_error_free(error);
        return status != SecretStatus::ServiceUnavailable;
    }

    return true;
}

SecretStatus SecretServiceStore::store(const std::string &accountId, const std::string &password)
{
    GError *error = nullptr;

    const gboolean stored = secret_password_store_sync(
        schema(), SECRET_COLLECTION_DEFAULT, labelFor(accountId).c_str(), password.c_str(), nullptr,
        &error, "account-id", accountId.c_str(), nullptr);

    if (error != nullptr) {
        const auto status = statusFrom(error);
        g_error_free(error);
        return status;
    }

    return stored != FALSE ? SecretStatus::Ok : SecretStatus::Failed;
}

SecretStatus SecretServiceStore::fetch(const std::string &accountId, std::string &password) const
{
    GError *error = nullptr;

    gchar *found = secret_password_lookup_sync(schema(), nullptr, &error, "account-id",
                                               accountId.c_str(), nullptr);

    if (error != nullptr) {
        const auto status = statusFrom(error);
        g_error_free(error);
        return status;
    }

    if (found == nullptr) {
        return SecretStatus::NotFound;
    }

    password.assign(found);

    secret_password_free(found);

    return SecretStatus::Ok;
}

SecretStatus SecretServiceStore::erase(const std::string &accountId)
{
    GError *error = nullptr;

    const gboolean removed = secret_password_clear_sync(schema(), nullptr, &error, "account-id",
                                                        accountId.c_str(), nullptr);

    if (error != nullptr) {
        const auto status = statusFrom(error);
        g_error_free(error);
        return status;
    }

    return removed != FALSE ? SecretStatus::Ok : SecretStatus::NotFound;
}

std::optional<std::string> SecretServiceStore::password(const std::string &accountId) const
{
    std::string found;
    if (fetch(accountId, found) != SecretStatus::Ok) {
        return std::nullopt;
    }
    return found;
}

std::unique_ptr<ICredentialStore> makePlatformCredentialStore()
{
    return std::make_unique<SecretServiceStore>();
}

}
