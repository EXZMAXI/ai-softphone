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

#include <cstdint>
#include <memory>
#include <span>

namespace softphone::capture {

class IEchoCanceller {
public:
    virtual ~IEchoCanceller() = default;

    IEchoCanceller() = default;
    IEchoCanceller(const IEchoCanceller &) = delete;
    IEchoCanceller &operator=(const IEchoCanceller &) = delete;
    IEchoCanceller(IEchoCanceller &&) = delete;
    IEchoCanceller &operator=(IEchoCanceller &&) = delete;

    virtual void cancel(std::span<std::int16_t> nearSide,
                        std::span<const std::int16_t> farSide) = 0;

    [[nodiscard]] virtual unsigned samplesPerFrame() const = 0;

    [[nodiscard]] virtual const char *name() const = 0;
};

[[nodiscard]] std::unique_ptr<IEchoCanceller> makeEchoCanceller(unsigned sampleRate,
                                                                unsigned samplesPerFrame);

}
