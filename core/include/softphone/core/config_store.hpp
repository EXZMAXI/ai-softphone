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

#include <softphone/core/settings.hpp>

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::core {

[[nodiscard]] std::filesystem::path defaultConfigDirectory();

[[nodiscard]] std::filesystem::path defaultSettingsFile();

enum class ConfigStoreStatus {
    Ok,

    NotFound,
    Unreadable,
    Malformed,

    UnsupportedSchema,
    NotWritable,
};

[[nodiscard]] std::string_view toString(ConfigStoreStatus status) noexcept;

struct ConfigLayout {

    std::string application;

    std::string telephone;
    std::string buttons;
    std::string recording;
    std::string transcription;
    std::string processing;

    std::string integration;
    std::string diagnostics;

    friend bool operator==(const ConfigLayout &, const ConfigLayout &) = default;
};

[[nodiscard]] const ConfigLayout &defaultConfigLayout();

inline constexpr std::string_view kLegacySettingsSection = "monolith";

struct ConfigDocument {
    std::function<std::string(std::string_view section)> read;

    std::function<std::vector<std::string>()> sections;
    std::function<void(std::string_view section, const std::string &text)> write;
    std::function<void(std::string_view section)> erase;
    std::function<bool()> flush;
    std::filesystem::path shownAs;

    [[nodiscard]] bool bound() const noexcept
    {
        return static_cast<bool>(read) && static_cast<bool>(write) && static_cast<bool>(erase) &&
               static_cast<bool>(flush);
    }
};

class ConfigStore {
public:
    explicit ConfigStore(std::filesystem::path directory);

    explicit ConfigStore(ConfigDocument document, ConfigLayout layout = defaultConfigLayout());

    [[nodiscard]] static ConfigStoreStatus read(std::string_view text, const ConfigLayout &layout,
                                                StoredConfiguration &out);

    [[nodiscard]] static std::string write(const StoredConfiguration &configuration);

    [[nodiscard]] static std::vector<std::pair<std::string, std::string>>
    writeSections(const StoredConfiguration &configuration, const ConfigLayout &layout);

    [[nodiscard]] static std::vector<std::string> groupsWithNoHome();

    [[nodiscard]] static ConfigStoreStatus
    readSections(const std::function<std::string(std::string_view)> &section,
                 const ConfigLayout &layout, StoredConfiguration &out,
                 const std::vector<std::string> &held = {});

    [[nodiscard]] const std::filesystem::path &directory() const noexcept
    {
        return directory_;
    }

    [[nodiscard]] std::filesystem::path filePath() const;

    [[nodiscard]] ConfigStoreStatus load(StoredConfiguration &out) const;

    [[nodiscard]] ConfigStoreStatus save(const StoredConfiguration &configuration) const;

private:
    std::filesystem::path directory_;
    ConfigDocument document_;
    ConfigLayout layout_;
};

}
