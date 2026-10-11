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

#include <softphone/container/loading.hpp>
#include <softphone/shell/ui_registry.hpp>

#include <QtGlobal>

#include <string>
#include <string_view>

namespace softphone::shell {

[[nodiscard]] inline container::BuildKey thisUiBuild()
{
    auto key = container::thisBuild();
    key.qt = QT_VERSION_STR;
    return key;
}

#define SOFTPHONE_UI_BUILD_KEY SOFTPHONE_BUILD_KEY ";qt=" QT_VERSION_STR

struct UiEntry {

    int abi;

    const char *buildKey;

    const char *id;

    void (*contribute)(UiRegistry &);
};

inline constexpr int kUiEntryAbi = 1;

inline constexpr const char *kUiEntryPoint = "softphone_plugin_ui_entry";

using UiEntryPoint = const UiEntry *(*)();

[[nodiscard]] std::string uiLibraryFileName(std::string_view id);

}

#define SOFTPHONE_PLUGIN_UI(ID, FUNCTION)                                                          \
    extern "C" SOFTPHONE_PLUGIN_EXPORT const ::softphone::shell::UiEntry *                         \
    softphone_plugin_ui_entry()                                                                    \
    {                                                                                              \
        static const ::softphone::shell::UiEntry entry{::softphone::shell::kUiEntryAbi,            \
                                                       SOFTPHONE_UI_BUILD_KEY, ID, &FUNCTION};     \
        return &entry;                                                                             \
    }
