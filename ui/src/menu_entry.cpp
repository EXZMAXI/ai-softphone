// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "menu_entry.hpp"

#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>

#include <charconv>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>
#include <vector>

namespace softphone::ui {
namespace {

constexpr auto kEntryName = "ai-softphone.desktop";

constexpr auto kIconStem = "ai-softphone";

constexpr auto kIconRoot = "icons/hicolor";

[[nodiscard]] const char *environmentValue(const char *name)
{
    const auto *value = std::getenv(name);
    return (value != nullptr && *value != '\0') ? value : nullptr;
}

[[nodiscard]] std::string readWholeFile(const std::filesystem::path &path)
{
    std::ifstream file{path, std::ios::binary};
    if (!file) {
        return {};
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

}

RunningImage runningImage()
{

    const auto *image = environmentValue("APPIMAGE");
    const auto *mount = environmentValue("APPDIR");
    if (image == nullptr || mount == nullptr) {
        return {};
    }
    return RunningImage{std::filesystem::path{image}, std::filesystem::path{mount}};
}

std::filesystem::path defaultDataHome()
{
    if (const auto *dataHome = environmentValue("XDG_DATA_HOME")) {
        return std::filesystem::path{dataHome};
    }
    if (const auto *home = environmentValue("HOME")) {
        return std::filesystem::path{home} / ".local" / "share";
    }
    return std::filesystem::path{".local"} / "share";
}

std::string escapedForTheFile(std::string_view text)
{
    std::string escaped;
    escaped.reserve(text.size());
    for (const char character : text) {
        if (character == '\\') {
            escaped += "\\\\";
        } else {
            escaped += character;
        }
    }
    return escaped;
}

std::string quotedForExec(std::string_view path)
{

    std::string quoted;
    quoted.reserve(path.size() + 2);
    quoted += '"';
    for (const char character : path) {
        if (character == '"' || character == '`' || character == '$' || character == '\\') {
            quoted += "\\\\";
        }
        quoted += character;
    }
    quoted += '"';
    return quoted;
}

std::string menuEntryText(std::string_view carried, const std::filesystem::path &image,
                          const std::filesystem::path &icon)
{
    const auto path = image.string();

    std::vector<std::string> lines;
    bool sawExec = false;
    bool sawIcon = false;

    std::istringstream reading{std::string{carried}};
    for (std::string line; std::getline(reading, line);) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.rfind("Exec=", 0) == 0) {
            sawExec = true;

            lines.push_back("Exec=" + quotedForExec(path) + " %u");

            lines.push_back("TryExec=" + escapedForTheFile(path));
            continue;
        }

        if (line.rfind("TryExec=", 0) == 0) {
            continue;
        }

        if (line.rfind("Icon=", 0) == 0 && !icon.empty()) {

            sawIcon = true;
            lines.push_back("Icon=" + escapedForTheFile(core::asUtf8(icon)));
            continue;
        }

        lines.push_back(std::move(line));
    }

    if (!sawExec) {

        return {};
    }

    if (!icon.empty() && !sawIcon) {
        lines.push_back("Icon=" + escapedForTheFile(core::asUtf8(icon)));
    }

    std::string text;
    for (const auto &line : lines) {
        text += line;
        text += '\n';
    }
    return text;
}

XdgMenuEntry::XdgMenuEntry(RunningImage image, std::filesystem::path dataHome)
    : image_{std::move(image)}, dataHome_{std::move(dataHome)}
{}

std::filesystem::path XdgMenuEntry::entryPath() const
{
    return dataHome_ / "applications" / kEntryName;
}

bool XdgMenuEntry::present() const
{
    std::error_code error;
    return std::filesystem::exists(entryPath(), error) && !error;
}

namespace {

[[nodiscard]] unsigned iconSizeOf(const std::filesystem::path &file)
{
    const auto directory = file.parent_path().parent_path().filename().string();
    unsigned size = 0;
    const auto *first = directory.data();
    const auto *last = directory.data() + directory.size();
    if (std::from_chars(first, last, size).ec != std::errc{}) {
        return 0;
    }
    return size;
}

}

std::filesystem::path XdgMenuEntry::copyTheIcons() const
{
    const auto from = image_.mount / "usr" / "share" / kIconRoot;
    const auto into = dataHome_ / kIconRoot;

    std::error_code error;
    if (!std::filesystem::is_directory(from, error)) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "the image carries no icons at " + from.string());
        return {};
    }

    std::filesystem::path best;
    unsigned biggest = 0;
    bool copiedAnything = false;
    for (const auto &entry : std::filesystem::recursive_directory_iterator{from, error}) {
        if (!entry.is_regular_file() || entry.path().stem() != kIconStem) {
            continue;
        }

        const auto relative = std::filesystem::relative(entry.path(), from, error);
        if (error) {
            continue;
        }

        const auto target = into / relative;
        std::filesystem::create_directories(target.parent_path(), error);
        if (error) {
            core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                         "cannot create " + target.parent_path().string() + ": " +
                                             error.message());
            return {};
        }

        std::filesystem::copy_file(entry.path(), target,
                                   std::filesystem::copy_options::overwrite_existing, error);
        if (error) {
            core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                         "cannot write " + target.string() + ": " +
                                             error.message());
            return {};
        }
        copiedAnything = true;

        if (const auto size = iconSizeOf(target); best.empty() || size > biggest) {
            best = target;
            biggest = size;
        }
    }

    return copiedAnything ? best : std::filesystem::path{};
}

bool XdgMenuEntry::add()
{
    if (!image_.valid()) {
        return false;
    }

    const auto icon = copyTheIcons();
    if (icon.empty()) {
        return false;
    }

    const auto carried =
        readWholeFile(image_.mount / "usr" / "share" / "applications" / kEntryName);
    const auto text = menuEntryText(carried, image_.image, icon);
    if (text.empty()) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "the image carries no desktop entry to put in the menu");
        return false;
    }

    std::error_code error;
    std::filesystem::create_directories(entryPath().parent_path(), error);
    if (error) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot create " + entryPath().parent_path().string() + ": " +
                                         error.message());
        return false;
    }

    if (readWholeFile(entryPath()) == text) {
        return true;
    }

    std::ofstream file{entryPath(), std::ios::trunc | std::ios::binary};
    if (!file) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot write " + entryPath().string());
        return false;
    }

    file << text;
    if (!file) {
        return false;
    }

    core::Logger::instance().log(core::LogLevel::Info, "ui",
                                 "the applications menu now starts " + image_.image.string());
    return true;
}

bool XdgMenuEntry::remove()
{
    bool removedEverything = true;

    std::error_code error;
    std::filesystem::remove(entryPath(), error);
    if (error) {
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot remove " + entryPath().string() + ": " +
                                         error.message());
        removedEverything = false;
    }

    const auto icons = dataHome_ / kIconRoot;
    if (std::filesystem::is_directory(icons, error)) {
        for (const auto &entry : std::filesystem::recursive_directory_iterator{icons, error}) {
            if (!entry.is_regular_file() || entry.path().stem() != kIconStem) {
                continue;
            }
            std::error_code removing;
            std::filesystem::remove(entry.path(), removing);
            if (removing) {
                removedEverything = false;
            }
        }
    }

    return removedEverything;
}

std::unique_ptr<IMenuEntry> makeMenuEntry()
{
    auto image = runningImage();
    if (!image.valid()) {
        return nullptr;
    }
    return std::make_unique<XdgMenuEntry>(std::move(image));
}

}
