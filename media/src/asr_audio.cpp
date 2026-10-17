// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/media/asr_audio.hpp>

#include <softphone/core/resample.hpp>
#include <softphone/core/uuid.hpp>
#include <softphone/media/mp3.hpp>
#include <softphone/media/waveform.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace softphone::media {
namespace {

using namespace std::chrono_literals;

constexpr std::size_t kBlockFrames = 16000;

[[nodiscard]] std::uint64_t framesIn(std::chrono::milliseconds span, unsigned rate)
{
    if (span <= 0ms || rate == 0) {
        return 0;
    }
    return static_cast<std::uint64_t>(span.count()) * rate / 1000;
}

[[nodiscard]] core::PreparedAudio refuse(std::string reason)
{
    core::PreparedAudio prepared;
    prepared.error = std::move(reason);
    return prepared;
}

void put(std::vector<char> &out, std::uint32_t value)
{
    for (int shift = 0; shift < 32; shift += 8) {
        out.push_back(static_cast<char>((value >> shift) & 0xFFU));
    }
}

void put(std::vector<char> &out, std::uint16_t value)
{
    out.push_back(static_cast<char>(value & 0xFFU));
    out.push_back(static_cast<char>((value >> 8) & 0xFFU));
}

void put(std::vector<char> &out, std::string_view letters)
{
    out.insert(out.end(), letters.begin(), letters.end());
}

[[nodiscard]] std::vector<char> wavHeader(unsigned rate, unsigned channels, std::uint64_t samples)
{
    const auto bytes = static_cast<std::uint32_t>(samples * 2);
    std::vector<char> header;
    header.reserve(44);
    put(header, std::string_view{"RIFF"});
    put(header, static_cast<std::uint32_t>(36 + bytes));
    put(header, std::string_view{"WAVE"});
    put(header, std::string_view{"fmt "});
    put(header, static_cast<std::uint32_t>(16));
    put(header, static_cast<std::uint16_t>(1));
    put(header, static_cast<std::uint16_t>(channels));
    put(header, static_cast<std::uint32_t>(rate));
    put(header, static_cast<std::uint32_t>(rate * channels * 2));
    put(header, static_cast<std::uint16_t>(channels * 2));
    put(header, static_cast<std::uint16_t>(16));
    put(header, std::string_view{"data"});
    put(header, bytes);
    return header;
}

[[nodiscard]] std::string extensionFor(std::string_view mime)
{
    if (mime == "audio/wav") {
        return ".wav";
    }
    return ".mp3";
}

[[nodiscard]] bool gather(Mp3Decoder &decoder, const Mp3Info &info, const core::AsrInput &input,
                          std::vector<std::int16_t> &out, unsigned &channelsKept)
{
    const auto channels = std::max(info.channels, 1U);
    channelsKept = input.sourceChannel.has_value() ? 1U : channels;
    if (input.sourceChannel.has_value() && *input.sourceChannel >= channels) {
        return false;
    }

    const auto first = framesIn(input.from, info.sampleRate);
    const auto wanted =
        input.duration > 0ms ? framesIn(input.duration, info.sampleRate) : info.frames;
    if (first > 0 && !succeeded(decoder.seek(first))) {
        return false;
    }

    std::vector<std::int16_t> block(kBlockFrames * channels);
    std::uint64_t taken = 0;
    while (taken < wanted) {
        const auto room = static_cast<std::size_t>(
            std::min<std::uint64_t>(kBlockFrames, wanted - taken) * channels);
        const auto got = decoder.read(std::span<std::int16_t>{block.data(), room});
        if (got == 0) {
            break;
        }
        const auto frames = got / channels;
        if (input.sourceChannel.has_value()) {
            for (std::size_t frame = 0; frame < frames; ++frame) {
                out.push_back(block[frame * channels + *input.sourceChannel]);
            }
        } else {
            out.insert(out.end(), block.begin(),
                       block.begin() + static_cast<std::ptrdiff_t>(frames * channels));
        }
        taken += frames;
    }
    return true;
}

}

core::PreparedAudio prepareAsrAudio(const core::AsrInput &input,
                                    const std::filesystem::path &scratch)
{
    if (input.file.empty()) {
        return refuse("there is no file to send");
    }

    if (!input.rewrite) {
        core::PreparedAudio prepared;
        prepared.file = input.file;
        prepared.temporary = false;
        return prepared;
    }

    Mp3Decoder decoder;
    if (const auto status = decoder.open(input.file); !succeeded(status)) {
        return refuse("the recording would not open: " + std::string{toString(status)});
    }
    const auto info = decoder.info();
    if (info.sampleRate == 0 || info.frames == 0) {
        return refuse("the recording holds no audio");
    }

    std::vector<std::int16_t> samples;
    unsigned channelsKept = 1;
    if (!gather(decoder, info, input, samples, channelsKept)) {
        return refuse("the recording does not have the channel this request wants");
    }
    if (samples.empty()) {
        return refuse("there is no audio in the part of the recording this request wants");
    }

    std::error_code failed;
    std::filesystem::create_directories(scratch, failed);
    if (failed) {
        return refuse("nowhere to write the audio: " + failed.message());
    }

    const auto file = scratch / ("asr-" + core::makeUuid() + extensionFor(input.mimeType));

    if (input.mimeType == "audio/wav") {
        std::ofstream out{file, std::ios::binary};
        if (!out) {
            return refuse("the audio could not be written");
        }
        const auto header = wavHeader(info.sampleRate, channelsKept, samples.size());
        out.write(header.data(), static_cast<std::streamsize>(header.size()));
        out.write(reinterpret_cast<const char *>(samples.data()),
                  static_cast<std::streamsize>(samples.size() * sizeof(std::int16_t)));
        if (!out) {
            return refuse("the audio could not be written");
        }
    } else {
        core::EncoderSettings settings;
        settings.sampleRate = info.sampleRate;
        settings.channels = channelsKept;

        Mp3Encoder encoder;
        if (const auto status = encoder.open(file, settings); !core::succeeded(status)) {
            return refuse("the audio could not be written: " + std::string{toString(status)});
        }

        if (channelsKept == 1) {
            if (const auto status = encoder.write(samples, {}); !core::succeeded(status)) {
                return refuse("the audio could not be written");
            }
        } else {
            const auto frames = samples.size() / channelsKept;
            std::vector<std::int16_t> left(frames);
            std::vector<std::int16_t> right(frames);
            for (std::size_t frame = 0; frame < frames; ++frame) {
                left[frame] = samples[frame * channelsKept];
                right[frame] = samples[frame * channelsKept + 1];
            }
            if (const auto status = encoder.write(left, right); !core::succeeded(status)) {
                return refuse("the audio could not be written");
            }
        }
        if (const auto status = encoder.finish(); !core::succeeded(status)) {
            return refuse("the audio could not be closed");
        }
    }

    core::PreparedAudio prepared;
    prepared.file = file;
    prepared.temporary = true;
    return prepared;
}

core::AudioPreparer makeAsrPreparer(std::filesystem::path scratch)
{
    return [scratch = std::move(scratch)](const core::AsrInput &input) {
        return prepareAsrAudio(input, scratch);
    };
}

bool readPcmForAsr(const std::filesystem::path &file, unsigned rate,
                   std::vector<std::int16_t> &mono)
{
    mono.clear();
    if (rate == 0) {
        return false;
    }

    Mp3Decoder decoder;
    if (!succeeded(decoder.open(file))) {
        return false;
    }

    const auto info = decoder.info();
    if (info.sampleRate == 0 || info.channels == 0) {
        return false;
    }

    std::vector<std::int16_t> interleaved(static_cast<std::size_t>(info.sampleRate) *
                                          info.channels);
    std::vector<std::int16_t> flattened;
    std::vector<std::int16_t> converted;
    core::ResampleState held;

    mono.reserve(static_cast<std::size_t>(info.frames) * rate / info.sampleRate + rate);

    for (;;) {
        const auto read = decoder.read(interleaved);
        if (read == 0) {
            break;
        }

        const auto frames = read / info.channels;
        flattened.resize(frames);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            if (info.channels == 1) {
                flattened[frame] = interleaved[frame];
                continue;
            }

            std::int32_t total = 0;
            for (unsigned channel = 0; channel < info.channels; ++channel) {
                total += interleaved[frame * info.channels + channel];
            }
            flattened[frame] = static_cast<std::int16_t>(total / static_cast<int>(info.channels));
        }

        if (info.sampleRate == rate) {
            mono.insert(mono.end(), flattened.begin(), flattened.end());
            continue;
        }

        converted.clear();
        core::resample(held, flattened, info.sampleRate, rate, converted);
        mono.insert(mono.end(), converted.begin(), converted.end());
    }

    decoder.close();
    return !mono.empty();
}

core::PcmReader makePcmReader()
{
    return [](const std::filesystem::path &file, unsigned rate, std::vector<std::int16_t> &mono) {
        return readPcmForAsr(file, rate, mono);
    };
}

bool readSidesForRehearsal(
    const std::filesystem::path &file, unsigned rate,
    const std::function<bool(std::span<const std::int16_t>, std::span<const std::int16_t>)> &taking)
{
    if (rate == 0 || !taking) {
        return false;
    }

    Mp3Decoder decoder;
    if (!succeeded(decoder.open(file))) {
        return false;
    }

    const auto info = decoder.info();
    if (info.sampleRate == 0 || info.channels == 0) {
        return false;
    }

    const auto sides = info.channels >= 2 ? 2U : 1U;

    std::vector<std::int16_t> interleaved(static_cast<std::size_t>(info.sampleRate) *
                                          info.channels);
    std::array<std::vector<std::int16_t>, 2> one;
    std::array<std::vector<std::int16_t>, 2> converted;

    std::array<core::ResampleState, 2> held;

    bool anything = false;
    for (;;) {
        const auto read = decoder.read(interleaved);
        if (read == 0) {
            break;
        }
        const auto frames = read / info.channels;

        for (unsigned side = 0; side < sides; ++side) {
            one[side].resize(frames);
            for (std::size_t frame = 0; frame < frames; ++frame) {
                one[side][frame] = interleaved[frame * info.channels + side];
            }
            if (info.sampleRate == rate) {
                continue;
            }
            converted[side].clear();
            core::resample(held[side], one[side], info.sampleRate, rate, converted[side]);
        }

        const auto lane = [&one, &converted, &info, rate](unsigned side) {
            return info.sampleRate == rate ? std::span<const std::int16_t>{one[side]}
                                           : std::span<const std::int16_t>{converted[side]};
        };

        const auto near = lane(0);
        const auto far = sides > 1 ? lane(1) : std::span<const std::int16_t>{};
        if (near.empty()) {
            continue;
        }
        anything = true;
        if (!taking(near, far)) {

            decoder.close();
            return false;
        }
    }

    decoder.close();
    return anything;
}

std::vector<core::SpeechSpan> speechSpansIn(const std::filesystem::path &file,
                                            std::chrono::milliseconds duration)
{
    std::vector<core::SpeechSpan> found;
    if (file.empty() || duration <= 0ms) {
        return found;
    }

    Waveform picture;
    if (!waveformFor(file, picture) || picture.empty()) {
        return found;
    }

    const auto add = [&found](std::vector<core::SpeechSpan> &&more) {
        found.insert(found.end(), std::make_move_iterator(more.begin()),
                     std::make_move_iterator(more.end()));
    };

    add(core::speechIn(picture.nearSide, duration, 0U));
    if (!picture.farSide.empty()) {
        add(core::speechIn(picture.farSide, duration, 1U));
    }

    std::vector<std::uint8_t> either = picture.nearSide;
    for (std::size_t at = 0; at < either.size() && at < picture.farSide.size(); ++at) {
        either[at] = std::max(either[at], picture.farSide[at]);
    }
    add(core::speechIn(either, duration, std::nullopt));
    return found;
}

core::SpeechFinder makeSpeechFinder()
{
    return [](const std::filesystem::path &file, std::chrono::milliseconds duration) {
        return speechSpansIn(file, duration);
    };
}

}
