// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "link_handler_xdg.hpp"

#include <softphone/core/logging.hpp>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>

namespace softphone::ui {
namespace {

[[nodiscard]] const char *environmentValue(const char *name)
{
    const auto *value = std::getenv(name);
    return (value != nullptr && *value != '\0') ? value : nullptr;
}

[[nodiscard]] std::string carried(const std::filesystem::path &file)
{
    std::ifstream reading{file};
    if (!reading) {
        return {};
    }
    std::ostringstream held;
    held << reading.rdbuf();
    return held.str();
}

}

std::filesystem::path defaultMimeAppsFile()
{
    if (const auto *configHome = environmentValue("XDG_CONFIG_HOME")) {
        return std::filesystem::path{configHome} / "mimeapps.list";
    }
    if (const auto *home = environmentValue("HOME")) {
        return std::filesystem::path{home} / ".config" / "mimeapps.list";
    }
    return std::filesystem::path{".config"} / "mimeapps.list";
}

XdgLinkHandler::XdgLinkHandler(std::filesystem::path file) : file_{std::move(file)} {}

LinkHandlerState XdgLinkHandler::state() const
{
    const auto text = carried(file_);

    LinkHandlerState answer;
    answer.possible = true;
    answer.preferred = mimeAppsNameUs(text);
    if (!answer.preferred) {
        auto held = mimeAppsHandlerOfTel(text);

        answer.heldBy = held == kDesktopEntryName ? std::string{} : std::move(held);
    }
    return answer;
}

bool XdgLinkHandler::claim()
{
    const auto wanted = mimeAppsPointedAtUs(carried(file_));

    std::error_code failed;
    std::filesystem::create_directories(file_.parent_path(), failed);

    const auto beside = file_.parent_path() / (file_.filename().string() + ".softphone-new");
    {
        std::ofstream writing{beside, std::ios::binary | std::ios::trunc};
        if (!writing) {
            core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                         "cannot write " + beside.string() +
                                             "; the desktop was not told about tel: links");
            return false;
        }
        writing << wanted;
    }

    std::filesystem::rename(beside, file_, failed);
    if (failed) {
        std::filesystem::remove(beside, failed);
        core::Logger::instance().log(core::LogLevel::Warning, "ui",
                                     "cannot replace " + file_.string() +
                                         "; the desktop was not told about tel: links");
        return false;
    }

    core::Logger::instance().log(core::LogLevel::Info, "ui",
                                 "this phone is now what the desktop opens tel: links with");
    return true;
}

std::unique_ptr<ILinkHandler> makeLinkHandler()
{
    return std::make_unique<XdgLinkHandler>();
}

}
