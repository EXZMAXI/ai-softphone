// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/ringtones.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <system_error>

namespace softphone::core {
namespace {

constexpr std::array<BundledRingtone, 5> kBundled{{
    {"classic", "ring-classic.wav"},
    {"bell", "ring-bell.wav"},
    {"soft", "ring-soft.wav"},
    {"digital", "ring-digital.wav"},
    {"insistent", "ring-insistent.wav"},
}};

constexpr std::uint16_t kPcm = 1;
constexpr std::uint16_t kALaw = 6;
constexpr std::uint16_t kMuLaw = 7;

[[nodiscard]] std::uint16_t little16(const unsigned char *bytes) noexcept
{
    return static_cast<std::uint16_t>(bytes[0] | (bytes[1] << 8));
}

[[nodiscard]] std::uint32_t little32(const unsigned char *bytes) noexcept
{
    return static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8) |
           (static_cast<std::uint32_t>(bytes[2]) << 16) |
           (static_cast<std::uint32_t>(bytes[3]) << 24);
}

[[nodiscard]] bool take(std::ifstream &file, unsigned char *into, std::streamsize count)
{
    file.read(reinterpret_cast<char *>(into), count);
    return file.gcount() == count;
}

struct WaveFormat {
    std::uint16_t tag{0};
    std::uint16_t channels{0};
    std::uint16_t bits{0};
};

}

std::span<const BundledRingtone> bundledRingtones() noexcept
{
    return kBundled;
}

std::string_view defaultRingtone() noexcept
{
    return kBundled.front().id;
}

std::string_view bundledRingtoneFile(std::string_view id) noexcept
{
    for (const auto &ringtone : kBundled) {
        if (ringtone.id == id) {
            return ringtone.file;
        }
    }
    return {};
}

RingtoneFault inspectRingtoneFile(const std::filesystem::path &file)
{
    if (file.empty()) {
        return RingtoneFault::Missing;
    }

    std::error_code failed;
    const auto status = std::filesystem::status(file, failed);
    if (failed || status.type() == std::filesystem::file_type::not_found) {
        return RingtoneFault::Missing;
    }
    if (status.type() != std::filesystem::file_type::regular) {
        return RingtoneFault::NotAFile;
    }

    std::ifstream stream{file, std::ios::binary};
    if (!stream.is_open()) {
        return RingtoneFault::Unreadable;
    }

    std::array<unsigned char, 12> header{};
    if (!take(stream, header.data(), header.size())) {
        return RingtoneFault::NotAWave;
    }
    if (std::memcmp(header.data(), "RIFF", 4) != 0 ||
        std::memcmp(header.data() + 8, "WAVE", 4) != 0) {
        return RingtoneFault::NotAWave;
    }

    WaveFormat format;
    bool described = false;

    while (true) {
        std::array<unsigned char, 8> chunk{};
        if (!take(stream, chunk.data(), chunk.size())) {

            return RingtoneFault::NotAWave;
        }

        const auto size = little32(chunk.data() + 4);

        if (std::memcmp(chunk.data(), "fmt ", 4) == 0) {
            std::array<unsigned char, 16> body{};
            if (size < body.size() || !take(stream, body.data(), body.size())) {
                return RingtoneFault::NotAWave;
            }
            format.tag = little16(body.data());
            format.channels = little16(body.data() + 2);
            format.bits = little16(body.data() + 14);
            described = true;

            stream.seekg(size - body.size(), std::ios::cur);
        } else if (std::memcmp(chunk.data(), "data", 4) == 0) {
            if (!described) {
                return RingtoneFault::NotAWave;
            }
            if (size == 0) {
                return RingtoneFault::Empty;
            }
            break;
        } else {
            stream.seekg(size, std::ios::cur);
        }

        if ((size % 2) != 0) {
            stream.seekg(1, std::ios::cur);
        }

        if (!stream.good()) {
            return RingtoneFault::NotAWave;
        }
    }

    const bool playable = (format.tag == kPcm && format.bits == 16) ||
                          ((format.tag == kALaw || format.tag == kMuLaw) && format.bits == 8);
    if (!playable || format.channels < 1 || format.channels > 2) {
        return RingtoneFault::Unsupported;
    }

    return RingtoneFault::None;
}

}
