// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "plugin_directory.hpp"

#include <softphone/core/paths.hpp>

#include <QCoreApplication>
#include <QString>

#include <system_error>

namespace softphone::ui {

std::filesystem::path pluginDirectory()
{

    const auto here = core::fromUtf8(QCoreApplication::applicationDirPath().toStdString());

#if defined(__APPLE__)

    const auto inTheBundle = here / ".." / "Resources" / "ai-softphone";

    std::error_code missing;
    if (std::filesystem::is_directory(inTheBundle, missing)) {
        return inTheBundle;
    }
#endif

    const auto beside = here / ".." / "lib" / "ai-softphone" / "plugins";

    std::error_code ignored;
    if (std::filesystem::is_directory(beside, ignored)) {
        return beside;
    }

#if defined(SOFTPHONE_PLUGIN_BUILD_TREE)

    return std::filesystem::path{SOFTPHONE_PLUGIN_BUILD_TREE};
#else
    return beside;
#endif
}

}
