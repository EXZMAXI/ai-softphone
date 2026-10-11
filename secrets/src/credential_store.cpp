// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/secrets/credential_store.hpp>

#include <softphone/secrets/sealed_file_store.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>
#include <softphone/core/telemetry.hpp>

#include <utility>

namespace softphone::secrets {
namespace {

constexpr auto kArea = "secrets";

}

std::string_view toString(SecretStatus status) noexcept
{
    switch (status) {
    case SecretStatus::Ok:
        return "ok";
    case SecretStatus::NotFound:
        return "not-found";
    case SecretStatus::ServiceUnavailable:
        return "service-unavailable";
    case SecretStatus::Denied:
        return "denied";
    case SecretStatus::Failed:
        return "failed";
    }
    return "unknown";
}

ChosenStore makeCredentialStore()
{
    ChosenStore chosen;
    chosen.store = makePlatformCredentialStore();

    if (chosen.store && chosen.store->available()) {
        chosen.kind = StoreKind::System;
        return chosen;
    }

    core::Logger::instance().log(
        core::LogLevel::Info, kArea,
        "no system keyring answered; keeping passwords in a sealed file of our own");

    auto sealed = makeSealedFileStore(core::stateDirectory() / "credentials.sealed",
                                      sealedBinding(core::installationId()));
    if (!sealed) {

        chosen.kind = StoreKind::None;
        return chosen;
    }

    chosen.store = std::move(sealed);
    chosen.kind = StoreKind::Sealed;
    return chosen;
}

}
