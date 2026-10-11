// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/capture/capture_session.hpp>

#include <softphone/core/level_meter.hpp>
#include <softphone/core/logging.hpp>

#include <algorithm>
#include <string>
#include <utility>

namespace softphone::capture {
namespace {

constexpr unsigned kHeard = 8;

[[nodiscard]] std::uint64_t samplesBetween(std::chrono::steady_clock::time_point from,
                                           std::chrono::steady_clock::time_point to,
                                           unsigned rate) noexcept
{
    if (to <= from) {
        return 0;
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(to - from).count();

    const auto whole = static_cast<std::uint64_t>(elapsed / 1'000'000'000);
    const auto rest = static_cast<std::uint64_t>(elapsed % 1'000'000'000);
    return whole * rate + rest * rate / 1'000'000'000;
}

constexpr std::chrono::milliseconds kLeadTolerance{60};

}

CaptureSession::CaptureSession(ISystemCapture &capture,
                               core::ConversationRecorder::EncoderFactory factory)
    : capture_{capture}, factory_{std::move(factory)}
{}

CaptureSession::~CaptureSession()
{
    stop();
}

std::optional<std::uint64_t> CaptureSession::positionFor(Side side,
                                                         std::chrono::steady_clock::time_point at,
                                                         std::size_t count) noexcept
{
    if (!anchored_.load(std::memory_order_acquire)) {

        origin_ = at;
        anchored_.store(true, std::memory_order_release);
    }

    const auto wanted = samplesBetween(origin_, at, sampleRate_);
    auto &next = side == Side::Near ? nextNear_ : nextFar_;
    const auto head = next.load(std::memory_order_relaxed);

    if (wanted >= head) {

        next.store(wanted + count, std::memory_order_relaxed);
        return wanted;
    }

    const auto lead = head - wanted;
    const auto tolerance = static_cast<std::uint64_t>(kLeadTolerance.count()) * sampleRate_ / 1000;
    if (lead <= tolerance) {

        next.store(head + count, std::memory_order_relaxed);
        return head;
    }

    shed_.fetch_add(1, std::memory_order_relaxed);
    return std::nullopt;
}

CaptureSession::Started CaptureSession::start(const CaptureRequest &request)
{
    const std::lock_guard guard{mutex_};
    if (running_.load(std::memory_order_acquire)) {
        return {};
    }

    sampleRate_ = request.sampleRate;
    anchored_.store(false, std::memory_order_relaxed);
    nextNear_.store(0, std::memory_order_relaxed);
    nextFar_.store(0, std::memory_order_relaxed);
    shed_.store(0, std::memory_order_relaxed);
    outcome_ = {};
    stopped_ = false;

    recorder_.reset();
    feed_.reset();

    feed_ = std::make_unique<core::RecordingFeed>(request.sampleRate, request.frameSamples);
    recorder_ = std::make_unique<core::ConversationRecorder>(*feed_, factory_);

    if (const auto opened = recorder_->start(); !core::succeeded(opened)) {

        return {opened, CaptureError::None};
    }

    const auto started =
        capture_.start(request, [this](Side side, std::span<const std::int16_t> samples,
                                       std::chrono::steady_clock::time_point at) {

            const auto position = positionFor(side, at, samples.size());
            if (!position.has_value()) {
                return;
            }
            handOver(side, *position, samples);
        });

    if (started != CaptureError::None) {
        outcome_ = recorder_->stop();
        stopped_ = true;
        return {core::EncoderStatus::Ok, started};
    }

    running_.store(true, std::memory_order_release);
    return {};
}

Ending endingFor(const Endings &when, std::chrono::milliseconds elapsed,
                 std::chrono::milliseconds quietFor)
{
    if (when.quiet > std::chrono::milliseconds::zero() && quietFor >= when.quiet) {
        return Ending::Quiet;
    }
    if (when.ceiling > std::chrono::milliseconds::zero() && elapsed >= when.ceiling) {
        return Ending::Roll;
    }
    return Ending::Nothing;
}

std::chrono::milliseconds
CaptureSession::sinceDelivery(Side side, std::chrono::steady_clock::time_point now) const
{
    const auto last =
        (side == Side::Near ? deliveredNear_ : deliveredFar_).load(std::memory_order_relaxed);
    if (last == 0) {
        return std::chrono::milliseconds::zero();
    }
    const auto since =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    if (since <= last) {
        return std::chrono::milliseconds::zero();
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::nanoseconds{since - last});
}

std::chrono::milliseconds CaptureSession::quietFor(std::chrono::steady_clock::time_point now) const
{
    const auto heard = heardAt_.load(std::memory_order_relaxed);
    if (heard == 0) {
        return std::chrono::milliseconds::zero();
    }
    const auto since =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    if (since <= heard) {
        return std::chrono::milliseconds::zero();
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::nanoseconds{since - heard});
}

CaptureSession::Peaks CaptureSession::peaksSinceLastAsked() noexcept
{
    return {peakNear_.exchange(0, std::memory_order_relaxed),
            peakFar_.exchange(0, std::memory_order_relaxed)};
}

namespace {
[[nodiscard]] unsigned loudest(std::span<const std::int16_t> samples) noexcept
{
    int peak = 0;
    for (const auto sample : samples) {
        const int size = sample < 0 ? -static_cast<int>(sample) : static_cast<int>(sample);
        peak = size > peak ? size : peak;
    }

    return static_cast<unsigned>(static_cast<long>(peak) * core::kLoudestLevel / 32767);
}
}

void CaptureSession::keep(Held &held, std::span<const std::int16_t> samples,
                          std::chrono::steady_clock::time_point at) noexcept
{
    if (held.places == 0 || samples.size() > held.stride) {
        return;
    }
    const auto index = held.written.load(std::memory_order_relaxed) % held.places;
    std::copy(samples.begin(), samples.end(),
              held.samples.begin() + static_cast<std::ptrdiff_t>(index) * held.stride);
    held.counts[index] = static_cast<std::uint32_t>(samples.size());
    held.when[index] = at;
    held.written.fetch_add(1, std::memory_order_release);
}

std::uint64_t CaptureSession::surviving(const Held &held, std::uint64_t from,
                                        std::uint64_t written) const noexcept
{
    for (auto at = from; at < written; ++at) {
        const auto index = at % held.places;
        if (held.counts[index] != 0 && held.when[index] >= keepFrom_) {
            return at;
        }
    }
    return written;
}

core::RecordingFeed *CaptureSession::listenTo()
{

    if (feed_ == nullptr) {
        return nullptr;
    }

    feed_->openListening();
    return feed_.get();
}

void CaptureSession::handOver(Side side, std::uint64_t position,
                              std::span<const std::int16_t> samples) noexcept
{
    if (feed_ == nullptr) {
        return;
    }

    auto &ring = side == Side::Near ? feed_->nearSide() : feed_->farSide();
    ring.write(position, samples.data(), samples.size());

    if (!feed_->listening()) {
        return;
    }
    if (auto *const listening = side == Side::Near ? feed_->nearListening() : feed_->farListening();
        listening != nullptr) {
        listening->write(position, samples.data(), samples.size());
    }
}

void CaptureSession::replayHeld()
{
    if (feed_ == nullptr) {
        return;
    }

    const auto pour = [this](Held &held, std::uint64_t at, Side side) {
        const auto index = at % held.places;
        const auto count = held.counts[index];
        const auto position = positionFor(side, held.when[index], count);
        if (!position.has_value()) {
            return;
        }

        auto &ring = side == Side::Near ? feed_->nearSide() : feed_->farSide();
        ring.write(*position,
                   held.samples.data() + static_cast<std::ptrdiff_t>(index) * held.stride, count);
    };

    const auto begin = [](const Held &held, std::uint64_t written) {
        return written > held.places ? written - held.places : 0;
    };

    const auto nearWritten =
        heldNear_.places == 0 ? 0 : heldNear_.written.load(std::memory_order_acquire);
    const auto farWritten =
        heldFar_.places == 0 ? 0 : heldFar_.written.load(std::memory_order_acquire);

    auto nearAt = surviving(heldNear_, begin(heldNear_, nearWritten), nearWritten);
    auto farAt = surviving(heldFar_, begin(heldFar_, farWritten), farWritten);

    while (nearAt < nearWritten || farAt < farWritten) {

        const bool takeFar = nearAt >= nearWritten ||
                             (farAt < farWritten && heldFar_.when[farAt % heldFar_.places] <=
                                                        heldNear_.when[nearAt % heldNear_.places]);

        if (takeFar) {
            pour(heldFar_, farAt, Side::Far);
            farAt = surviving(heldFar_, farAt + 1, farWritten);
        } else {
            pour(heldNear_, nearAt, Side::Near);
            nearAt = surviving(heldNear_, nearAt + 1, nearWritten);
        }
    }
}

CaptureError CaptureSession::arm(const CaptureRequest &request, std::chrono::milliseconds preRoll)
{
    const std::lock_guard guard{mutex_};
    if (armed_.load(std::memory_order_acquire)) {
        return CaptureError::None;
    }

    sampleRate_ = request.sampleRate;
    request_ = request;
    preRoll_ = preRoll;

    const auto frames = static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(preRoll.count()) * request.sampleRate) / 1000 /
        std::max(1U, request.frameSamples));

    for (Held *held : {&heldNear_, &heldFar_}) {
        held->stride = request.frameSamples;
        held->places = std::max<std::uint32_t>(frames, 1);
        held->samples.assign(static_cast<std::size_t>(held->places) * held->stride, 0);
        held->counts.assign(held->places, 0);
        held->when.assign(held->places, std::chrono::steady_clock::time_point{});
        held->written.store(0, std::memory_order_relaxed);
    }

    const auto started = capture_.start(request, [this](Side side,
                                                        std::span<const std::int16_t> samples,
                                                        std::chrono::steady_clock::time_point at) {

        const auto here = loudest(samples);
        const auto raise = [here](std::atomic<unsigned> &mark) {
            auto seen = mark.load(std::memory_order_relaxed);
            while (here > seen &&
                   !mark.compare_exchange_weak(seen, here, std::memory_order_relaxed)) {
            }
        };
        raise(side == Side::Near ? peakNear_ : peakFar_);

        raise(side == Side::Near ? loudestNear_ : loudestFar_);

        (side == Side::Near ? deliveredNear_ : deliveredFar_)
            .store(
                std::chrono::duration_cast<std::chrono::nanoseconds>(at.time_since_epoch()).count(),
                std::memory_order_relaxed);

        if (here >= kHeard) {
            heardAt_.store(
                std::chrono::duration_cast<std::chrono::nanoseconds>(at.time_since_epoch()).count(),
                std::memory_order_relaxed);
        }

        if (auto *const drawing = signals_.load(std::memory_order_acquire)) {
            auto &channel = side == Side::Near ? drawing->local : drawing->remote;
            channel.ring().write(samples.data(), samples.size());
        }

        if (recording_.load(std::memory_order_acquire) && feed_ != nullptr) {
            const auto position = positionFor(side, at, samples.size());
            if (!position.has_value()) {
                return;
            }
            handOver(side, *position, samples);
            return;
        }
        keep(side == Side::Near ? heldNear_ : heldFar_, samples, at);
    });

    if (started != CaptureError::None) {
        return started;
    }

    const auto opened = capture_.status();
    const core::LogAttribute fields[] = {{"microphone", opened.inputDevice},
                                         {"output", opened.outputDevice}};
    core::Logger::instance().log(core::LogLevel::Info, "capture", "listening to this machine",
                                 fields);

    armed_.store(true, std::memory_order_release);
    return CaptureError::None;
}

core::EncoderStatus CaptureSession::beginRecording()
{
    const std::lock_guard guard{mutex_};
    if (!armed_.load(std::memory_order_acquire) || recording_.load(std::memory_order_acquire)) {
        return core::EncoderStatus::Failed;
    }

    anchored_.store(false, std::memory_order_relaxed);
    nextNear_.store(0, std::memory_order_relaxed);
    nextFar_.store(0, std::memory_order_relaxed);
    shed_.store(0, std::memory_order_relaxed);
    loudestNear_.store(0, std::memory_order_relaxed);
    loudestFar_.store(0, std::memory_order_relaxed);

    heardAt_.store(std::chrono::duration_cast<std::chrono::nanoseconds>(
                       std::chrono::steady_clock::now().time_since_epoch())
                       .count(),
                   std::memory_order_relaxed);
    outcome_ = {};
    stopped_ = false;

    keepFrom_ = std::chrono::steady_clock::now() - preRoll_;

    recorder_.reset();
    feed_.reset();

    const auto preRollSeconds =
        static_cast<unsigned>(std::chrono::duration_cast<std::chrono::seconds>(preRoll_).count());
    feed_ = std::make_unique<core::RecordingFeed>(request_.sampleRate, request_.frameSamples,
                                                  preRollSeconds + 1);
    recorder_ = std::make_unique<core::ConversationRecorder>(*feed_, factory_);

    if (const auto opened = recorder_->start(); !core::succeeded(opened)) {
        recorder_.reset();
        feed_.reset();
        return opened;
    }

    replayHeld();

    recording_.store(true, std::memory_order_release);
    running_.store(true, std::memory_order_release);
    return core::EncoderStatus::Ok;
}

core::RecordingOutcome CaptureSession::endRecording()
{
    const std::lock_guard guard{mutex_};
    if (!recording_.load(std::memory_order_acquire)) {
        return outcome_;
    }
    recording_.store(false, std::memory_order_release);
    if (recorder_) {
        outcome_ = recorder_->stop();
    }
    running_.store(false, std::memory_order_release);

    {

        const auto heardNear = loudestNear_.load(std::memory_order_relaxed);
        const auto heardFar = loudestFar_.load(std::memory_order_relaxed);
        const auto status = capture_.status();
        const auto nearPeak = std::to_string(heardNear);
        const auto farPeak = std::to_string(heardFar);
        const auto shed = std::to_string(shed_.load(std::memory_order_relaxed));
        const core::LogAttribute fields[] = {{"near_peak", nearPeak},
                                             {"far_peak", farPeak},
                                             {"shed", shed},
                                             {"microphone", status.inputDevice},
                                             {"output", status.outputDevice}};

        if (heardNear == 0 || heardFar == 0) {
            core::Logger::instance().log(
                core::LogLevel::Warning, "capture",
                heardNear == 0 && heardFar == 0
                    ? "this recording is silent on both sides"
                    : (heardNear == 0 ? "this recording heard nothing from the microphone"
                                      : "this recording heard nothing from the machine's output"),
                fields);
        } else {
            core::Logger::instance().log(core::LogLevel::Info, "capture",
                                         "the recording is finished", fields);
        }
    }

    for (Held *held : {&heldNear_, &heldFar_}) {
        held->written.store(0, std::memory_order_release);
        std::fill(held->counts.begin(), held->counts.end(), 0);
    }
    return outcome_;
}

void CaptureSession::disarm()
{
    static_cast<void>(endRecording());
    const std::lock_guard guard{mutex_};
    if (!armed_.load(std::memory_order_acquire)) {
        return;
    }
    capture_.stop();
    armed_.store(false, std::memory_order_release);
}

core::RecordingOutcome CaptureSession::stop()
{
    const std::lock_guard guard{mutex_};
    if (stopped_) {
        return outcome_;
    }
    stopped_ = true;

    capture_.stop();

    if (recorder_) {
        outcome_ = recorder_->stop();
    }

    running_.store(false, std::memory_order_release);
    return outcome_;
}

bool CaptureSession::running() const noexcept
{
    return running_.load(std::memory_order_acquire);
}

std::chrono::milliseconds CaptureSession::elapsed() const
{
    const std::lock_guard guard{mutex_};
    return recorder_ ? recorder_->elapsed() : std::chrono::milliseconds{0};
}

void CaptureSession::keepAudio(bool keeping)
{
    const std::lock_guard guard{mutex_};
    if (recorder_) {
        recorder_->keepAudio(keeping);
    }
}

CaptureStatus CaptureSession::captureStatus() const
{
    return capture_.status();
}

}
