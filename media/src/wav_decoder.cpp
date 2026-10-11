// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/media/wav.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>

namespace softphone::media {
namespace {

constexpr std::uint16_t kPcm = 0x0001;
constexpr std::uint16_t kFloat = 0x0003;
constexpr std::uint16_t kALaw = 0x0006;
constexpr std::uint16_t kMuLaw = 0x0007;
constexpr std::uint16_t kExtensible = 0xFFFE;

constexpr std::uint64_t kMostFrames = 3'600ULL * 192'000ULL;

[[nodiscard]] std::uint16_t little16(const std::array<unsigned char, 4> &bytes) noexcept
{
    return static_cast<std::uint16_t>(bytes[0] | (bytes[1] << 8));
}

[[nodiscard]] std::uint32_t little32(const std::array<unsigned char, 4> &bytes) noexcept
{
    return static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8) |
           (static_cast<std::uint32_t>(bytes[2]) << 16) |
           (static_cast<std::uint32_t>(bytes[3]) << 24);
}

[[nodiscard]] bool readExactly(std::istream &from, char *into, std::streamsize many)
{
    from.read(into, many);
    return from.gcount() == many;
}

[[nodiscard]] std::int16_t fromALaw(unsigned char value) noexcept
{
    value ^= 0x55U;
    const auto sign = (value & 0x80U) != 0;
    const auto exponent = static_cast<unsigned>((value & 0x70U) >> 4U);
    auto mantissa = static_cast<int>(value & 0x0FU);

    mantissa = exponent == 0 ? (mantissa << 4) + 8 : ((mantissa << 4) + 0x108) << (exponent - 1);
    return static_cast<std::int16_t>(sign ? -mantissa : mantissa);
}

[[nodiscard]] std::int16_t fromMuLaw(unsigned char value) noexcept
{
    value = static_cast<unsigned char>(~value);
    const auto sign = (value & 0x80U) != 0;
    const auto exponent = static_cast<unsigned>((value & 0x70U) >> 4U);
    const auto mantissa = static_cast<int>(value & 0x0FU);

    auto sample = (((mantissa << 3) + 0x84) << exponent) - 0x84;
    return static_cast<std::int16_t>(sign ? -sample : sample);
}

[[nodiscard]] std::int16_t clampToSixteen(double value) noexcept
{
    constexpr auto kTop = static_cast<double>(std::numeric_limits<std::int16_t>::max());
    constexpr auto kBottom = static_cast<double>(std::numeric_limits<std::int16_t>::min());
    return static_cast<std::int16_t>(std::lround(std::clamp(value, kBottom, kTop)));
}

struct Shape {
    std::uint16_t format{0};
    std::uint16_t channels{0};
    std::uint32_t sampleRate{0};
    std::uint16_t bits{0};

    [[nodiscard]] unsigned bytesPerSample() const noexcept
    {
        return (bits + 7U) / 8U;
    }

    [[nodiscard]] bool readable() const noexcept
    {
        if (channels == 0 || sampleRate == 0 || bits == 0) {
            return false;
        }
        switch (format) {
        case kPcm:
            return bits == 8 || bits == 16 || bits == 24 || bits == 32;
        case kFloat:
            return bits == 32 || bits == 64;
        case kALaw:
        case kMuLaw:
            return bits == 8;
        default:
            return false;
        }
    }
};

[[nodiscard]] std::int16_t sampleFrom(const Shape &shape, const unsigned char *bytes) noexcept
{
    switch (shape.format) {
    case kALaw:
        return fromALaw(bytes[0]);
    case kMuLaw:
        return fromMuLaw(bytes[0]);
    case kFloat: {
        if (shape.bits == 32) {
            std::uint32_t raw = static_cast<std::uint32_t>(bytes[0]) |
                                (static_cast<std::uint32_t>(bytes[1]) << 8) |
                                (static_cast<std::uint32_t>(bytes[2]) << 16) |
                                (static_cast<std::uint32_t>(bytes[3]) << 24);
            return clampToSixteen(static_cast<double>(std::bit_cast<float>(raw)) * 32767.0);
        }
        std::uint64_t raw = 0;
        for (int at = 7; at >= 0; --at) {
            raw = (raw << 8) | bytes[at];
        }
        return clampToSixteen(std::bit_cast<double>(raw) * 32767.0);
    }
    default:
        break;
    }

    switch (shape.bits) {
    case 8:

        return static_cast<std::int16_t>((static_cast<int>(bytes[0]) - 128) << 8);
    case 16:
        return static_cast<std::int16_t>(static_cast<std::uint16_t>(bytes[0]) |
                                         (static_cast<std::uint16_t>(bytes[1]) << 8));
    case 24: {
        auto raw = static_cast<std::int32_t>(static_cast<std::uint32_t>(bytes[0]) << 8 |
                                             static_cast<std::uint32_t>(bytes[1]) << 16 |
                                             static_cast<std::uint32_t>(bytes[2]) << 24);
        return static_cast<std::int16_t>(raw >> 16);
    }
    default: {
        auto raw = static_cast<std::int32_t>(static_cast<std::uint32_t>(bytes[0]) |
                                             static_cast<std::uint32_t>(bytes[1]) << 8 |
                                             static_cast<std::uint32_t>(bytes[2]) << 16 |
                                             static_cast<std::uint32_t>(bytes[3]) << 24);
        return static_cast<std::int16_t>(raw >> 16);
    }
    }
}

[[nodiscard]] DecoderStatus walkTo(std::istream &from, Shape &shape, std::uint32_t &audioBytes)
{
    std::array<unsigned char, 4> tag{};
    std::array<unsigned char, 4> length{};

    if (!readExactly(from, reinterpret_cast<char *>(tag.data()), 4) ||
        std::memcmp(tag.data(), "RIFF", 4) != 0) {
        return DecoderStatus::Malformed;
    }
    if (!readExactly(from, reinterpret_cast<char *>(length.data()), 4) ||
        !readExactly(from, reinterpret_cast<char *>(tag.data()), 4) ||
        std::memcmp(tag.data(), "WAVE", 4) != 0) {
        return DecoderStatus::Malformed;
    }

    auto haveShape = false;
    while (readExactly(from, reinterpret_cast<char *>(tag.data()), 4) &&
           readExactly(from, reinterpret_cast<char *>(length.data()), 4)) {
        const auto size = little32(length);

        if (std::memcmp(tag.data(), "fmt ", 4) == 0) {
            if (size < 16) {
                return DecoderStatus::Malformed;
            }

            std::array<unsigned char, 4> field{};
            const auto read16 = [&from, &field]() -> std::uint16_t {
                return readExactly(from, reinterpret_cast<char *>(field.data()), 2)
                           ? little16(field)
                           : 0;
            };
            const auto read32 = [&from, &field]() -> std::uint32_t {
                return readExactly(from, reinterpret_cast<char *>(field.data()), 4)
                           ? little32(field)
                           : 0;
            };

            shape.format = read16();
            shape.channels = read16();
            shape.sampleRate = read32();
            static_cast<void>(read32());
            static_cast<void>(read16());
            shape.bits = read16();

            auto rest = size - 16;
            if (shape.format == kExtensible && rest >= 24) {

                static_cast<void>(read16());
                static_cast<void>(read16());
                static_cast<void>(read32());
                shape.format = read16();
                rest -= 10;
            }

            from.seekg(static_cast<std::streamoff>(rest), std::ios::cur);
            haveShape = true;
        } else if (std::memcmp(tag.data(), "data", 4) == 0) {
            if (!haveShape) {
                return DecoderStatus::Malformed;
            }
            audioBytes = size;
            return DecoderStatus::Ok;
        } else {
            from.seekg(static_cast<std::streamoff>(size), std::ios::cur);
        }

        if ((size % 2) != 0) {
            from.seekg(1, std::ios::cur);
        }
        if (!from) {
            break;
        }
    }

    return DecoderStatus::Malformed;
}

[[nodiscard]] DecoderStatus openAndWalk(const std::filesystem::path &file, std::ifstream &from,
                                        Shape &shape, std::uint32_t &audioBytes)
{
    from.open(file, std::ios::binary);
    if (!from) {
        return DecoderStatus::Missing;
    }

    if (const auto walked = walkTo(from, shape, audioBytes); !succeeded(walked)) {
        return walked;
    }
    return shape.readable() ? DecoderStatus::Ok : DecoderStatus::Malformed;
}

}

DecoderStatus inspectWav(const std::filesystem::path &file, WavInfo &info)
{
    info = WavInfo{};

    std::ifstream from;
    Shape shape;
    std::uint32_t audioBytes = 0;
    if (const auto opened = openAndWalk(file, from, shape, audioBytes); !succeeded(opened)) {
        return opened;
    }

    const auto perFrame = shape.bytesPerSample() * shape.channels;
    if (perFrame == 0) {
        return DecoderStatus::Malformed;
    }

    const auto here = from.tellg();
    from.seekg(0, std::ios::end);
    const auto ends = from.tellg();
    const auto available = ends > here ? static_cast<std::uint64_t>(ends - here) : 0ULL;
    const auto usable = std::min<std::uint64_t>(audioBytes, available);

    info.sampleRate = shape.sampleRate;
    info.channels = shape.channels;
    info.frames = usable / perFrame;
    return info.frames == 0 ? DecoderStatus::Malformed : DecoderStatus::Ok;
}

DecoderStatus decodeWav(const std::filesystem::path &file, std::vector<std::int16_t> &interleaved,
                        WavInfo &info)
{
    interleaved.clear();
    info = WavInfo{};

    std::ifstream from;
    Shape shape;
    std::uint32_t audioBytes = 0;
    if (const auto opened = openAndWalk(file, from, shape, audioBytes); !succeeded(opened)) {
        return opened;
    }

    const auto perFrame = shape.bytesPerSample() * shape.channels;
    if (perFrame == 0) {
        return DecoderStatus::Malformed;
    }

    std::vector<char> raw;
    raw.resize(audioBytes);
    from.read(raw.data(), static_cast<std::streamsize>(audioBytes));
    const auto got = static_cast<std::uint64_t>(from.gcount());

    const auto frames = std::min(got / perFrame, kMostFrames);
    if (frames == 0) {
        return DecoderStatus::Malformed;
    }

    interleaved.resize(static_cast<std::size_t>(frames) * shape.channels);
    const auto *bytes = reinterpret_cast<const unsigned char *>(raw.data());
    const auto width = shape.bytesPerSample();
    for (std::size_t at = 0; at < interleaved.size(); ++at) {
        interleaved[at] = sampleFrom(shape, bytes + (at * width));
    }

    info.sampleRate = shape.sampleRate;
    info.channels = shape.channels;
    info.frames = frames;
    return DecoderStatus::Ok;
}

}
