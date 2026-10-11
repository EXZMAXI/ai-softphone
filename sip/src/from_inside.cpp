// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/sip/from_inside.hpp>

#include "device_descriptions.hpp"

#include <pjlib.h>
#include <pjmedia/audiodev.h>
#include <pjsua-lib/pjsua.h>

#include <atomic>
#include <mutex>
#include <thread>

namespace softphone::sip {
namespace {

struct Counted {
    std::atomic<long> captured{0};
    std::atomic<long> played{0};
};

Counted &counted()
{
    static Counted one;
    return one;
}

std::mutex &oneAtATime()
{
    static std::mutex one;
    return one;
}

[[nodiscard]] AudioDeviceFacts factsOf(const pjmedia_aud_dev_info &info)
{
    AudioDeviceFacts facts;

    facts.name = asText(info.name);
    facts.inputs = info.input_count;
    facts.outputs = info.output_count;
    return facts;
}

}

void makePjlibReady()
{

    static_cast<void>(pj_init());
}

std::vector<AudioDeviceFacts> devicesInTheStack()
{
    std::vector<AudioDeviceFacts> devices;
    const auto count = pjmedia_aud_dev_count();
    devices.reserve(count);

    for (unsigned at = 0; at < count; ++at) {
        pjmedia_aud_dev_info info{};
        if (pjmedia_aud_dev_get_info(static_cast<pjmedia_aud_dev_index>(at), &info) != PJ_SUCCESS) {
            continue;
        }
        devices.push_back(factsOf(info));
    }
    return devices;
}

std::optional<AudioDeviceFacts> deviceInTheStack(int id)
{
    pjmedia_aud_dev_info info{};
    if (pjmedia_aud_dev_get_info(static_cast<pjmedia_aud_dev_index>(id), &info) != PJ_SUCCESS) {
        return std::nullopt;
    }
    return factsOf(info);
}

std::optional<std::pair<int, int>> devicesTheStackIsPointedAt()
{
    int capture = 0;
    int playback = 0;
    if (pjsua_get_snd_dev(&capture, &playback) != PJ_SUCCESS) {
        return std::nullopt;
    }
    return std::pair{capture, playback};
}

std::optional<BytesMoved> bytesThrough(int recordId, int playId, std::chrono::milliseconds howLong)
{
    const std::lock_guard running{oneAtATime()};

    counted().captured.store(0);
    counted().played.store(0);

    pjmedia_aud_param param{};
    if (pjmedia_aud_dev_default_param(static_cast<pjmedia_aud_dev_index>(recordId), &param) !=
        PJ_SUCCESS) {
        return std::nullopt;
    }

    param.dir = PJMEDIA_DIR_CAPTURE_PLAYBACK;
    param.rec_id = static_cast<pjmedia_aud_dev_index>(recordId);
    param.play_id = static_cast<pjmedia_aud_dev_index>(playId);
    param.clock_rate = 16000;
    param.channel_count = 1;
    param.samples_per_frame = 320;
    param.bits_per_sample = 16;

    pjmedia_aud_stream *stream = nullptr;
    if (pjmedia_aud_stream_create(
            &param,
            +[](void *, pjmedia_frame *frame) -> pj_status_t {
                counted().captured.fetch_add(static_cast<long>(frame->size));
                return PJ_SUCCESS;
            },
            +[](void *, pjmedia_frame *frame) -> pj_status_t {
                counted().played.fetch_add(static_cast<long>(frame->size));
                frame->type = PJMEDIA_FRAME_TYPE_AUDIO;
                return PJ_SUCCESS;
            },
            nullptr, &stream) != PJ_SUCCESS) {
        return std::nullopt;
    }

    if (pjmedia_aud_stream_start(stream) != PJ_SUCCESS) {
        static_cast<void>(pjmedia_aud_stream_destroy(stream));
        return std::nullopt;
    }

    std::this_thread::sleep_for(howLong);

    static_cast<void>(pjmedia_aud_stream_stop(stream));
    static_cast<void>(pjmedia_aud_stream_destroy(stream));

    BytesMoved moved;
    moved.captured = counted().captured.load();
    moved.played = counted().played.load();
    return moved;
}

}
