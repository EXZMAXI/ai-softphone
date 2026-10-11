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

#include <softphone/core/playback.hpp>
#include <softphone/core/recording_ring.hpp>
#include <softphone/media/time_stretch.hpp>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace softphone::media {

class Mp3Stream final : public core::IPlaybackSource {
public:

    Mp3Stream(std::filesystem::path file, unsigned rate);

    Mp3Stream(const Mp3Stream &) = delete;
    Mp3Stream &operator=(const Mp3Stream &) = delete;
    Mp3Stream(Mp3Stream &&) = delete;
    Mp3Stream &operator=(Mp3Stream &&) = delete;
    ~Mp3Stream() override;

    [[nodiscard]] bool start();

    void stop();

    [[nodiscard]] std::uint64_t durationMs() const noexcept
    {
        return durationMs_;
    }

    [[nodiscard]] std::uint64_t positionMs() const noexcept;

    void seek(std::uint64_t positionMs);

    void setSpeed(double speed);

    void setChannels(bool nearSide, bool farSide);

    [[nodiscard]] unsigned sampleRate() const noexcept override
    {
        return rate_;
    }

    [[nodiscard]] std::size_t pull(std::span<std::int16_t> out) noexcept override;

    [[nodiscard]] bool finished() const noexcept override
    {
        return finished_.load(std::memory_order_acquire) && ring_.waiting() == 0;
    }

private:
    void run();

    bool fill();

    void deliver(bool everything);

    std::filesystem::path file_;
    unsigned rate_;

    struct Reader;
    std::unique_ptr<Reader> reader_;

    core::RecordingRing ring_;

    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable wake_;
    bool stopping_{false};

    double cursor_{0.0};

    std::atomic<std::uint64_t> played_{0};
    std::atomic<bool> finished_{false};
    std::atomic<double> speed_{1.0};
    std::atomic<bool> nearSide_{true};
    std::atomic<bool> farSide_{true};

    std::uint64_t durationMs_{0};
    unsigned sourceRate_{0};
    unsigned channels_{0};

    std::vector<std::int16_t> pending_;

    std::vector<std::int16_t> mono_;
    std::vector<std::int16_t> frame_;

    std::vector<std::int16_t> spill_;
    std::size_t held_{0};

    TimeStretch stretch_;
};

}
