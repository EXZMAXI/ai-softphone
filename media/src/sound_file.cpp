// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/media/sound_file.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/media/mp3.hpp>
#include <softphone/media/wav.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <system_error>
#include <vector>

namespace softphone::media {
namespace {

core::RecordingLibrary::FileFacts told(unsigned rate, unsigned channels, std::uint64_t frames)
{
    core::RecordingLibrary::FileFacts facts;
    facts.readable = true;
    facts.sampleRate = rate;
    facts.channels = channels;
    facts.duration = std::chrono::milliseconds{rate == 0 ? 0 : frames * 1000 / rate};
    return facts;
}

}

core::RecordingLibrary::FileFacts inspectSoundFile(const std::filesystem::path &file)
{

    WavInfo wav;
    if (succeeded(inspectWav(file, wav)) && wav.frames > 0) {
        return told(wav.sampleRate, wav.channels, wav.frames);
    }

    Mp3Decoder decoder;
    if (succeeded(decoder.open(file))) {
        const auto &info = decoder.info();
        if (info.frames > 0) {
            return told(info.sampleRate, info.channels, info.frames);
        }
    }

    return {};
}

bool needsConvertingToMp3(const std::filesystem::path &file)
{
    WavInfo wav;
    return succeeded(inspectWav(file, wav)) && wav.frames > 0;
}

bool convertToMp3(const std::filesystem::path &from, const std::filesystem::path &to)
{
    std::vector<std::int16_t> samples;
    WavInfo wav;
    if (!succeeded(decodeWav(from, samples, wav)) || wav.channels == 0 || wav.frames == 0) {
        return false;
    }

    const auto channels = std::min<unsigned>(wav.channels, 2);

    const auto frames =
        std::min<std::uint64_t>(wav.frames, samples.size() / std::max(wav.channels, 1u));

    std::vector<std::int16_t> left;
    std::vector<std::int16_t> right;
    left.reserve(static_cast<std::size_t>(frames));
    if (channels > 1) {
        right.reserve(static_cast<std::size_t>(frames));
    }
    for (std::uint64_t frame = 0; frame < frames; ++frame) {
        const auto at = static_cast<std::size_t>(frame) * wav.channels;
        left.push_back(samples[at]);
        if (channels > 1) {
            right.push_back(samples[at + 1]);
        }
    }

    core::EncoderSettings encoding;
    encoding.sampleRate = wav.sampleRate;
    encoding.channels = channels;

    Mp3Encoder encoder;
    if (encoder.open(to, encoding) != core::EncoderStatus::Ok ||
        encoder.write(left, right) != core::EncoderStatus::Ok ||
        encoder.finish() != core::EncoderStatus::Ok) {
        core::Logger::instance().log(core::LogLevel::Warning, "recordings",
                                     "the imported wav could not be encoded");
        std::error_code ignored;
        std::filesystem::remove(to, ignored);
        return false;
    }
    return true;
}

}
