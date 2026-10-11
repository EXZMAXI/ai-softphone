// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/capture/system_capture.hpp>

#include <softphone/capture/applications.hpp>
#include <softphone/capture/devices.hpp>

#include <softphone/core/logging.hpp>

#include <pulse/pulseaudio.h>

#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace softphone::capture {
namespace {

constexpr auto kHandshake = std::chrono::seconds{3};

constexpr auto kReconsider = std::chrono::seconds{1};

constexpr std::uint32_t kMeterRate = 25;
constexpr auto kMeterWindow = std::chrono::milliseconds{200};

[[nodiscard]] std::string propertyOf(const pa_proplist *list, const char *key)
{
    if (list == nullptr) {
        return {};
    }
    const char *value = pa_proplist_gets(list, key);
    return value != nullptr ? std::string{value} : std::string{};
}

[[nodiscard]] bool somebodyElses(const pa_proplist *proplist, int corked)
{
    if (corked != 0) {
        return false;
    }

    const auto pid = propertyOf(proplist, PA_PROP_APPLICATION_PROCESS_ID);
    if (!pid.empty() && pid == std::to_string(static_cast<long long>(::getpid()))) {
        return false;
    }

    auto binary = propertyOf(proplist, PA_PROP_APPLICATION_PROCESS_BINARY);
    if (binary.empty()) {
        binary = propertyOf(proplist, PA_PROP_APPLICATION_NAME);
    }
    return !neverAConversation(binary);
}

class PulseCapture final : public ISystemCapture {
public:
    ~PulseCapture() override
    {
        PulseCapture::stop();
    }

    CaptureError start(const CaptureRequest &request, FrameSink sink) override;
    void stop() override;
    [[nodiscard]] CaptureStatus status() const override;

    [[nodiscard]] Yields mustYieldTheSoundCard() const override
    {
        return Yields::Nothing;
    }

private:

    struct Wire {
        pa_stream *stream{nullptr};
        Side side{Side::Near};
        PulseCapture *owner{nullptr};
    };

    struct Metering {
        pa_stream *stream{nullptr};
        float peak{0.0F};
    };

    struct Look {
        std::vector<Device> what;

        std::vector<std::uint32_t> indexes;
    };

    static void onContext(pa_context *context, void *self) noexcept;
    static void onServer(pa_context *context, const pa_server_info *info, void *self) noexcept;
    static void onSourceNamed(pa_context *context, const pa_source_info *info, int eol,
                              void *self) noexcept;
    static void onSink(pa_context *context, const pa_sink_info *info, int eol, void *self) noexcept;
    static void onSource(pa_context *context, const pa_source_info *info, int eol,
                         void *self) noexcept;
    static void onSinkInput(pa_context *context, const pa_sink_input_info *info, int eol,
                            void *self) noexcept;
    static void onSourceOutput(pa_context *context, const pa_source_output_info *info, int eol,
                               void *self) noexcept;
    static void onReadable(pa_stream *stream, std::size_t bytes, void *wire) noexcept;
    static void onStreamState(pa_stream *stream, void *self) noexcept;
    static void onPeak(pa_stream *stream, std::size_t bytes, void *metering) noexcept;

    void deliver(Side side, const std::int16_t *samples, std::size_t count) noexcept;
    [[nodiscard]] bool hasSource(const std::string &name);
    [[nodiscard]] bool openWire(Wire &wire, const std::string &device);
    void tearDown() noexcept;

    [[nodiscard]] Look lookAt(bool outputs);

    void meter(Look &seen);

    [[nodiscard]] std::string wanted(const Look &seen, const std::string &standing, bool outputs);

    [[nodiscard]] bool moveWire(Wire &wire, const std::string &device);

    void closeWire(Wire &wire) noexcept;

    void follow();

    pa_threaded_mainloop *loop_{nullptr};
    pa_context *context_{nullptr};
    Wire near_{nullptr, Side::Near, this};
    Wire far_{nullptr, Side::Far, this};

    CaptureRequest request_;

    FrameSink sink_;

    mutable std::mutex mutex_;
    CaptureStatus status_;

    std::atomic<bool> settled_{false};
    std::string defaultSink_;
    std::string defaultSource_;

    std::atomic<bool> lookedUp_{false};
    bool sourceThere_{false};

    Look sweep_;
    std::atomic<bool> swept_{false};

    std::string farSink_;
    std::string nearSource_;

    std::thread watcher_;
    std::mutex watching_;
    std::condition_variable wake_;
    bool leaving_{false};
};

void PulseCapture::onContext(pa_context *context, void *self) noexcept
{
    auto *const capture = static_cast<PulseCapture *>(self);
    switch (pa_context_get_state(context)) {
    case PA_CONTEXT_READY:
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        pa_threaded_mainloop_signal(capture->loop_, 0);
        break;
    default:
        break;
    }
}

void PulseCapture::onServer(pa_context * , const pa_server_info *info,
                            void *self) noexcept
{
    auto *const capture = static_cast<PulseCapture *>(self);
    if (info != nullptr) {

        capture->defaultSink_ = info->default_sink_name != nullptr ? info->default_sink_name : "";
        capture->defaultSource_ =
            info->default_source_name != nullptr ? info->default_source_name : "";
    }
    capture->settled_.store(true, std::memory_order_release);
    pa_threaded_mainloop_signal(capture->loop_, 0);
}

void PulseCapture::onSink(pa_context * , const pa_sink_info *info, int eol,
                          void *self) noexcept
{
    auto *const capture = static_cast<PulseCapture *>(self);
    if (info != nullptr) {
        Device one;
        one.id = info->name != nullptr ? info->name : "";
        one.name = info->description != nullptr ? info->description : one.id;
        one.console = !one.id.empty() && one.id == capture->defaultSink_;
        capture->sweep_.what.push_back(std::move(one));
        capture->sweep_.indexes.push_back(info->index);
    }
    if (eol != 0) {
        capture->swept_.store(true, std::memory_order_release);
        pa_threaded_mainloop_signal(capture->loop_, 0);
    }
}

void PulseCapture::onSource(pa_context * , const pa_source_info *info, int eol,
                            void *self) noexcept
{
    auto *const capture = static_cast<PulseCapture *>(self);

    if (info != nullptr && info->monitor_of_sink == PA_INVALID_INDEX) {
        Device one;
        one.id = info->name != nullptr ? info->name : "";
        one.name = info->description != nullptr ? info->description : one.id;
        one.console = !one.id.empty() && one.id == capture->defaultSource_;
        capture->sweep_.what.push_back(std::move(one));
        capture->sweep_.indexes.push_back(info->index);
    }
    if (eol != 0) {
        capture->swept_.store(true, std::memory_order_release);
        pa_threaded_mainloop_signal(capture->loop_, 0);
    }
}

void PulseCapture::onSinkInput(pa_context * , const pa_sink_input_info *info, int eol,
                               void *self) noexcept
{
    auto *const capture = static_cast<PulseCapture *>(self);
    if (info != nullptr && somebodyElses(info->proplist, info->corked)) {
        for (std::size_t at = 0; at < capture->sweep_.indexes.size(); ++at) {
            if (capture->sweep_.indexes[at] == info->sink) {
                capture->sweep_.what[at].held = true;
            }
        }
    }
    if (eol != 0) {
        capture->swept_.store(true, std::memory_order_release);
        pa_threaded_mainloop_signal(capture->loop_, 0);
    }
}

void PulseCapture::onSourceOutput(pa_context * , const pa_source_output_info *info,
                                  int eol, void *self) noexcept
{
    auto *const capture = static_cast<PulseCapture *>(self);
    if (info != nullptr && somebodyElses(info->proplist, info->corked)) {
        for (std::size_t at = 0; at < capture->sweep_.indexes.size(); ++at) {
            if (capture->sweep_.indexes[at] == info->source) {
                capture->sweep_.what[at].held = true;
            }
        }
    }
    if (eol != 0) {
        capture->swept_.store(true, std::memory_order_release);
        pa_threaded_mainloop_signal(capture->loop_, 0);
    }
}

void PulseCapture::onSourceNamed(pa_context * , const pa_source_info *info, int eol,
                                 void *self) noexcept
{
    auto *const capture = static_cast<PulseCapture *>(self);
    if (info != nullptr) {
        capture->sourceThere_ = true;
    }

    if (eol != 0) {
        capture->lookedUp_.store(true, std::memory_order_release);
        pa_threaded_mainloop_signal(capture->loop_, 0);
    }
}

void PulseCapture::onStreamState(pa_stream *stream, void *self) noexcept
{
    auto *const capture = static_cast<PulseCapture *>(self);
    switch (pa_stream_get_state(stream)) {
    case PA_STREAM_READY:
    case PA_STREAM_FAILED:
    case PA_STREAM_TERMINATED:
        pa_threaded_mainloop_signal(capture->loop_, 0);
        break;
    default:
        break;
    }
}

void PulseCapture::onReadable(pa_stream *stream, std::size_t , void *wire) noexcept
{
    auto *const one = static_cast<Wire *>(wire);
    auto *const capture = one->owner;

    while (pa_stream_readable_size(stream) > 0) {
        const void *data = nullptr;
        std::size_t bytes = 0;
        if (pa_stream_peek(stream, &data, &bytes) < 0) {
            return;
        }
        if (bytes == 0) {
            return;
        }
        if (data == nullptr) {

            static_cast<void>(pa_stream_drop(stream));
            continue;
        }

        capture->deliver(one->side, static_cast<const std::int16_t *>(data),
                         bytes / sizeof(std::int16_t));
        static_cast<void>(pa_stream_drop(stream));
    }
}

void PulseCapture::deliver(Side side, const std::int16_t *samples, std::size_t count) noexcept
{
    if (!sink_) {
        const std::lock_guard guard{mutex_};
        ++status_.dropped;
        return;
    }

    const auto at = std::chrono::steady_clock::now();
    const auto limit = static_cast<std::size_t>(request_.frameSamples);

    std::size_t done = 0;
    while (done < count) {
        const auto take = std::min(limit, count - done);
        sink_(side, std::span<const std::int16_t>{samples + done, take}, at);
        done += take;
    }

    const std::lock_guard guard{mutex_};
    (side == Side::Near ? status_.nearFrames : status_.farFrames) += 1;
}

bool PulseCapture::hasSource(const std::string &name)
{
    if (name.empty()) {
        return true;
    }

    sourceThere_ = false;
    lookedUp_.store(false, std::memory_order_relaxed);

    auto *const asking = pa_context_get_source_info_by_name(context_, name.c_str(),
                                                            &PulseCapture::onSourceNamed, this);
    if (asking == nullptr) {
        return false;
    }

    while (!lookedUp_.load(std::memory_order_acquire)) {
        pa_threaded_mainloop_wait(loop_);
    }
    pa_operation_unref(asking);
    return sourceThere_;
}

void PulseCapture::onPeak(pa_stream *stream, std::size_t , void *metering) noexcept
{
    auto *const one = static_cast<Metering *>(metering);

    while (pa_stream_readable_size(stream) > 0) {
        const void *data = nullptr;
        std::size_t bytes = 0;
        if (pa_stream_peek(stream, &data, &bytes) < 0) {
            return;
        }
        if (bytes == 0) {
            return;
        }

        if (data != nullptr) {
            const auto *const peaks = static_cast<const float *>(data);
            for (std::size_t at = 0; at < bytes / sizeof(float); ++at) {
                one->peak = std::max(one->peak, peaks[at]);
            }
        }
        static_cast<void>(pa_stream_drop(stream));
    }
}

void PulseCapture::meter(Look &seen)
{
    if (std::ranges::count_if(seen.what, [](const Device &one) { return one.held; }) < 2) {
        return;
    }

    const pa_sample_spec spec{PA_SAMPLE_FLOAT32NE, kMeterRate, 1};
    std::vector<Metering> metering(seen.what.size());

    for (std::size_t at = 0; at < seen.what.size(); ++at) {
        if (!seen.what[at].held) {
            continue;
        }

        auto *const stream = pa_stream_new(context_, "ai-softphone", &spec, nullptr);
        if (stream == nullptr) {
            continue;
        }

        pa_stream_set_state_callback(stream, &PulseCapture::onStreamState, this);
        pa_stream_set_read_callback(stream, &PulseCapture::onPeak, &metering[at]);

        pa_buffer_attr attr{};
        attr.maxlength = static_cast<std::uint32_t>(-1);
        attr.tlength = static_cast<std::uint32_t>(-1);
        attr.prebuf = static_cast<std::uint32_t>(-1);
        attr.minreq = static_cast<std::uint32_t>(-1);
        attr.fragsize = sizeof(float);

        const auto monitor = seen.what[at].id + ".monitor";
        if (pa_stream_connect_record(stream, monitor.c_str(), &attr,
                                     static_cast<pa_stream_flags_t>(
                                         PA_STREAM_PEAK_DETECT | PA_STREAM_ADJUST_LATENCY)) < 0) {
            pa_stream_set_read_callback(stream, nullptr, nullptr);
            pa_stream_set_state_callback(stream, nullptr, nullptr);
            pa_stream_unref(stream);
            continue;
        }

        metering[at].stream = stream;
    }

    pa_threaded_mainloop_unlock(loop_);
    std::this_thread::sleep_for(kMeterWindow);
    pa_threaded_mainloop_lock(loop_);

    std::string readings;

    for (std::size_t at = 0; at < metering.size(); ++at) {
        if (metering[at].stream == nullptr) {
            continue;
        }

        seen.what[at].peak = metering[at].peak;

        std::array<char, 16> said{};
        static_cast<void>(std::snprintf(said.data(), said.size(), "%.3f",
                                        static_cast<double>(metering[at].peak)));
        readings += (readings.empty() ? "" : ", ") + seen.what[at].id + " at " + said.data();

        pa_stream_set_read_callback(metering[at].stream, nullptr, nullptr);
        pa_stream_set_state_callback(metering[at].stream, nullptr, nullptr);
        static_cast<void>(pa_stream_disconnect(metering[at].stream));
        pa_stream_unref(metering[at].stream);
    }

    if (!readings.empty()) {
        core::Logger::instance().log(core::LogLevel::Info, "capture",
                                     "more than one output is carrying sound: " + readings);
    }
}

PulseCapture::Look PulseCapture::lookAt(bool outputs)
{
    sweep_ = Look{};

    const auto ask = [this](pa_operation *asking) {
        if (asking == nullptr) {
            return;
        }
        while (!swept_.load(std::memory_order_acquire)) {
            pa_threaded_mainloop_wait(loop_);
        }
        pa_operation_unref(asking);
    };

    swept_.store(false, std::memory_order_relaxed);
    ask(outputs ? pa_context_get_sink_info_list(context_, &PulseCapture::onSink, this)
                : pa_context_get_source_info_list(context_, &PulseCapture::onSource, this));

    swept_.store(false, std::memory_order_relaxed);
    ask(outputs ? pa_context_get_sink_input_info_list(context_, &PulseCapture::onSinkInput, this)
                : pa_context_get_source_output_info_list(context_, &PulseCapture::onSourceOutput,
                                                         this));

    if (outputs) {
        meter(sweep_);
    }

    return sweep_;
}

std::string PulseCapture::wanted(const Look &seen, const std::string &standing, bool outputs)
{
    const auto at = outputs ? theOneCarryingSound(seen.what) : theOneBeingListenedTo(seen.what);
    if (at == kNoDevice || seen.what[at].id == standing || !seen.what[at].held) {
        return {};
    }

    const auto ours = std::ranges::find_if(
        seen.what, [&standing](const Device &one) { return one.id == standing; });
    if (ours != seen.what.end() && ours->held) {
        return {};
    }

    return seen.what[at].id;
}

bool PulseCapture::moveWire(Wire &wire, const std::string &device)
{
    closeWire(wire);

    if (!openWire(wire, device)) {

        closeWire(wire);
        return false;
    }

    const std::lock_guard guard{mutex_};
    (wire.side == Side::Far ? status_.outputDevice : status_.inputDevice) = device;
    return true;
}

void PulseCapture::closeWire(Wire &wire) noexcept
{
    if (wire.stream == nullptr) {
        return;
    }

    pa_stream_set_read_callback(wire.stream, nullptr, nullptr);
    pa_stream_set_state_callback(wire.stream, nullptr, nullptr);
    static_cast<void>(pa_stream_disconnect(wire.stream));
    pa_stream_unref(wire.stream);
    wire.stream = nullptr;
}

void PulseCapture::follow()
{
    for (;;) {
        {
            std::unique_lock waiting{watching_};
            wake_.wait_for(waiting, kReconsider, [this] { return leaving_; });
            if (leaving_) {
                return;
            }
        }

        pa_threaded_mainloop_lock(loop_);
        if (context_ == nullptr || pa_context_get_state(context_) != PA_CONTEXT_READY) {
            pa_threaded_mainloop_unlock(loop_);
            continue;
        }

        if (request_.outputDevice.empty() && far_.stream != nullptr) {
            const auto seen = lookAt(true);
            if (const auto next = wanted(seen, farSink_, true); !next.empty()) {
                core::Logger::instance().log(core::LogLevel::Info, "capture",
                                             "nobody is using " + farSink_ + " and " + next +
                                                 " is carrying a conversation");
                if (moveWire(far_, next + ".monitor")) {
                    farSink_ = next;
                } else if (!moveWire(far_, farSink_ + ".monitor")) {
                    core::Logger::instance().log(core::LogLevel::Warning, "capture",
                                                 "neither " + next + " nor " + farSink_ +
                                                     " would open; the far side is silent");
                }
            }
        }

        if (request_.inputDevice.empty() && near_.stream != nullptr) {
            const auto seen = lookAt(false);
            if (const auto next = wanted(seen, nearSource_, false); !next.empty()) {
                core::Logger::instance().log(core::LogLevel::Info, "capture",
                                             "nobody is holding " + nearSource_ + " and " + next +
                                                 " is being spoken into");
                if (moveWire(near_, next)) {
                    nearSource_ = next;
                } else if (!moveWire(near_, nearSource_)) {
                    core::Logger::instance().log(core::LogLevel::Warning, "capture",
                                                 "neither " + next + " nor " + nearSource_ +
                                                     " would open; the near side is silent");
                }
            }
        }

        pa_threaded_mainloop_unlock(loop_);
    }
}

bool PulseCapture::openWire(Wire &wire, const std::string &device)
{
    const pa_sample_spec spec{PA_SAMPLE_S16NE, request_.sampleRate, 1};

    wire.owner = this;
    wire.stream = pa_stream_new(context_, "ai-softphone", &spec, nullptr);
    if (wire.stream == nullptr) {
        return false;
    }

    pa_stream_set_state_callback(wire.stream, &PulseCapture::onStreamState, this);
    pa_stream_set_read_callback(wire.stream, &PulseCapture::onReadable, &wire);

    pa_buffer_attr attr{};
    attr.maxlength = static_cast<std::uint32_t>(-1);
    attr.tlength = static_cast<std::uint32_t>(-1);
    attr.prebuf = static_cast<std::uint32_t>(-1);
    attr.minreq = static_cast<std::uint32_t>(-1);
    attr.fragsize = static_cast<std::uint32_t>(request_.frameSamples * sizeof(std::int16_t));

    if (pa_stream_connect_record(wire.stream, device.empty() ? nullptr : device.c_str(), &attr,
                                 static_cast<pa_stream_flags_t>(PA_STREAM_ADJUST_LATENCY)) < 0) {
        return false;
    }

    for (;;) {
        const auto state = pa_stream_get_state(wire.stream);
        if (state == PA_STREAM_READY) {
            return true;
        }
        if (state == PA_STREAM_FAILED || state == PA_STREAM_TERMINATED) {
            return false;
        }
        pa_threaded_mainloop_wait(loop_);
    }
}

CaptureError PulseCapture::start(const CaptureRequest &request, FrameSink sink)
{
    stop();

    request_ = request;
    sink_ = std::move(sink);
    settled_.store(false, std::memory_order_relaxed);
    defaultSink_.clear();
    defaultSource_.clear();
    {
        const std::lock_guard guard{mutex_};
        status_ = {};
    }

    loop_ = pa_threaded_mainloop_new();
    if (loop_ == nullptr) {
        return CaptureError::NoServer;
    }

    context_ = pa_context_new(pa_threaded_mainloop_get_api(loop_), "ai-softphone");
    if (context_ == nullptr) {
        tearDown();
        return CaptureError::NoServer;
    }

    pa_context_set_state_callback(context_, &PulseCapture::onContext, this);

    if (pa_context_connect(context_, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
        tearDown();
        return CaptureError::NoServer;
    }

    if (pa_threaded_mainloop_start(loop_) < 0) {
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

        struct timeval when {};

        pa_gettimeofday(&when);
        pa_timeval_add(&when, 200000);
        static_cast<void>(pa_threaded_mainloop_wait(loop_));
        if (std::chrono::steady_clock::now() > deadline) {
            pa_threaded_mainloop_unlock(loop_);
            tearDown();
            return CaptureError::NoServer;
        }
    }

    if (auto *const asking = pa_context_get_server_info(context_, &PulseCapture::onServer, this)) {
        while (!settled_.load(std::memory_order_acquire)) {
            pa_threaded_mainloop_wait(loop_);
        }
        pa_operation_unref(asking);
    }

    if (request_.outputDevice.empty()) {
        const auto seen = lookAt(true);
        if (const auto at = theOneCarryingSound(seen.what); at != kNoDevice) {
            farSink_ = seen.what[at].id;
        }
    }
    if (farSink_.empty()) {
        farSink_ = defaultSink_;
    }

    if (request_.inputDevice.empty()) {
        const auto seen = lookAt(false);
        if (const auto at = theOneBeingListenedTo(seen.what); at != kNoDevice) {
            nearSource_ = seen.what[at].id;
        }
    }
    if (nearSource_.empty()) {
        nearSource_ = defaultSource_;
    }

    const auto far = !request_.outputDevice.empty() ? request_.outputDevice
                     : !farSink_.empty()            ? farSink_ + ".monitor"
                                                    : std::string{};
    const auto near = !request_.inputDevice.empty() ? request_.inputDevice : nearSource_;

    if (far.empty() && request_.outputDevice.empty()) {
        pa_threaded_mainloop_unlock(loop_);
        tearDown();
        return CaptureError::NoDevice;
    }

    if (!hasSource(far) || !hasSource(near)) {
        pa_threaded_mainloop_unlock(loop_);
        tearDown();
        return CaptureError::NoDevice;
    }

    const bool opened = openWire(far_, far) && openWire(near_, near);
    pa_threaded_mainloop_unlock(loop_);

    if (!opened) {
        tearDown();
        return CaptureError::NoDevice;
    }

    {
        const std::lock_guard guard{mutex_};
        status_.running = true;
        status_.outputDevice = far;
        status_.inputDevice = near;
    }

    {
        const std::lock_guard guard{watching_};
        leaving_ = false;
    }
    watcher_ = std::thread{[this] { follow(); }};

    return CaptureError::None;
}

void PulseCapture::tearDown() noexcept
{

    if (watcher_.joinable()) {
        {
            const std::lock_guard guard{watching_};
            leaving_ = true;
        }
        wake_.notify_all();
        watcher_.join();
    }
    farSink_.clear();
    nearSource_.clear();

    if (loop_ != nullptr) {
        pa_threaded_mainloop_lock(loop_);

        sink_ = nullptr;

        for (Wire *wire : {&far_, &near_}) {
            closeWire(*wire);
        }

        if (context_ != nullptr) {
            pa_context_set_state_callback(context_, nullptr, nullptr);
            pa_context_disconnect(context_);
            pa_context_unref(context_);
            context_ = nullptr;
        }

        pa_threaded_mainloop_unlock(loop_);
        pa_threaded_mainloop_stop(loop_);
        pa_threaded_mainloop_free(loop_);
        loop_ = nullptr;
    } else {
        sink_ = nullptr;
        context_ = nullptr;
    }

    const std::lock_guard guard{mutex_};
    status_.running = false;
}

void PulseCapture::stop()
{
    tearDown();
}

CaptureStatus PulseCapture::status() const
{
    const std::lock_guard guard{mutex_};
    return status_;
}

}

std::unique_ptr<ISystemCapture> makeSystemCapture()
{
    return std::make_unique<PulseCapture>();
}

}
