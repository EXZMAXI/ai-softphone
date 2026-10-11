// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/capture/devices.hpp>

namespace softphone::capture {
namespace {

[[nodiscard]] int doing(const Device &device) noexcept
{
    const bool audible = device.peak > kAudible;

    const int activity = device.held ? (audible ? 3 : 2) : (audible ? 1 : 0);

    const int role = audible ? 0 : (device.communications ? 2 : (device.console ? 1 : 0));

    return activity * 4 + role;
}

[[nodiscard]] std::size_t bestOf(std::span<const Device> devices)
{
    std::size_t best = kNoDevice;
    int bestRank = -1;
    float bestPeak = -1.0F;

    for (std::size_t at = 0; at < devices.size(); ++at) {
        const int rank = doing(devices[at]);

        if (rank > bestRank || (rank == bestRank && devices[at].peak > bestPeak)) {
            best = at;
            bestRank = rank;
            bestPeak = devices[at].peak;
        }
    }

    return best;
}

}

std::size_t theOneCarryingSound(std::span<const Device> outputs)
{
    return bestOf(outputs);
}

std::size_t theOneBeingListenedTo(std::span<const Device> inputs)
{

    return bestOf(inputs);
}

}
