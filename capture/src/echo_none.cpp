// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/capture/echo.hpp>

namespace softphone::capture {
namespace {

class NoCanceller final : public IEchoCanceller {
public:
    NoCanceller(unsigned samplesPerFrame) : frame_{samplesPerFrame} {}

    void cancel(std::span<std::int16_t>, std::span<const std::int16_t>) override {}

    [[nodiscard]] unsigned samplesPerFrame() const override
    {
        return frame_;
    }

    [[nodiscard]] const char *name() const override
    {
        return "none";
    }

private:
    unsigned frame_;
};

}

std::unique_ptr<IEchoCanceller> makeEchoCanceller(unsigned, unsigned samplesPerFrame)
{
    return std::make_unique<NoCanceller>(samplesPerFrame);
}

}
