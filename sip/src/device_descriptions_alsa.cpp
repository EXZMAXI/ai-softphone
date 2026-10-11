// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "device_descriptions.hpp"

#include <map>
#include <string>
#include <utility>
#include <vector>

#if defined(SOFTPHONE_HAS_ALSA)
#include <alsa/asoundlib.h>

#include <cstdlib>
#endif

#if defined(SOFTPHONE_HAS_PULSE)
#include <pulse/pulseaudio.h>

#include <chrono>
#include <cstdlib>
#include <optional>
#endif

namespace softphone::sip {
namespace {

#if defined(SOFTPHONE_HAS_ALSA)

[[nodiscard]] std::map<std::string, std::string> alsaWords()
{
    std::map<std::string, std::string> descriptions;

    void **hints = nullptr;
    if (snd_device_name_hint(-1, "pcm", &hints) != 0) {
        return descriptions;
    }

    for (void **hint = hints; *hint != nullptr; ++hint) {
        char *name = snd_device_name_get_hint(*hint, "NAME");
        char *description = snd_device_name_get_hint(*hint, "DESC");

        if (name != nullptr && description != nullptr) {
            std::string text{description};

            if (const auto newline = text.find('\n'); newline != std::string::npos) {
                text.resize(newline);
            }

            while (!text.empty() && (text.back() == ' ' || text.back() == ',')) {
                text.pop_back();
            }

            if (!text.empty()) {
                descriptions.emplace(name, std::move(text));
            }
        }

        std::free(name);
        std::free(description);
    }

    snd_device_name_free_hint(hints);
    return descriptions;
}

#else

[[nodiscard]] std::map<std::string, std::string> alsaWords()
{
    return {};
}

#endif

#if defined(SOFTPHONE_HAS_PULSE)

constexpr auto kServerTimeout = std::chrono::milliseconds{300};

struct Collector {
    std::vector<SoundServerNode> nodes;
    bool sourcesDone{false};
    bool sinksDone{false};
};

[[nodiscard]] std::optional<std::pair<std::string, unsigned>> alsaOrigin(pa_proplist *properties)
{
    if (properties == nullptr) {
        return std::nullopt;
    }

    const char *card = pa_proplist_gets(properties, "alsa.id");
    if (card == nullptr) {
        return std::nullopt;
    }

    unsigned device = 0;
    if (const char *within = pa_proplist_gets(properties, "alsa.device"); within != nullptr) {
        device = static_cast<unsigned>(std::atoi(within));
    }

    return std::pair{std::string{card}, device};
}

void onSource(pa_context * , const pa_source_info *info, int end, void *userdata)
{
    auto &collector = *static_cast<Collector *>(userdata);
    if (end != 0) {
        collector.sourcesDone = true;
        return;
    }

    if (info->monitor_of_sink != PA_INVALID_INDEX) {
        return;
    }

    if (const auto origin = alsaOrigin(info->proplist); origin && info->description != nullptr) {
        collector.nodes.push_back(SoundServerNode{origin->first, origin->second,
                                                  core::DeviceRole::Capture, info->description});
    }
}

void onSink(pa_context * , const pa_sink_info *info, int end, void *userdata)
{
    auto &collector = *static_cast<Collector *>(userdata);
    if (end != 0) {
        collector.sinksDone = true;
        return;
    }

    if (const auto origin = alsaOrigin(info->proplist); origin && info->description != nullptr) {
        collector.nodes.push_back(SoundServerNode{origin->first, origin->second,
                                                  core::DeviceRole::Playback, info->description});
    }
}

[[nodiscard]] std::vector<SoundServerNode> soundServerWords()
{
    Collector collector;

    pa_mainloop *loop = pa_mainloop_new();
    if (loop == nullptr) {
        return {};
    }

    pa_context *context = pa_context_new(pa_mainloop_get_api(loop), "ai-softphone");
    if (context == nullptr) {
        pa_mainloop_free(loop);
        return {};
    }

    bool asked = false;
    const auto deadline = std::chrono::steady_clock::now() + kServerTimeout;

    if (pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) >= 0) {
        while (std::chrono::steady_clock::now() < deadline) {
            const auto state = pa_context_get_state(context);

            if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
                break;
            }

            if (state == PA_CONTEXT_READY && !asked) {
                asked = true;
                pa_operation_unref(pa_context_get_source_info_list(context, onSource, &collector));
                pa_operation_unref(pa_context_get_sink_info_list(context, onSink, &collector));
            }

            if (asked && collector.sourcesDone && collector.sinksDone) {
                break;
            }

            if (pa_mainloop_prepare(loop, 20 * 1000) < 0 || pa_mainloop_poll(loop) < 0 ||
                pa_mainloop_dispatch(loop) < 0) {
                break;
            }
        }

        pa_context_disconnect(context);
    }

    pa_context_unref(context);
    pa_mainloop_free(loop);

    if (!collector.sourcesDone || !collector.sinksDone) {
        return {};
    }

    return std::move(collector.nodes);
}

#else

[[nodiscard]] std::vector<SoundServerNode> soundServerWords()
{
    return {};
}

#endif

}

std::string asText(std::string reported)
{

    return reported;
}

std::vector<core::AudioDeviceInfo> described(std::vector<core::AudioDeviceInfo> devices)
{
    const auto nodes = soundServerWords();
    return describedWith(std::move(devices), nodes, alsaWords());
}

}
