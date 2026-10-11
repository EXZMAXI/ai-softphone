// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/capture/detector.hpp>

#include <softphone/capture/signal_trail.hpp>

#include <softphone/capture/applications.hpp>

#include <softphone/core/logging.hpp>

#include <pulse/pulseaudio.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <ranges>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace softphone::capture {
namespace {

constexpr auto kHandshake = std::chrono::seconds{3};

constexpr auto kTick = std::chrono::milliseconds{250};

[[nodiscard]] std::string propertyOf(const pa_proplist *list, const char *key)
{
    if (list == nullptr) {
        return {};
    }
    const char *value = pa_proplist_gets(list, key);
    return value != nullptr ? std::string{value} : std::string{};
}

class PulseDetector final : public IConversationDetector {
public:
    ~PulseDetector() override
    {
        PulseDetector::stop();
    }

    CaptureError start(const DetectorSettings &settings, Began began, Ended ended) override;
    void stop() override;

    [[nodiscard]] bool talking() const override
    {
        return talking_.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::string trail() const override
    {
        const std::lock_guard guard{trailing_};
        return trail_.render(settings_.settle, settings_.holdOff);
    }

    [[nodiscard]] bool available() const override
    {
        return true;
    }

private:
    static void onContext(pa_context *context, void *self) noexcept;
    static void onEvent(pa_context *context, pa_subscription_event_type_t type, std::uint32_t index,
                        void *self) noexcept;
    static void onSource(pa_context *context, const pa_source_info *info, int eol,
                         void *self) noexcept;
    static void onSourceOutput(pa_context *context, const pa_source_output_info *info, int eol,
                               void *self) noexcept;
    static void onSinkInput(pa_context *context, const pa_sink_input_info *info, int eol,
                            void *self) noexcept;

    void look() noexcept;
    void tick();
    void tearDown() noexcept;

    pa_threaded_mainloop *loop_{nullptr};
    pa_context *context_{nullptr};

    std::thread ticker_;
    std::atomic<bool> running_{false};
    std::mutex sleeping_;
    std::condition_variable wake_;

    DetectorSettings settings_;
    Began began_;
    Ended ended_;

    std::atomic<bool> held_{false};
    std::atomic<bool> playing_{false};

    std::atomic<bool> duplex_{false};
    std::atomic<bool> talking_{false};

    mutable std::mutex naming_;
    Conversation seen_;

    mutable std::mutex trailing_;
    SignalTrail trail_;
    std::chrono::steady_clock::time_point watching_{};

    std::set<std::uint32_t> monitors_;

    std::set<std::uint32_t> sweepMonitors_;
    bool sweepHeld_{false};
    bool sweepPlaying_{false};
    bool sweepDuplex_{false};
    Conversation sweepSeen_;

    struct Holder {
        std::string pid;

        std::string plain;

        std::string binary;
    };

    std::vector<Holder> sweepHolders_;

    std::set<std::string> sweepPlayers_;

    bool saidHeld_{false};
    bool saidPlaying_{false};
    bool everSaid_{false};
};

void PulseDetector::onContext(pa_context *context, void *self) noexcept
{
    auto *const detector = static_cast<PulseDetector *>(self);
    switch (pa_context_get_state(context)) {
    case PA_CONTEXT_READY:
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        pa_threaded_mainloop_signal(detector->loop_, 0);
        break;
    default:
        break;
    }
}

void PulseDetector::onSource(pa_context * , const pa_source_info *info, int eol,
                             void *self) noexcept
{
    auto *const detector = static_cast<PulseDetector *>(self);
    if (eol != 0) {
        detector->monitors_ = detector->sweepMonitors_;
        return;
    }
    if (info == nullptr) {
        return;
    }
    if (info->monitor_of_sink != PA_INVALID_INDEX) {
        detector->sweepMonitors_.insert(info->index);
    }
}

void PulseDetector::onSourceOutput(pa_context * , const pa_source_output_info *info,
                                   int eol, void *self) noexcept
{
    auto *const detector = static_cast<PulseDetector *>(self);
    if (eol != 0) {

        detector->held_.store(detector->sweepHeld_, std::memory_order_release);
        pa_threaded_mainloop_signal(detector->loop_, 0);
        return;
    }
    if (info == nullptr) {
        return;
    }

    const auto pid = propertyOf(info->proplist, PA_PROP_APPLICATION_PROCESS_ID);
    if (!pid.empty() && detector->settings_.ownProcessId != 0 &&
        pid == std::to_string(detector->settings_.ownProcessId)) {
        return;
    }

    if (info->corked != 0) {
        return;
    }

    if (detector->monitors_.count(info->source) != 0) {
        return;
    }

    auto binary = propertyOf(info->proplist, PA_PROP_APPLICATION_PROCESS_BINARY);
    if (binary.empty()) {
        binary = propertyOf(info->proplist, PA_PROP_APPLICATION_NAME);
    }

    const auto named = propertyOf(info->proplist, PA_PROP_APPLICATION_NAME);
    if (neverAConversation(binary) || neverAConversation(named)) {
        return;
    }

    const auto &excluded = detector->settings_.excluded;
    if (std::find(excluded.begin(), excluded.end(), binary) != excluded.end()) {
        return;
    }

    detector->sweepHeld_ = true;
    detector->sweepHolders_.push_back(Holder{pid, plainly(binary), binary});
}

void PulseDetector::onSinkInput(pa_context * , const pa_sink_input_info *info, int eol,
                                void *self) noexcept
{
    auto *const detector = static_cast<PulseDetector *>(self);
    if (eol != 0) {

        const auto found =
            std::ranges::find_if(detector->sweepHolders_, [detector](const Holder &one) {
                return (!one.pid.empty() && detector->sweepPlayers_.count(one.pid) != 0) ||
                       (!one.plain.empty() && detector->sweepPlayers_.count(one.plain) != 0);
            });
        detector->sweepDuplex_ = found != detector->sweepHolders_.end();

        const Holder naming = detector->sweepDuplex_            ? *found
                              : detector->sweepHolders_.empty() ? Holder{}
                                                                : detector->sweepHolders_.front();
        detector->sweepSeen_.application = naming.binary;

        detector->sweepSeen_.title.clear();

        detector->playing_.store(detector->sweepPlaying_, std::memory_order_release);
        detector->duplex_.store(detector->sweepDuplex_, std::memory_order_release);
        {
            const std::lock_guard guard{detector->naming_};
            detector->seen_ = detector->sweepSeen_;
        }
        pa_threaded_mainloop_signal(detector->loop_, 0);
        return;
    }
    if (info == nullptr) {
        return;
    }

    const auto pid = propertyOf(info->proplist, PA_PROP_APPLICATION_PROCESS_ID);
    if (!pid.empty() && detector->settings_.ownProcessId != 0 &&
        pid == std::to_string(detector->settings_.ownProcessId)) {
        return;
    }

    auto binary = propertyOf(info->proplist, PA_PROP_APPLICATION_PROCESS_BINARY);
    if (binary.empty()) {
        binary = propertyOf(info->proplist, PA_PROP_APPLICATION_NAME);
    }
    if (!pid.empty()) {
        detector->sweepPlayers_.insert(pid);
    }
    if (!binary.empty()) {
        detector->sweepPlayers_.insert(plainly(binary));
    }

    if (info->corked != 0) {
        return;
    }
    detector->sweepPlaying_ = true;
}

void PulseDetector::onEvent(pa_context *context, pa_subscription_event_type_t ,
                            std::uint32_t , void *self) noexcept
{
    static_cast<void>(context);
    static_cast<PulseDetector *>(self)->look();
}

void PulseDetector::look() noexcept
{

    sweepMonitors_.clear();
    if (auto *const asking =
            pa_context_get_source_info_list(context_, &PulseDetector::onSource, this)) {
        pa_operation_unref(asking);
    }

    sweepHeld_ = false;
    sweepSeen_ = {};
    sweepHolders_.clear();
    sweepPlayers_.clear();
    if (auto *const asking = pa_context_get_source_output_info_list(
            context_, &PulseDetector::onSourceOutput, this)) {
        pa_operation_unref(asking);
    }

    sweepPlaying_ = false;
    sweepDuplex_ = false;
    if (auto *const asking =
            pa_context_get_sink_input_info_list(context_, &PulseDetector::onSinkInput, this)) {
        pa_operation_unref(asking);
    }
}

void PulseDetector::tick()
{
    ConversationGate gate{settings_};

    while (running_.load(std::memory_order_acquire)) {
        {
            std::unique_lock sleeping{sleeping_};
            wake_.wait_for(sleeping, kTick,
                           [this] { return !running_.load(std::memory_order_acquire); });
        }
        if (!running_.load(std::memory_order_acquire)) {
            return;
        }

        const auto held = held_.load(std::memory_order_acquire);
        const auto playing = playing_.load(std::memory_order_acquire);
        const auto duplex = duplex_.load(std::memory_order_acquire);

        const auto answering = duplex;

        if (!everSaid_ || held != saidHeld_ || answering != saidPlaying_) {
            everSaid_ = true;
            saidHeld_ = held;
            saidPlaying_ = answering;
            std::string who;
            {
                const std::lock_guard guard{naming_};
                who = seen_.application;
            }
            core::Logger::instance().log(
                core::LogLevel::Debug, "capture",
                std::string{"the machine: microphone "} + (held ? "held" : "free") +
                    (who.empty() ? std::string{} : std::string{" by "} + who) + ", output " +
                    (playing ? "playing" : "quiet") + ", the holder " +
                    (duplex ? "has one open" : "has none"));
        }

        const auto at = std::chrono::steady_clock::now();
        const auto move = gate.saw(held, answering, at);
        talking_.store(gate.talking(), std::memory_order_release);

        {
            const std::lock_guard guard{trailing_};
            trail_.saw(
                Reading{std::chrono::duration_cast<std::chrono::milliseconds>(at - watching_), held,
                        playing, duplex, gate.talking()});
        }

        if (move == ConversationGate::Move::Began && began_) {
            Conversation what;
            {
                const std::lock_guard guard{naming_};
                what = seen_;
            }
            began_(what);
        } else if (move == ConversationGate::Move::Ended && ended_) {
            ended_();
        }
    }
}

CaptureError PulseDetector::start(const DetectorSettings &settings, Began began, Ended ended)
{
    stop();

    settings_ = settings;
    began_ = std::move(began);
    ended_ = std::move(ended);
    {
        const std::lock_guard guard{trailing_};
        trail_.forget();
        watching_ = std::chrono::steady_clock::now();
    }
    held_.store(false, std::memory_order_relaxed);
    playing_.store(false, std::memory_order_relaxed);
    duplex_.store(false, std::memory_order_relaxed);
    talking_.store(false, std::memory_order_relaxed);

    loop_ = pa_threaded_mainloop_new();
    if (loop_ == nullptr) {
        return CaptureError::NoServer;
    }
    context_ = pa_context_new(pa_threaded_mainloop_get_api(loop_), "ai-softphone");
    if (context_ == nullptr) {
        tearDown();
        return CaptureError::NoServer;
    }
    pa_context_set_state_callback(context_, &PulseDetector::onContext, this);
    if (pa_context_connect(context_, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0 ||
        pa_threaded_mainloop_start(loop_) < 0) {
        tearDown();
        return CaptureError::NoServer;
    }

    pa_threaded_mainloop_lock(loop_);
    const auto deadline = std::chrono::steady_clock::now() + kHandshake;
    for (;;) {
        const auto state = pa_context_get_state(context_);
        if (state == PA_CONTEXT_READY) {
            break;
        }
        if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED ||
            std::chrono::steady_clock::now() > deadline) {
            pa_threaded_mainloop_unlock(loop_);
            tearDown();
            return CaptureError::NoServer;
        }
        pa_threaded_mainloop_wait(loop_);
    }

    pa_context_set_subscribe_callback(context_, &PulseDetector::onEvent, this);
    if (auto *const asking = pa_context_subscribe(
            context_,
            static_cast<pa_subscription_mask_t>(PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT |
                                                PA_SUBSCRIPTION_MASK_SINK_INPUT),
            nullptr, nullptr)) {
        pa_operation_unref(asking);
    }

    look();
    pa_threaded_mainloop_unlock(loop_);

    running_.store(true, std::memory_order_release);
    ticker_ = std::thread{[this] { tick(); }};
    return CaptureError::None;
}

void PulseDetector::tearDown() noexcept
{
    running_.store(false, std::memory_order_release);
    wake_.notify_all();

    if (ticker_.get_id() == std::this_thread::get_id()) {
        return;
    }
    if (ticker_.joinable()) {
        ticker_.join();
    }

    if (loop_ != nullptr) {
        pa_threaded_mainloop_lock(loop_);
        if (context_ != nullptr) {
            pa_context_set_subscribe_callback(context_, nullptr, nullptr);
            pa_context_set_state_callback(context_, nullptr, nullptr);
            pa_context_disconnect(context_);
            pa_context_unref(context_);
            context_ = nullptr;
        }
        pa_threaded_mainloop_unlock(loop_);
        pa_threaded_mainloop_stop(loop_);
        pa_threaded_mainloop_free(loop_);
        loop_ = nullptr;
    }
    talking_.store(false, std::memory_order_release);
}

void PulseDetector::stop()
{
    tearDown();
}

}

std::unique_ptr<IConversationDetector> makeConversationDetector()
{
    return std::make_unique<PulseDetector>();
}

}
