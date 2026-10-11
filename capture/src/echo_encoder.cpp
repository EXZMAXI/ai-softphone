// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/capture/echo_encoder.hpp>

#include <algorithm>
#include <utility>

namespace softphone::capture {

EchoCancellingEncoder::EchoCancellingEncoder(std::unique_ptr<core::IAudioEncoder> below,
                                             std::unique_ptr<IEchoCanceller> canceller)
    : below_{std::move(below)}, canceller_{std::move(canceller)}
{
    if (canceller_) {
        workNear_.resize(canceller_->samplesPerFrame());
        workFar_.resize(canceller_->samplesPerFrame());
    }
}

core::EncoderStatus EchoCancellingEncoder::write(std::span<const std::int16_t> nearSide,
                                                 std::span<const std::int16_t> farSide)
{
    if (!below_) {
        return core::EncoderStatus::Failed;
    }
    if (!canceller_ || canceller_->samplesPerFrame() == 0) {
        return below_->write(nearSide, farSide);
    }

    if (nearSide.size() != farSide.size()) {
        return below_->write(nearSide, farSide);
    }

    near_.insert(near_.end(), nearSide.begin(), nearSide.end());
    far_.insert(far_.end(), farSide.begin(), farSide.end());
    return drain(false);
}

core::EncoderStatus EchoCancellingEncoder::drain(bool everything)
{
    const auto frame = canceller_ ? canceller_->samplesPerFrame() : 0U;
    if (frame == 0) {
        return core::EncoderStatus::Ok;
    }

    std::size_t done = 0;
    while (near_.size() - done >= frame) {
        std::copy_n(near_.begin() + static_cast<std::ptrdiff_t>(done), frame, workNear_.begin());
        std::copy_n(far_.begin() + static_cast<std::ptrdiff_t>(done), frame, workFar_.begin());

        canceller_->cancel(workNear_, workFar_);

        if (const auto status = below_->write(workNear_, workFar_); !core::succeeded(status)) {
            near_.erase(near_.begin(), near_.begin() + static_cast<std::ptrdiff_t>(done + frame));
            far_.erase(far_.begin(), far_.begin() + static_cast<std::ptrdiff_t>(done + frame));
            return status;
        }
        done += frame;
    }

    near_.erase(near_.begin(), near_.begin() + static_cast<std::ptrdiff_t>(done));
    far_.erase(far_.begin(), far_.begin() + static_cast<std::ptrdiff_t>(done));

    if (everything && !near_.empty()) {

        const auto status = below_->write(near_, far_);
        near_.clear();
        far_.clear();
        return status;
    }
    return core::EncoderStatus::Ok;
}

core::EncoderStatus EchoCancellingEncoder::finish()
{
    if (!below_) {
        return core::EncoderStatus::Failed;
    }
    const auto flushed = drain(true);
    const auto closed = below_->finish();
    return core::succeeded(flushed) ? closed : flushed;
}

std::uint64_t EchoCancellingEncoder::bytes() const noexcept
{
    return below_ ? below_->bytes() : 0;
}

const char *EchoCancellingEncoder::canceller() const
{
    return canceller_ ? canceller_->name() : "none";
}

}
