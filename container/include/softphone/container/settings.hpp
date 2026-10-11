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

#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::container {

enum class SettingsStatus {
    Ok,

    NotFound,
    Unreadable,
    Malformed,

    UnsupportedSchema,
    NotWritable,
};

[[nodiscard]] std::string_view toString(SettingsStatus status) noexcept;

using FileProtector = std::function<bool(const std::filesystem::path &)>;

[[nodiscard]] FileProtector noProtection();

inline constexpr std::string_view kContainerSection = "@container";

class Settings {
public:

    static constexpr int kSchema = 3;

    Settings(std::filesystem::path file, FileProtector protect);

    Settings(const Settings &) = delete;
    Settings &operator=(const Settings &) = delete;
    Settings(Settings &&) = delete;
    Settings &operator=(Settings &&) = delete;
    ~Settings() = default;

    void adoptOlderDocumentsAs(std::string owner);

    [[nodiscard]] SettingsStatus load();

    [[nodiscard]] SettingsStatus save() const;

    [[nodiscard]] std::string section(std::string_view owner) const;

    void setSection(std::string_view owner, std::string text);

    void removeSection(std::string_view owner);

    [[nodiscard]] std::vector<std::string> owners() const;

    [[nodiscard]] std::vector<std::string> disabledPlugins() const;

    void setDisabledPlugins(const std::vector<std::string> &ids);

    [[nodiscard]] const std::filesystem::path &file() const noexcept
    {
        return file_;
    }

private:
    [[nodiscard]] SettingsStatus readInto(std::string_view text,
                                          std::vector<std::pair<std::string, std::string>> &out);

    std::filesystem::path file_;
    FileProtector protect_;
    std::string adoptAs_;

    mutable std::mutex mutex_;
    std::vector<std::pair<std::string, std::string>> sections_;
};

class SettingsSection {
public:
    SettingsSection(Settings &settings, std::string owner) noexcept
        : settings_{settings}, owner_{std::move(owner)}
    {}

    [[nodiscard]] std::string read() const
    {
        return settings_.section(owner_);
    }

    void write(std::string text)
    {
        settings_.setSection(owner_, std::move(text));
    }

    [[nodiscard]] const std::string &owner() const noexcept
    {
        return owner_;
    }

private:
    Settings &settings_;
    std::string owner_;
};

}
