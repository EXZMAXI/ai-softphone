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

#include <softphone/container/manifest.hpp>
#include <softphone/container/plugin.hpp>
#include <softphone/core/contacts.hpp>

#include <string_view>

namespace softphone::plugins::directory {

class DirectoryPlugin final : public container::IPlugin {
public:
    bool initialize(container::PluginContext &context, std::string &problem) override;

private:
    core::IContactStore *book_{nullptr};
};

[[nodiscard]] container::Manifest directoryManifest();

inline constexpr std::string_view kOwner = "directory";

inline constexpr std::string_view kContactStore = "directory.contacts";

[[nodiscard]] inline std::string_view collectionsDdl()
{
    return R"(
        CREATE TABLE IF NOT EXISTS contacts (
            id              TEXT    PRIMARY KEY,
            name            TEXT    NOT NULL,
            number          TEXT    NOT NULL,
            number_key      TEXT    NOT NULL,
            account_id      TEXT    NOT NULL DEFAULT '',
            account_address TEXT    NOT NULL DEFAULT '',
            created_at      INTEGER NOT NULL,
            updated_at      INTEGER NOT NULL
        );

        -- **The indexes belong with the table**, and leaving one behind has
        -- been paid for once: a `CREATE INDEX` left in the product's schema
        -- names a table that is no longer there, and a store that will not open
        -- is unavailable to everybody rather than to one plugin.
        CREATE INDEX IF NOT EXISTS contacts_by_number ON contacts(number_key);
        CREATE INDEX IF NOT EXISTS contacts_by_name ON contacts(name);
    )";
}

inline constexpr int kCollectionsVersion = 1;

}
