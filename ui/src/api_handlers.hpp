// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#pragma once

#include <softphone/core/api.hpp>
#include <softphone/core/call_controller.hpp>
#include <softphone/core/call_log.hpp>
#include <softphone/core/contacts.hpp>
#include <softphone/core/settings_controller.hpp>
#include <softphone/core/taxonomy.hpp>

#include <functional>
#include <optional>
#include <string>

namespace softphone::ui {

using StorageWork = std::function<std::string(core::IContactStore &, core::ICallLog &)>;

using EngineWork = std::function<std::string()>;

using TaxonomyWork = std::function<std::string(core::ITaxonomyStore &)>;

struct ApiDependencies {

    std::function<std::optional<std::string>(StorageWork)> askStorage;

    std::function<std::optional<std::string>(EngineWork)> askEngine;

    std::function<std::optional<std::string>(TaxonomyWork)> askTaxonomy;

    std::function<core::CallController &()> calls;
    std::function<const core::StoredConfiguration &()> configuration;
    std::function<std::string(const std::string &accountId)> registrationState;

    std::function<core::SettingsController *()> settings;

    std::function<void()> contactsChanged;
    std::function<void()> historyChanged;
};

void registerApiHandlers(core::ApiRouter &router, ApiDependencies dependencies);

}
