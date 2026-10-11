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

#include <softphone/capture/system_capture.hpp>

#include <cstdint>
#include <mutex>
#include <vector>

namespace softphone::capture {

class FakeCapture final : public ISystemCapture {
public:
    CaptureError start(const CaptureRequest &request, FrameSink sink) override;
    void stop() override;
    [[nodiscard]] CaptureStatus status() const override;

    void refuseWith(CaptureError error);

    void deliver(Side side, std::span<const std::int16_t> samples,
                 std::chrono::steady_clock::time_point at);

    void deliverSilence(Side side, std::size_t count, std::chrono::steady_clock::time_point at);

    void resolveTo(std::string output, std::string input);

    void yieldsTheSoundCard(Yields must);

    [[nodiscard]] Yields mustYieldTheSoundCard() const override;

    void holdOffWhatTheMachinePlays(bool held) override;

    [[nodiscard]] bool whatTheMachinePlaysIsHeldOff() const;

private:
    mutable std::mutex mutex_;
    FrameSink sink_;
    CaptureRequest request_;
    CaptureStatus status_;
    CaptureError refusal_{CaptureError::None};
    std::vector<std::int16_t> silence_;
    Yields yields_{Yields::Nothing};
    bool farHeldOff_{false};
};

}
