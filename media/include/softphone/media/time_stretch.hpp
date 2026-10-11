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

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace softphone::media {

class TimeStretch {
public:
    explicit TimeStretch(unsigned sampleRate);

    void setSpeed(double speed);

    [[nodiscard]] double speed() const noexcept
    {
        return speed_;
    }

    void write(std::span<const std::int16_t> samples);

    [[nodiscard]] std::size_t read(std::span<std::int16_t> out);

    void finish();

    void reset();

    [[nodiscard]] std::size_t waiting() const noexcept
    {
        return made_.size() - taken_;
    }

private:

    void grind();

    void layDown(std::size_t from);

    unsigned rate_;
    double speed_{1.0};

    std::size_t window_{0};
    std::size_t hop_{0};

    std::size_t search_{0};

    std::vector<float> shape_;

    std::vector<std::int16_t> source_;

    std::size_t nominal_{0};

    std::size_t at_{0};
    bool first_{true};
    bool ending_{false};

    std::vector<std::int16_t> wanted_;

    std::vector<float> made_;
    std::size_t taken_{0};

    std::size_t open_{0};

    std::size_t settled_{0};
};

}
