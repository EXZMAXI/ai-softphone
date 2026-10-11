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

#include <softphone/core/call.hpp>

#include <optional>
#include <string>
#include <vector>

namespace softphone::core {

struct AssertedIdentity {

    std::vector<std::string> pAssertedIdentity;

    std::vector<std::string> remotePartyId;

    std::vector<std::string> privacy;
};

[[nodiscard]] std::optional<SipAddress> identityToShow(const AssertedIdentity &asserted,
                                                       CallDirection direction);

}
