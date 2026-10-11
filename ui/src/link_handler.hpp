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
#include <memory>
#include <string>
#include <string_view>

namespace softphone::ui {

struct LinkHandlerState {

    bool possible{false};

    bool preferred{false};

    std::string heldBy;

    friend bool operator==(const LinkHandlerState &, const LinkHandlerState &) = default;
};

inline constexpr std::string_view kDesktopEntryName = "ai-softphone.desktop";

[[nodiscard]] bool mimeAppsNameUs(std::string_view carried,
                                  std::string_view entryName = kDesktopEntryName);

[[nodiscard]] std::string mimeAppsHandlerOfTel(std::string_view carried);

[[nodiscard]] std::string mimeAppsPointedAtUs(std::string_view carried,
                                              std::string_view entryName = kDesktopEntryName);

[[nodiscard]] std::string urlOpenCommand(const std::filesystem::path &binary);

inline constexpr std::string_view kWindowsProgId = "AiSoftphone.Call";

class ILinkHandler {
public:
    ILinkHandler() = default;
    ILinkHandler(const ILinkHandler &) = delete;
    ILinkHandler &operator=(const ILinkHandler &) = delete;
    ILinkHandler(ILinkHandler &&) = delete;
    ILinkHandler &operator=(ILinkHandler &&) = delete;
    virtual ~ILinkHandler() = default;

    [[nodiscard]] virtual LinkHandlerState state() const = 0;

    [[nodiscard]] virtual bool claim() = 0;
};

[[nodiscard]] std::unique_ptr<ILinkHandler> makeLinkHandler();

}
