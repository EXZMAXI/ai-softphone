// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/codecs.hpp>

#include <algorithm>
#include <charconv>

namespace softphone::core {
namespace {

constexpr std::string_view kOffered[] = {"opus", "G722", "PCMU", "PCMA"};

[[nodiscard]] unsigned numberIn(std::string_view text)
{
    unsigned value = 0;
    const auto *const end = text.data() + text.size();
    const auto [stopped, problem] = std::from_chars(text.data(), end, value);
    if (problem != std::errc{} || stopped != end) {
        return 0;
    }
    return value;
}

}

CodecInfo describeCodec(std::string_view id)
{
    CodecInfo codec;
    codec.id = std::string{id};

    const auto rate = id.find('/');
    if (rate == std::string_view::npos) {
        codec.name = std::string{id};
        return codec;
    }

    codec.name = std::string{id.substr(0, rate)};

    const auto rest = id.substr(rate + 1);
    const auto channels = rest.find('/');
    if (channels == std::string_view::npos) {
        codec.clockRate = numberIn(rest);
        return codec;
    }

    codec.clockRate = numberIn(rest.substr(0, channels));
    codec.channels = numberIn(rest.substr(channels + 1));
    return codec;
}

std::vector<CodecChoice> planCodecs(std::span<const CodecChoice> chosen,
                                    std::span<const CodecInfo> available)
{
    std::vector<CodecChoice> planned;
    planned.reserve(available.size());

    for (const auto &choice : chosen) {
        const auto here = std::ranges::find(available, choice.id, &CodecInfo::id);
        if (here != available.end()) {
            planned.push_back(choice);
        }
    }

    const auto appendIfNew = [&planned](const CodecInfo &codec) {
        if (std::ranges::find(planned, codec.id, &CodecChoice::id) == planned.end()) {
            planned.push_back(CodecChoice{codec.id, offeredByDefault(codec)});
        }
    };

    for (const auto &name : kOffered) {
        for (const auto &codec : available) {
            if (codec.name == name) {
                appendIfNew(codec);
            }
        }
    }

    for (const auto &codec : available) {
        appendIfNew(codec);
    }

    if (!planned.empty() && std::ranges::none_of(planned, &CodecChoice::enabled)) {
        planned.front().enabled = true;
    }

    return planned;
}

bool offeredByDefault(const CodecInfo &codec)
{
    return std::ranges::find(kOffered, codec.name) != std::ranges::end(kOffered);
}

}
