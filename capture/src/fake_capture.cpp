// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/capture/fake_capture.hpp>

#include <algorithm>

namespace softphone::capture {

CaptureError FakeCapture::start(const CaptureRequest &request, FrameSink sink)
{
    const std::lock_guard guard{mutex_};
    if (refusal_ != CaptureError::None) {
        return refusal_;
    }
    request_ = request;
    sink_ = std::move(sink);
    status_.running = true;
    status_.nearFrames = 0;
    status_.farFrames = 0;
    status_.dropped = 0;
    if (status_.outputDevice.empty()) {

        status_.outputDevice = request.outputDevice.empty() ? "fake-output" : request.outputDevice;
        status_.inputDevice = request.inputDevice.empty() ? "fake-input" : request.inputDevice;
    }
    return CaptureError::None;
}

void FakeCapture::stop()
{
    const std::lock_guard guard{mutex_};
    sink_ = nullptr;
    status_.running = false;
}

CaptureStatus FakeCapture::status() const
{
    const std::lock_guard guard{mutex_};
    return status_;
}

void FakeCapture::refuseWith(CaptureError error)
{
    const std::lock_guard guard{mutex_};
    refusal_ = error;
}

void FakeCapture::resolveTo(std::string output, std::string input)
{
    const std::lock_guard guard{mutex_};
    status_.outputDevice = std::move(output);
    status_.inputDevice = std::move(input);
}

void FakeCapture::deliver(Side side, std::span<const std::int16_t> samples,
                          std::chrono::steady_clock::time_point at)
{
    FrameSink sink;
    {
        const std::lock_guard guard{mutex_};
        if (!status_.running || !sink_) {
            ++status_.dropped;
            return;
        }
        if (samples.size() > request_.frameSamples) {

            ++status_.dropped;
            return;
        }
        (side == Side::Near ? status_.nearFrames : status_.farFrames)++;
        sink = sink_;
    }
    sink(side, samples, at);
}

void FakeCapture::deliverSilence(Side side, std::size_t count,
                                 std::chrono::steady_clock::time_point at)
{
    if (silence_.size() < count) {
        silence_.assign(count, 0);
    }
    deliver(side, std::span<const std::int16_t>{silence_.data(), count}, at);
}

void FakeCapture::yieldsTheSoundCard(Yields must)
{
    const std::lock_guard guard{mutex_};
    yields_ = must;
}

ISystemCapture::Yields FakeCapture::mustYieldTheSoundCard() const
{
    const std::lock_guard guard{mutex_};
    return yields_;
}

void FakeCapture::holdOffWhatTheMachinePlays(bool held)
{
    const std::lock_guard guard{mutex_};
    farHeldOff_ = held;
}

bool FakeCapture::whatTheMachinePlaysIsHeldOff() const
{
    const std::lock_guard guard{mutex_};
    return farHeldOff_;
}

}
