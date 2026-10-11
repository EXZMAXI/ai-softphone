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

#include <softphone/core/providers.hpp>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace softphone::core {

enum class KnobKind {

    Number,

    Decimal,

    Flag,

    Text,
};

struct ProviderKnob {

    std::string name;

    KnobKind kind{KnobKind::Number};

    std::string fallback;

    bool sent{true};

    bool optional{true};
};

[[nodiscard]] std::vector<ProviderKnob> knobsFor(ProviderKind kind, std::string_view type);

[[nodiscard]] const ProviderKnob *knobIn(const std::vector<ProviderKnob> &knobs,
                                         std::string_view name);

[[nodiscard]] std::string knobValue(const std::vector<std::pair<std::string, std::string>> &params,
                                    const std::vector<ProviderKnob> &knobs, std::string_view name);

[[nodiscard]] bool isKnob(const std::vector<ProviderKnob> &knobs, std::string_view name);

}
