// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#pragma once

#include <softphone/core/audio_encoder.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace softphone::media {

class Mp3Encoder final : public core::IAudioEncoder {
public:
    Mp3Encoder();
    ~Mp3Encoder() override;

    [[nodiscard]] core::EncoderStatus open(const std::filesystem::path &file,
                                           const core::EncoderSettings &settings);

    [[nodiscard]] const std::filesystem::path &file() const noexcept
    {
        return file_;
    }

    [[nodiscard]] core::EncoderStatus write(std::span<const std::int16_t> left,
                                            std::span<const std::int16_t> right) override;

    [[nodiscard]] core::EncoderStatus finish() override;

    [[nodiscard]] std::uint64_t bytes() const noexcept override
    {
        return bytes_;
    }

private:
    struct Codec;
    std::unique_ptr<Codec> codec_;

    std::filesystem::path file_;
    std::uint64_t bytes_{0};

    std::vector<unsigned char> scratch_;

    std::vector<std::int16_t> silence_;

    bool finished_{false};
};

enum class DecoderStatus {
    Ok,

    Missing,

    Malformed,
    Failed,
};

[[nodiscard]] std::string_view toString(DecoderStatus status) noexcept;

[[nodiscard]] constexpr bool succeeded(DecoderStatus status) noexcept
{
    return status == DecoderStatus::Ok;
}

struct Mp3Info {
    unsigned sampleRate{0};
    unsigned channels{0};

    std::uint64_t frames{0};
};

class Mp3Decoder {
public:
    Mp3Decoder();
    Mp3Decoder(const Mp3Decoder &) = delete;
    Mp3Decoder &operator=(const Mp3Decoder &) = delete;
    Mp3Decoder(Mp3Decoder &&) = delete;
    Mp3Decoder &operator=(Mp3Decoder &&) = delete;
    ~Mp3Decoder();

    [[nodiscard]] DecoderStatus open(const std::filesystem::path &file);

    [[nodiscard]] const Mp3Info &info() const noexcept
    {
        return info_;
    }

    [[nodiscard]] std::size_t read(std::span<std::int16_t> out);

    [[nodiscard]] DecoderStatus seek(std::uint64_t frame);

    void close();

    [[nodiscard]] bool isOpen() const noexcept;

private:
    struct Reader;
    std::unique_ptr<Reader> reader_;
    Mp3Info info_;
};

[[nodiscard]] DecoderStatus decodeMp3(const std::filesystem::path &file,
                                      std::vector<std::int16_t> &interleaved, Mp3Info &info);

[[nodiscard]] DecoderStatus decodeMp3Channel(const std::filesystem::path &file, unsigned channel,
                                             std::vector<std::int16_t> &samples, Mp3Info &info);

}
