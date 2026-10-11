// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/container/settings.hpp>

#include <softphone/container/manifest.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <system_error>
#include <utility>

namespace softphone::container {
namespace {

template<typename Sections>
[[nodiscard]] auto findOwner(Sections &sections, std::string_view owner)
{
    return std::ranges::find_if(sections,
                                [owner](const auto &entry) { return entry.first == owner; });
}

}

std::string_view toString(SettingsStatus status) noexcept
{
    switch (status) {
    case SettingsStatus::Ok:
        return "ok";
    case SettingsStatus::NotFound:
        return "not found";
    case SettingsStatus::Unreadable:
        return "unreadable";
    case SettingsStatus::Malformed:
        return "malformed";
    case SettingsStatus::UnsupportedSchema:
        return "unsupported schema";
    case SettingsStatus::NotWritable:
        return "not writable";
    }
    return "unknown";
}

FileProtector noProtection()
{
    return [](const std::filesystem::path &) { return true; };
}

Settings::Settings(std::filesystem::path file, FileProtector protect)
    : file_{std::move(file)}, protect_{std::move(protect)}
{}

void Settings::adoptOlderDocumentsAs(std::string owner)
{
    const std::lock_guard lock{mutex_};
    adoptAs_ = std::move(owner);
}

SettingsStatus Settings::readInto(std::string_view text,
                                  std::vector<std::pair<std::string, std::string>> &out)
{
    const auto document = nlohmann::json::parse(text, nullptr, false);
    if (document.is_discarded() || !document.is_object()) {
        return SettingsStatus::Malformed;
    }

    const auto schema = document.value("schemaVersion", 0);
    if (schema <= 0) {
        return SettingsStatus::Malformed;
    }

    const auto sections = document.find("sections");
    if (sections == document.end()) {

        if (adoptAs_.empty()) {
            return SettingsStatus::Malformed;
        }
        out.emplace_back(adoptAs_, std::string{text});
        return SettingsStatus::Ok;
    }

    if (schema > kSchema) {
        return SettingsStatus::UnsupportedSchema;
    }
    if (!sections->is_object()) {
        return SettingsStatus::Malformed;
    }

    for (const auto &[owner, content] : sections->items()) {
        if (owner.empty()) {
            return SettingsStatus::Malformed;
        }
        out.emplace_back(owner, content.dump(2));
    }
    return SettingsStatus::Ok;
}

SettingsStatus Settings::load()
{
    std::error_code error;
    if (!std::filesystem::exists(file_, error) || error) {
        return SettingsStatus::NotFound;
    }

    std::ifstream stream{file_};
    if (!stream) {
        return SettingsStatus::Unreadable;
    }
    const std::string text{std::istreambuf_iterator<char>{stream},
                           std::istreambuf_iterator<char>{}};
    if (!stream.eof() && stream.fail()) {
        return SettingsStatus::Unreadable;
    }

    std::vector<std::pair<std::string, std::string>> read;
    const std::lock_guard lock{mutex_};
    if (const auto status = readInto(text, read); status != SettingsStatus::Ok) {
        return status;
    }

    sections_ = std::move(read);
    return SettingsStatus::Ok;
}

SettingsStatus Settings::save() const
{
    std::error_code error;
    std::filesystem::create_directories(file_.parent_path(), error);
    if (error) {
        return SettingsStatus::NotWritable;
    }

    nlohmann::json document;
    document["schemaVersion"] = kSchema;
    auto sections = nlohmann::json::object();
    {
        const std::lock_guard lock{mutex_};
        for (const auto &[owner, content] : sections_) {

            auto parsed = nlohmann::json::parse(content, nullptr, false);
            sections[owner] = parsed.is_discarded() ? nlohmann::json{content} : std::move(parsed);
        }
    }
    document["sections"] = std::move(sections);

    const auto temporary = std::filesystem::path{file_}.concat(".new");
    {
        std::ofstream stream{temporary, std::ios::trunc};
        if (!stream) {
            return SettingsStatus::NotWritable;
        }
        stream << document.dump(2) << '\n';
        if (!stream) {
            return SettingsStatus::NotWritable;
        }
    }

    if (!protect_ || !protect_(temporary)) {
        std::filesystem::remove(temporary, error);
        return SettingsStatus::NotWritable;
    }

    std::filesystem::rename(temporary, file_, error);
    if (error) {
        std::filesystem::remove(temporary, error);
        return SettingsStatus::NotWritable;
    }
    return SettingsStatus::Ok;
}

std::string Settings::section(std::string_view owner) const
{
    const std::lock_guard lock{mutex_};
    const auto found = findOwner(sections_, owner);
    return found == sections_.end() ? std::string{} : found->second;
}

void Settings::setSection(std::string_view owner, std::string text)
{
    if (owner.empty()) {
        return;
    }

    const std::lock_guard lock{mutex_};
    const auto found = findOwner(sections_, owner);
    if (found != sections_.end()) {
        found->second = std::move(text);
        return;
    }
    sections_.emplace_back(std::string{owner}, std::move(text));
}

void Settings::removeSection(std::string_view owner)
{
    const std::lock_guard lock{mutex_};
    const auto found = findOwner(sections_, owner);
    if (found != sections_.end()) {
        sections_.erase(found);
    }
}

std::vector<std::string> Settings::owners() const
{
    const std::lock_guard lock{mutex_};

    std::vector<std::string> names;
    names.reserve(sections_.size());
    for (const auto &[owner, content] : sections_) {
        names.push_back(owner);
    }
    return names;
}

std::vector<std::string> Settings::disabledPlugins() const
{

    const auto text = section(kContainerSection);
    const auto document = nlohmann::json::parse(text, nullptr, false);
    if (document.is_discarded() || !document.is_object()) {
        return {};
    }

    const auto off = document.find("disabled");
    if (off == document.end() || !off->is_array()) {
        return {};
    }

    std::vector<std::string> ids;
    for (const auto &entry : *off) {
        if (!entry.is_string()) {
            continue;
        }
        auto id = entry.get<std::string>();

        if (isValidPluginId(id)) {
            ids.push_back(std::move(id));
        }
    }
    return ids;
}

void Settings::setDisabledPlugins(const std::vector<std::string> &ids)
{
    auto off = nlohmann::json::array();
    for (const auto &id : ids) {
        if (isValidPluginId(id)) {
            off.push_back(id);
        }
    }

    nlohmann::json document;
    document["disabled"] = std::move(off);
    setSection(kContainerSection, document.dump(2));
}

}
