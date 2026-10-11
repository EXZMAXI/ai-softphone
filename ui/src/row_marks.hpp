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

#include <QString>

#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace softphone::ui {

[[nodiscard]] QString providerNameOf(const std::string &providerId, const std::string &providerType,
                                     const std::vector<std::pair<QString, QString>> &names);

[[nodiscard]] QString markOn(const QString &name, const QString &mark);

[[nodiscard]] QString whenMark(std::chrono::system_clock::time_point at);

[[nodiscard]] std::vector<QString> toldApart(std::vector<QString> base,
                                             const std::vector<std::vector<QString>> &candidates);

}
