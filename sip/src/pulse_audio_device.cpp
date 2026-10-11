// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/sip/pulse_audio_device.hpp>

#include <pjlib.h>

#include <pjmedia-audiodev/audiodev_imp.h>
#include <pjmedia/audiodev.h>

#include <pulse/pulseaudio.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>
#include <vector>

namespace {

constexpr const char *kDriver = "PulseAudio";

constexpr auto kHandshakeUs = 3 * 1000 * 1000;

struct Device {

    std::string address;

    std::string shown;

    unsigned inputs{0};
    unsigned outputs{0};
    unsigned rate{48000};
};

struct Factory {

    pjmedia_aud_dev_factory base{};

    pj_pool_factory *pools{nullptr};
    pj_pool_t *pool{nullptr};

    pa_threaded_mainloop *loop{nullptr};
    pa_context *context{nullptr};

    std::vector<Device> devices;

    std::vector<Device> sweep;
    bool done{false};

    std::string defaultSink;
    std::string defaultSource;
};

struct Stream {
    pjmedia_aud_stream base{};

    Factory *factory{nullptr};
    pjmedia_aud_param param{};
    pjmedia_aud_rec_cb recorded{nullptr};
    pjmedia_aud_play_cb played{nullptr};
    void *user{nullptr};

    pa_threaded_mainloop *loop{nullptr};
    pa_context *context{nullptr};
    pa_stream *capture{nullptr};
    pa_stream *playback{nullptr};

    std::size_t frameBytes{0};

    std::vector<char> capturing;
    std::vector<char> playing;

    pj_timestamp captureClock{};
    pj_timestamp playClock{};

    pj_thread_desc captureDesc{};
    pj_thread_desc playDesc{};
    pj_thread_t *captureThread{nullptr};
    pj_thread_t *playThread{nullptr};

    bool running{false};
};

void beKnown(pj_thread_desc &desc, pj_thread_t *&thread, const char *name)
{
    if (thread != nullptr || pj_thread_is_registered()) {
        return;
    }
    std::memset(&desc, 0, sizeof(desc));
    pj_thread_register(name, desc, &thread);
}

void onContext(pa_context *context, void *self) noexcept
{
    auto *const factory = static_cast<Factory *>(self);
    switch (pa_context_get_state(context)) {
    case PA_CONTEXT_READY:
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        pa_threaded_mainloop_signal(factory->loop, 0);
        break;
    default:
        break;
    }
}

void onServer(pa_context * , const pa_server_info *info, void *self) noexcept
{
    auto *const factory = static_cast<Factory *>(self);
    if (info != nullptr) {
        factory->defaultSink = info->default_sink_name != nullptr ? info->default_sink_name : "";
        factory->defaultSource =
            info->default_source_name != nullptr ? info->default_source_name : "";
    }
    factory->done = true;
    pa_threaded_mainloop_signal(factory->loop, 0);
}

void onSink(pa_context * , const pa_sink_info *info, int eol, void *self) noexcept
{
    auto *const factory = static_cast<Factory *>(self);
    if (eol != 0) {
        factory->done = true;
        pa_threaded_mainloop_signal(factory->loop, 0);
        return;
    }
    if (info == nullptr) {
        return;
    }
    Device device;
    device.address = info->name != nullptr ? info->name : "";
    device.shown = info->description != nullptr ? info->description : device.address;
    device.outputs = info->sample_spec.channels != 0 ? info->sample_spec.channels : 2;
    device.rate = info->sample_spec.rate != 0 ? info->sample_spec.rate : 48000;
    factory->sweep.push_back(std::move(device));
}

void onSource(pa_context * , const pa_source_info *info, int eol, void *self) noexcept
{
    auto *const factory = static_cast<Factory *>(self);
    if (eol != 0) {
        factory->done = true;
        pa_threaded_mainloop_signal(factory->loop, 0);
        return;
    }
    if (info == nullptr) {
        return;
    }

    if (info->monitor_of_sink != PA_INVALID_INDEX) {
        return;
    }
    Device device;
    device.address = info->name != nullptr ? info->name : "";
    device.shown = info->description != nullptr ? info->description : device.address;
    device.inputs = info->sample_spec.channels != 0 ? info->sample_spec.channels : 1;
    device.rate = info->sample_spec.rate != 0 ? info->sample_spec.rate : 48000;
    factory->sweep.push_back(std::move(device));
}

[[nodiscard]] bool enumerate(Factory *factory)
{
    factory->sweep.clear();

    factory->done = false;
    factory->defaultSink.clear();
    factory->defaultSource.clear();
    if (auto *const asking = pa_context_get_server_info(factory->context, &onServer, factory)) {
        while (!factory->done) {
            pa_threaded_mainloop_wait(factory->loop);
        }
        pa_operation_unref(asking);
    }

    factory->done = false;
    if (auto *const asking =
            pa_context_get_source_info_list(factory->context, &onSource, factory)) {
        while (!factory->done) {
            pa_threaded_mainloop_wait(factory->loop);
        }
        pa_operation_unref(asking);
    } else {
        return false;
    }

    factory->done = false;
    if (auto *const asking = pa_context_get_sink_info_list(factory->context, &onSink, factory)) {
        while (!factory->done) {
            pa_threaded_mainloop_wait(factory->loop);
        }
        pa_operation_unref(asking);
    } else {
        return false;
    }

    auto &all = factory->sweep;
    const auto firstOf = [&all](bool inputs) {
        return std::find_if(all.begin(), all.end(), [inputs](const Device &one) {
            return inputs ? one.inputs > 0 : one.outputs > 0;
        });
    };
    const auto promote = [&all, &firstOf](const std::string &wanted, bool inputs) {
        if (wanted.empty()) {
            return;
        }
        const auto found = std::find_if(all.begin(), all.end(), [&](const Device &one) {
            return one.address == wanted && (inputs ? one.inputs > 0 : one.outputs > 0);
        });
        const auto first = firstOf(inputs);
        if (found != all.end() && first != all.end() && found != first) {
            std::rotate(first, found, found + 1);
        }
    };
    promote(factory->defaultSource, true);
    promote(factory->defaultSink, false);

    factory->devices = std::move(factory->sweep);
    factory->sweep.clear();
    return true;
}

void onStreamState(pa_stream * , void *self) noexcept
{
    pa_threaded_mainloop_signal(static_cast<Stream *>(self)->loop, 0);
}

void onReadable(pa_stream *stream, std::size_t , void *self) noexcept
{
    auto *const wire = static_cast<Stream *>(self);
    beKnown(wire->captureDesc, wire->captureThread, "pulse-capture");

    while (pa_stream_readable_size(stream) > 0) {
        const void *data = nullptr;
        std::size_t bytes = 0;
        if (pa_stream_peek(stream, &data, &bytes) < 0 || bytes == 0) {
            return;
        }
        if (data != nullptr) {
            const auto *from = static_cast<const char *>(data);
            wire->capturing.insert(wire->capturing.end(), from, from + bytes);
        }
        static_cast<void>(pa_stream_drop(stream));

        while (wire->capturing.size() >= wire->frameBytes && wire->recorded != nullptr) {
            pjmedia_frame frame{};
            frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
            frame.buf = wire->capturing.data();
            frame.size = wire->frameBytes;
            frame.timestamp.u64 = wire->captureClock.u64;
            frame.bit_info = 0;

            static_cast<void>(wire->recorded(wire->user, &frame));

            wire->captureClock.u64 += wire->param.samples_per_frame / wire->param.channel_count;
            wire->capturing.erase(wire->capturing.begin(),
                                  wire->capturing.begin() +
                                      static_cast<std::ptrdiff_t>(wire->frameBytes));
        }
    }
}

void onWritable(pa_stream *stream, std::size_t bytes, void *self) noexcept
{
    auto *const wire = static_cast<Stream *>(self);
    beKnown(wire->playDesc, wire->playThread, "pulse-playback");

    if (wire->played == nullptr || wire->frameBytes == 0) {
        return;
    }

    std::size_t left = bytes;
    while (left >= wire->frameBytes) {
        pjmedia_frame frame{};
        frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
        frame.buf = wire->playing.data();
        frame.size = wire->frameBytes;
        frame.timestamp.u64 = wire->playClock.u64;
        frame.bit_info = 0;

        if (wire->played(wire->user, &frame) != PJ_SUCCESS) {
            return;
        }

        if (frame.type != PJMEDIA_FRAME_TYPE_AUDIO || frame.size == 0) {
            std::fill(wire->playing.begin(), wire->playing.end(), 0);
        }

        if (pa_stream_write(stream, wire->playing.data(), wire->frameBytes, nullptr, 0,
                            PA_SEEK_RELATIVE) < 0) {
            return;
        }
        wire->playClock.u64 += wire->param.samples_per_frame / wire->param.channel_count;
        left -= wire->frameBytes;
    }
}

pj_status_t streamGetParam(pjmedia_aud_stream *base, pjmedia_aud_param *param)
{
    *param = reinterpret_cast<Stream *>(base)->param;
    return PJ_SUCCESS;
}

pj_status_t streamGetCap(pjmedia_aud_stream *base, pjmedia_aud_dev_cap cap, void *value)
{
    auto *const wire = reinterpret_cast<Stream *>(base);
    if (cap == PJMEDIA_AUD_DEV_CAP_INPUT_LATENCY) {
        *static_cast<unsigned *>(value) = wire->param.input_latency_ms;
        return PJ_SUCCESS;
    }
    if (cap == PJMEDIA_AUD_DEV_CAP_OUTPUT_LATENCY) {
        *static_cast<unsigned *>(value) = wire->param.output_latency_ms;
        return PJ_SUCCESS;
    }
    return PJMEDIA_EAUD_INVCAP;
}

pj_status_t streamSetCap(pjmedia_aud_stream *base, pjmedia_aud_dev_cap cap, const void *value)
{
    auto *const wire = reinterpret_cast<Stream *>(base);
    if (cap == PJMEDIA_AUD_DEV_CAP_INPUT_LATENCY) {
        wire->param.input_latency_ms = *static_cast<const unsigned *>(value);
        return PJ_SUCCESS;
    }
    if (cap == PJMEDIA_AUD_DEV_CAP_OUTPUT_LATENCY) {
        wire->param.output_latency_ms = *static_cast<const unsigned *>(value);
        return PJ_SUCCESS;
    }
    return PJMEDIA_EAUD_INVCAP;
}

pj_status_t streamStart(pjmedia_aud_stream *base)
{
    auto *const wire = reinterpret_cast<Stream *>(base);
    if (wire->running) {
        return PJ_SUCCESS;
    }
    pa_threaded_mainloop_lock(wire->loop);
    bool started = true;
    for (pa_stream *one : {wire->capture, wire->playback}) {
        if (one == nullptr) {
            continue;
        }

        auto *const asking = pa_stream_cork(one, 0, nullptr, nullptr);
        if (asking == nullptr) {
            started = false;
        } else {
            pa_operation_unref(asking);
        }
    }
    pa_threaded_mainloop_unlock(wire->loop);
    if (!started) {
        return PJMEDIA_EAUD_SYSERR;
    }
    wire->running = true;
    return PJ_SUCCESS;
}

pj_status_t streamStop(pjmedia_aud_stream *base)
{
    auto *const wire = reinterpret_cast<Stream *>(base);
    if (!wire->running) {
        return PJ_SUCCESS;
    }
    pa_threaded_mainloop_lock(wire->loop);
    for (pa_stream *one : {wire->capture, wire->playback}) {
        if (one == nullptr) {
            continue;
        }
        if (auto *const asking = pa_stream_cork(one, 1, nullptr, nullptr)) {
            pa_operation_unref(asking);
        }
    }
    pa_threaded_mainloop_unlock(wire->loop);
    wire->running = false;
    return PJ_SUCCESS;
}

pj_status_t streamDestroy(pjmedia_aud_stream *base)
{
    auto *const wire = reinterpret_cast<Stream *>(base);
    static_cast<void>(streamStop(base));

    pa_threaded_mainloop_lock(wire->loop);
    for (pa_stream **one : {&wire->capture, &wire->playback}) {
        if (*one != nullptr) {
            pa_stream_set_read_callback(*one, nullptr, nullptr);
            pa_stream_set_write_callback(*one, nullptr, nullptr);
            pa_stream_set_state_callback(*one, nullptr, nullptr);
            static_cast<void>(pa_stream_disconnect(*one));
            pa_stream_unref(*one);
            *one = nullptr;
        }
    }
    pa_threaded_mainloop_unlock(wire->loop);

    wire->~Stream();
    return PJ_SUCCESS;
}

pjmedia_aud_stream_op kStreamOps{
    &streamGetParam, &streamGetCap, &streamSetCap, &streamStart, &streamStop, &streamDestroy,
};

pj_status_t factoryInit(pjmedia_aud_dev_factory *base)
{
    auto *const factory = reinterpret_cast<Factory *>(base);

    factory->loop = pa_threaded_mainloop_new();
    if (factory->loop == nullptr) {
        return PJMEDIA_EAUD_INIT;
    }
    factory->context = pa_context_new(pa_threaded_mainloop_get_api(factory->loop), "ai-softphone");
    if (factory->context == nullptr) {
        return PJMEDIA_EAUD_INIT;
    }
    pa_context_set_state_callback(factory->context, &onContext, factory);
    if (pa_context_connect(factory->context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0 ||
        pa_threaded_mainloop_start(factory->loop) < 0) {
        return PJMEDIA_EAUD_INIT;
    }

    pa_threaded_mainloop_lock(factory->loop);
    pj_time_val started;
    pj_gettimeofday(&started);
    for (;;) {
        const auto state = pa_context_get_state(factory->context);
        if (state == PA_CONTEXT_READY) {
            break;
        }
        pj_time_val now;
        pj_gettimeofday(&now);
        PJ_TIME_VAL_SUB(now, started);
        if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED ||
            PJ_TIME_VAL_MSEC(now) * 1000 > kHandshakeUs) {
            pa_threaded_mainloop_unlock(factory->loop);

            return PJMEDIA_EAUD_INIT;
        }
        pa_threaded_mainloop_wait(factory->loop);
    }

    const bool listed = enumerate(factory);
    pa_threaded_mainloop_unlock(factory->loop);

    return listed && !factory->devices.empty() ? PJ_SUCCESS : PJMEDIA_EAUD_INIT;
}

pj_status_t factoryDestroy(pjmedia_aud_dev_factory *base)
{
    auto *const factory = reinterpret_cast<Factory *>(base);

    if (factory->loop != nullptr) {
        pa_threaded_mainloop_lock(factory->loop);
        if (factory->context != nullptr) {
            pa_context_set_state_callback(factory->context, nullptr, nullptr);
            pa_context_disconnect(factory->context);
            pa_context_unref(factory->context);
            factory->context = nullptr;
        }
        pa_threaded_mainloop_unlock(factory->loop);
        pa_threaded_mainloop_stop(factory->loop);
        pa_threaded_mainloop_free(factory->loop);
        factory->loop = nullptr;
    }

    auto *const pool = factory->pool;
    factory->~Factory();
    pj_pool_release(pool);
    return PJ_SUCCESS;
}

unsigned factoryCount(pjmedia_aud_dev_factory *base)
{
    return static_cast<unsigned>(reinterpret_cast<Factory *>(base)->devices.size());
}

pj_status_t factoryInfo(pjmedia_aud_dev_factory *base, unsigned index, pjmedia_aud_dev_info *info)
{
    auto *const factory = reinterpret_cast<Factory *>(base);
    if (index >= factory->devices.size()) {
        return PJMEDIA_EAUD_INVDEV;
    }
    const auto &device = factory->devices[index];

    std::memset(info, 0, sizeof(*info));
    std::snprintf(info->name, sizeof(info->name), "%s", device.shown.c_str());
    std::snprintf(info->driver, sizeof(info->driver), "%s", kDriver);
    info->input_count = device.inputs;
    info->output_count = device.outputs;
    info->default_samples_per_sec = device.rate;
    info->caps = PJMEDIA_AUD_DEV_CAP_INPUT_LATENCY | PJMEDIA_AUD_DEV_CAP_OUTPUT_LATENCY;
    info->routes = PJMEDIA_AUD_DEV_ROUTE_DEFAULT;
    return PJ_SUCCESS;
}

pj_status_t factoryDefaults(pjmedia_aud_dev_factory *base, unsigned index, pjmedia_aud_param *param)
{
    auto *const factory = reinterpret_cast<Factory *>(base);
    if (index >= factory->devices.size()) {
        return PJMEDIA_EAUD_INVDEV;
    }
    const auto &device = factory->devices[index];

    std::memset(param, 0, sizeof(*param));
    if (device.inputs > 0) {
        param->dir = PJMEDIA_DIR_CAPTURE;
        param->rec_id = static_cast<pjmedia_aud_dev_index>(index);
        param->play_id = PJMEDIA_AUD_INVALID_DEV;
    } else {
        param->dir = PJMEDIA_DIR_PLAYBACK;
        param->play_id = static_cast<pjmedia_aud_dev_index>(index);
        param->rec_id = PJMEDIA_AUD_INVALID_DEV;
    }
    param->clock_rate = device.rate;
    param->channel_count = 1;
    param->samples_per_frame = device.rate * 20 / 1000;
    param->bits_per_sample = 16;
    param->flags = PJMEDIA_AUD_DEV_CAP_INPUT_LATENCY | PJMEDIA_AUD_DEV_CAP_OUTPUT_LATENCY;
    param->input_latency_ms = PJMEDIA_SND_DEFAULT_REC_LATENCY;
    param->output_latency_ms = PJMEDIA_SND_DEFAULT_PLAY_LATENCY;
    return PJ_SUCCESS;
}

[[nodiscard]] pa_stream *openWire(Stream *wire, const Device &device, bool recording,
                                  unsigned latencyMs)
{
    const pa_sample_spec spec{PA_SAMPLE_S16NE, wire->param.clock_rate,
                              static_cast<std::uint8_t>(wire->param.channel_count)};
    auto *const stream =
        pa_stream_new(wire->context, recording ? "ai-softphone capture" : "ai-softphone playback",
                      &spec, nullptr);
    if (stream == nullptr) {
        return nullptr;
    }

    pa_buffer_attr attr{};
    attr.maxlength = static_cast<std::uint32_t>(-1);
    attr.prebuf = static_cast<std::uint32_t>(-1);
    attr.minreq = static_cast<std::uint32_t>(-1);
    const auto latencyBytes =
        static_cast<std::uint32_t>(pa_usec_to_bytes(latencyMs * 1000U, &spec));
    attr.fragsize = static_cast<std::uint32_t>(wire->frameBytes);
    attr.tlength =
        std::max<std::uint32_t>(latencyBytes, static_cast<std::uint32_t>(wire->frameBytes * 2));

    const auto flags =
        static_cast<pa_stream_flags_t>(PA_STREAM_ADJUST_LATENCY | PA_STREAM_START_CORKED);
    const char *address = device.address.empty() ? nullptr : device.address.c_str();

    pa_stream_set_state_callback(stream, &onStreamState, wire);

    if (recording) {
        pa_stream_set_read_callback(stream, &onReadable, wire);
        if (pa_stream_connect_record(stream, address, &attr, flags) < 0) {
            pa_stream_unref(stream);
            return nullptr;
        }
    } else {
        pa_stream_set_write_callback(stream, &onWritable, wire);
        if (pa_stream_connect_playback(stream, address, &attr, flags, nullptr, nullptr) < 0) {
            pa_stream_unref(stream);
            return nullptr;
        }
    }

    for (;;) {
        const auto state = pa_stream_get_state(stream);
        if (state == PA_STREAM_READY) {
            return stream;
        }
        if (state == PA_STREAM_FAILED || state == PA_STREAM_TERMINATED) {
            pa_stream_set_read_callback(stream, nullptr, nullptr);
            pa_stream_set_write_callback(stream, nullptr, nullptr);
            pa_stream_set_state_callback(stream, nullptr, nullptr);
            pa_stream_unref(stream);
            return nullptr;
        }
        pa_threaded_mainloop_wait(wire->loop);
    }
}

pj_status_t factoryCreate(pjmedia_aud_dev_factory *base, const pjmedia_aud_param *param,
                          pjmedia_aud_rec_cb recorded, pjmedia_aud_play_cb played, void *user,
                          pjmedia_aud_stream **out)
{
    auto *const factory = reinterpret_cast<Factory *>(base);
    if (param->bits_per_sample != 16) {
        return PJMEDIA_EAUD_BADFORMAT;
    }

    auto *const memory = pj_pool_zalloc(factory->pool, sizeof(Stream));
    auto *const wire = new (memory) Stream{};

    wire->base.op = &kStreamOps;
    wire->factory = factory;
    wire->param = *param;
    wire->recorded = recorded;
    wire->played = played;
    wire->user = user;
    wire->loop = factory->loop;
    wire->context = factory->context;
    wire->frameBytes = static_cast<std::size_t>(param->samples_per_frame) * sizeof(std::int16_t);
    wire->playing.assign(wire->frameBytes, 0);
    wire->capturing.reserve(wire->frameBytes * 4);

    pa_threaded_mainloop_lock(factory->loop);

    if ((param->dir & PJMEDIA_DIR_CAPTURE) != 0) {
        const auto index = static_cast<unsigned>(param->rec_id);
        if (index >= factory->devices.size()) {
            pa_threaded_mainloop_unlock(factory->loop);
            return PJMEDIA_EAUD_INVDEV;
        }
        wire->capture = openWire(wire, factory->devices[index], true, param->input_latency_ms);
        if (wire->capture == nullptr) {
            pa_threaded_mainloop_unlock(factory->loop);
            return PJMEDIA_EAUD_SYSERR;
        }
    }

    if ((param->dir & PJMEDIA_DIR_PLAYBACK) != 0) {
        const auto index = static_cast<unsigned>(param->play_id);
        if (index >= factory->devices.size()) {
            pa_threaded_mainloop_unlock(factory->loop);
            return PJMEDIA_EAUD_INVDEV;
        }
        wire->playback = openWire(wire, factory->devices[index], false, param->output_latency_ms);
        if (wire->playback == nullptr) {
            pa_threaded_mainloop_unlock(factory->loop);
            return PJMEDIA_EAUD_SYSERR;
        }
    }

    pa_threaded_mainloop_unlock(factory->loop);

    *out = &wire->base;
    return PJ_SUCCESS;
}

pj_status_t factoryRefresh(pjmedia_aud_dev_factory *base)
{
    auto *const factory = reinterpret_cast<Factory *>(base);
    if (factory->loop == nullptr || factory->context == nullptr) {
        return PJMEDIA_EAUD_INIT;
    }
    pa_threaded_mainloop_lock(factory->loop);
    const bool listed = enumerate(factory);
    pa_threaded_mainloop_unlock(factory->loop);
    return listed ? PJ_SUCCESS : PJMEDIA_EAUD_SYSERR;
}

pjmedia_aud_dev_factory_op kFactoryOps{
    &factoryInit,     &factoryDestroy, &factoryCount,   &factoryInfo,
    &factoryDefaults, &factoryCreate,  &factoryRefresh,
};

}

namespace {

struct Defaults {
    std::string sink;
    std::string source;
    bool done{false};
    pa_threaded_mainloop *loop{nullptr};
};

void onDefaults(pa_context * , const pa_server_info *info, void *self) noexcept
{
    auto *const found = static_cast<Defaults *>(self);
    if (info != nullptr) {
        found->sink = info->default_sink_name != nullptr ? info->default_sink_name : "";
        found->source = info->default_source_name != nullptr ? info->default_source_name : "";
    }
    found->done = true;
    pa_threaded_mainloop_signal(found->loop, 0);
}

void onDescription(pa_context * , const pa_sink_info *info, int eol, void *self) noexcept
{
    auto *const found = static_cast<Defaults *>(self);
    if (eol != 0) {
        found->done = true;
        pa_threaded_mainloop_signal(found->loop, 0);
        return;
    }
    if (info != nullptr && info->description != nullptr) {
        found->sink = info->description;
    }
}

void onSourceDescription(pa_context * , const pa_source_info *info, int eol,
                         void *self) noexcept
{
    auto *const found = static_cast<Defaults *>(self);
    if (eol != 0) {
        found->done = true;
        pa_threaded_mainloop_signal(found->loop, 0);
        return;
    }
    if (info != nullptr && info->description != nullptr) {
        found->source = info->description;
    }
}

}

extern "C" int softphone_pulse_aud_default_names(char *sink, unsigned sinkSize, char *source,
                                                 unsigned sourceSize)
{
    if (sink == nullptr || source == nullptr || sinkSize == 0 || sourceSize == 0) {
        return 0;
    }
    sink[0] = '\0';
    source[0] = '\0';

    Defaults found;
    found.loop = pa_threaded_mainloop_new();
    if (found.loop == nullptr) {
        return 0;
    }
    auto *const context = pa_context_new(pa_threaded_mainloop_get_api(found.loop), "ai-softphone");
    if (context == nullptr) {
        pa_threaded_mainloop_free(found.loop);
        return 0;
    }

    bool ok = false;
    pa_context_set_state_callback(
        context,
        [](pa_context *asked, void *self) noexcept {
            switch (pa_context_get_state(asked)) {
            case PA_CONTEXT_READY:
            case PA_CONTEXT_FAILED:
            case PA_CONTEXT_TERMINATED:
                pa_threaded_mainloop_signal(static_cast<Defaults *>(self)->loop, 0);
                break;
            default:
                break;
            }
        },
        &found);

    if (pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) >= 0 &&
        pa_threaded_mainloop_start(found.loop) >= 0) {
        pa_threaded_mainloop_lock(found.loop);
        for (;;) {
            const auto state = pa_context_get_state(context);
            if (state == PA_CONTEXT_READY) {
                break;
            }
            if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
                pa_threaded_mainloop_unlock(found.loop);
                goto done;
            }
            pa_threaded_mainloop_wait(found.loop);
        }

        found.done = false;
        if (auto *const asking = pa_context_get_server_info(context, &onDefaults, &found)) {
            while (!found.done) {
                pa_threaded_mainloop_wait(found.loop);
            }
            pa_operation_unref(asking);
        }

        const auto sinkName = found.sink;
        const auto sourceName = found.source;
        found.done = false;
        if (auto *const asking = pa_context_get_sink_info_by_name(context, sinkName.c_str(),
                                                                  &onDescription, &found)) {
            while (!found.done) {
                pa_threaded_mainloop_wait(found.loop);
            }
            pa_operation_unref(asking);
        }
        found.done = false;
        if (auto *const asking = pa_context_get_source_info_by_name(context, sourceName.c_str(),
                                                                    &onSourceDescription, &found)) {
            while (!found.done) {
                pa_threaded_mainloop_wait(found.loop);
            }
            pa_operation_unref(asking);
        }
        pa_threaded_mainloop_unlock(found.loop);
        ok = !found.sink.empty() && !found.source.empty();
    }

done:
    pa_context_set_state_callback(context, nullptr, nullptr);
    pa_context_disconnect(context);
    pa_context_unref(context);
    pa_threaded_mainloop_stop(found.loop);
    pa_threaded_mainloop_free(found.loop);

    if (ok) {
        std::snprintf(sink, sinkSize, "%s", found.sink.c_str());
        std::snprintf(source, sourceSize, "%s", found.source.c_str());
    }
    return ok ? 1 : 0;
}

extern "C" int softphone_pulse_aud_available()
{

    auto *const loop = pa_threaded_mainloop_new();
    if (loop == nullptr) {
        return 0;
    }
    auto *const context = pa_context_new(pa_threaded_mainloop_get_api(loop), "ai-softphone");
    if (context == nullptr) {
        pa_threaded_mainloop_free(loop);
        return 0;
    }

    bool ready = false;
    pa_context_set_state_callback(
        context,
        [](pa_context *asked, void *self) noexcept {
            switch (pa_context_get_state(asked)) {
            case PA_CONTEXT_READY:
            case PA_CONTEXT_FAILED:
            case PA_CONTEXT_TERMINATED:
                pa_threaded_mainloop_signal(static_cast<pa_threaded_mainloop *>(self), 0);
                break;
            default:
                break;
            }
        },
        loop);

    if (pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) >= 0 &&
        pa_threaded_mainloop_start(loop) >= 0) {
        pa_threaded_mainloop_lock(loop);
        pj_time_val started;
        pj_gettimeofday(&started);
        for (;;) {
            const auto state = pa_context_get_state(context);
            if (state == PA_CONTEXT_READY) {
                ready = true;
                break;
            }
            pj_time_val now;
            pj_gettimeofday(&now);
            PJ_TIME_VAL_SUB(now, started);
            if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED ||
                PJ_TIME_VAL_MSEC(now) * 1000 > kHandshakeUs) {
                break;
            }
            pa_threaded_mainloop_wait(loop);
        }
        pa_threaded_mainloop_unlock(loop);
    }

    pa_context_set_state_callback(context, nullptr, nullptr);
    pa_context_disconnect(context);
    pa_context_unref(context);
    pa_threaded_mainloop_stop(loop);
    pa_threaded_mainloop_free(loop);
    return ready ? 1 : 0;
}

extern "C" pjmedia_aud_dev_factory *softphone_pulse_aud_factory(pj_pool_factory *pools)
{
    auto *const pool = pj_pool_create(pools, "pulse-aud", 1024, 1024, nullptr);
    if (pool == nullptr) {
        return nullptr;
    }
    auto *const memory = pj_pool_zalloc(pool, sizeof(Factory));
    auto *const factory = new (memory) Factory{};
    factory->base.op = &kFactoryOps;
    factory->pools = pools;
    factory->pool = pool;
    return &factory->base;
}
