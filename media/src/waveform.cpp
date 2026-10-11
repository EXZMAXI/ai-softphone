// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/media/waveform.hpp>

#include <softphone/media/mp3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <system_error>

namespace softphone::media {
namespace {

constexpr std::uint32_t kMagic = 0x50454B32;

}

unsigned waveformBucketsFor(std::uint64_t durationMs)
{
    const auto wanted = (durationMs + kWaveformBucketMs - 1) / kWaveformBucketMs;
    return static_cast<unsigned>(
        std::clamp<std::uint64_t>(wanted, kWaveformBucketsMin, kWaveformBucketsMax));
}

std::filesystem::path waveformCacheFor(const std::filesystem::path &file)
{
    auto cache = file;
    cache += ".peaks";
    return cache;
}

Waveform waveformOf(const std::vector<std::int16_t> &interleaved, unsigned channels,
                    unsigned sampleRate)
{
    Waveform picture;
    if (channels == 0 || interleaved.empty()) {
        return picture;
    }

    const std::size_t frames = interleaved.size() / channels;
    if (frames == 0) {
        return picture;
    }

    const auto durationMs =
        sampleRate == 0 ? std::uint64_t{0} : std::uint64_t{frames} * 1000U / sampleRate;
    const auto buckets = waveformBucketsFor(durationMs);

    picture.nearSide.assign(buckets, 0);
    picture.farSide.assign(buckets, 0);

    for (unsigned bucket = 0; bucket < buckets; ++bucket) {
        const std::size_t from = frames * bucket / buckets;
        const std::size_t to = std::max(from + 1, frames * (bucket + 1) / buckets);

        double nearTotal = 0.0;
        double farTotal = 0.0;
        for (std::size_t at = from; at < to && at < frames; ++at) {
            const double one = interleaved[at * channels];
            nearTotal += one * one;
            if (channels > 1) {
                const double other = interleaved[at * channels + 1];
                farTotal += other * other;
            }
        }

        const auto count = static_cast<double>(to - from);

        const auto shade = [count](double total) {
            const double rms = std::sqrt(total / count) / 32768.0;
            if (rms <= 0.0) {
                return std::uint8_t{0};
            }
            const double decibels = 20.0 * std::log10(rms);
            const double scaled = std::clamp((decibels + 60.0) / 60.0, 0.0, 1.0);
            return static_cast<std::uint8_t>(scaled * 255.0);
        };

        picture.nearSide[bucket] = shade(nearTotal);
        picture.farSide[bucket] = channels > 1 ? shade(farTotal) : picture.nearSide[bucket];
    }

    return picture;
}

bool waveformFor(const std::filesystem::path &file, Waveform &out)
{
    out = Waveform{};

    const auto cache = waveformCacheFor(file);

    std::error_code failed;
    if (std::filesystem::exists(cache, failed) && std::filesystem::exists(file, failed) &&
        std::filesystem::last_write_time(cache, failed) >=
            std::filesystem::last_write_time(file, failed)) {
        if (std::FILE *reading = std::fopen(cache.string().c_str(), "rb"); reading != nullptr) {
            std::uint32_t magic = 0;
            std::uint32_t buckets = 0;
            const bool header = std::fread(&magic, sizeof magic, 1, reading) == 1 &&
                                std::fread(&buckets, sizeof buckets, 1, reading) == 1 &&
                                magic == kMagic && buckets >= kWaveformBucketsMin &&
                                buckets <= kWaveformBucketsMax;

            std::vector<std::uint8_t> both(header ? std::size_t{buckets} * 2 : 0);
            const bool body =
                header && std::fread(both.data(), 1, both.size(), reading) == both.size();
            static_cast<void>(std::fclose(reading));

            if (body) {
                out.nearSide.assign(both.begin(), both.begin() + buckets);
                out.farSide.assign(both.begin() + buckets, both.end());
                return true;
            }
        }
    }

    Mp3Info info;
    std::vector<std::int16_t> samples;
    if (!succeeded(decodeMp3(file, samples, info))) {
        return false;
    }

    out = waveformOf(samples, info.channels, info.sampleRate);
    if (out.empty()) {
        return false;
    }

    if (std::FILE *writing = std::fopen(cache.string().c_str(), "wb"); writing != nullptr) {
        const auto buckets = static_cast<std::uint32_t>(out.nearSide.size());
        static_cast<void>(std::fwrite(&kMagic, sizeof kMagic, 1, writing));
        static_cast<void>(std::fwrite(&buckets, sizeof buckets, 1, writing));
        static_cast<void>(std::fwrite(out.nearSide.data(), 1, out.nearSide.size(), writing));
        static_cast<void>(std::fwrite(out.farSide.data(), 1, out.farSide.size(), writing));
        static_cast<void>(std::fclose(writing));
    }

    return true;
}

}
