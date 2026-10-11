// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/media/mp3.hpp>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#include <minimp3_ex.h>

#include <algorithm>

namespace softphone::media {

std::string_view toString(DecoderStatus status) noexcept
{
    switch (status) {
    case DecoderStatus::Ok:
        return "ok";
    case DecoderStatus::Missing:
        return "missing";
    case DecoderStatus::Malformed:
        return "malformed";
    case DecoderStatus::Failed:
        return "failed";
    }
    return "failed";
}

struct Mp3Decoder::Reader {
    mp3dec_ex_t handle{};
    bool open{false};

    ~Reader()
    {
        if (open) {
            mp3dec_ex_close(&handle);
        }
    }
};

Mp3Decoder::Mp3Decoder() : reader_{std::make_unique<Reader>()} {}

Mp3Decoder::~Mp3Decoder() = default;

DecoderStatus Mp3Decoder::open(const std::filesystem::path &file)
{
    close();

#ifdef _WIN32
    const int failed = mp3dec_ex_open_w(&reader_->handle, file.c_str(), MP3D_SEEK_TO_SAMPLE);
#else
    const int failed = mp3dec_ex_open(&reader_->handle, file.c_str(), MP3D_SEEK_TO_SAMPLE);
#endif
    if (failed != 0) {

        return std::filesystem::exists(file) ? DecoderStatus::Malformed : DecoderStatus::Missing;
    }

    reader_->open = true;

    if (reader_->handle.info.channels <= 0 || reader_->handle.info.hz <= 0) {
        close();
        return DecoderStatus::Malformed;
    }

    info_.channels = static_cast<unsigned>(reader_->handle.info.channels);
    info_.sampleRate = static_cast<unsigned>(reader_->handle.info.hz);

    info_.frames = static_cast<std::uint64_t>(reader_->handle.samples) / info_.channels;

    return DecoderStatus::Ok;
}

std::size_t Mp3Decoder::read(std::span<std::int16_t> out)
{
    if (!reader_->open || out.empty()) {
        return 0;
    }
    return mp3dec_ex_read(&reader_->handle, out.data(), out.size());
}

DecoderStatus Mp3Decoder::seek(std::uint64_t frame)
{
    if (!reader_->open) {
        return DecoderStatus::Failed;
    }
    const int failed = mp3dec_ex_seek(&reader_->handle, frame * info_.channels);
    return failed == 0 ? DecoderStatus::Ok : DecoderStatus::Failed;
}

void Mp3Decoder::close()
{
    if (reader_->open) {
        mp3dec_ex_close(&reader_->handle);
        reader_->open = false;
    }
    reader_->handle = mp3dec_ex_t{};
    info_ = Mp3Info{};
}

bool Mp3Decoder::isOpen() const noexcept
{
    return reader_->open;
}

DecoderStatus decodeMp3(const std::filesystem::path &file, std::vector<std::int16_t> &interleaved,
                        Mp3Info &info)
{
    interleaved.clear();
    info = Mp3Info{};

    Mp3Decoder decoder;
    if (const auto status = decoder.open(file); !succeeded(status)) {
        return status;
    }

    info = decoder.info();
    interleaved.resize(static_cast<std::size_t>(info.frames) * info.channels);

    std::size_t filled = 0;
    while (filled < interleaved.size()) {
        const std::size_t took = decoder.read(std::span<std::int16_t>{interleaved}.subspan(filled));
        if (took == 0) {
            break;
        }
        filled += took;
    }

    interleaved.resize(filled);
    info.frames = info.channels > 0 ? filled / info.channels : 0;

    return DecoderStatus::Ok;
}

DecoderStatus decodeMp3Channel(const std::filesystem::path &file, unsigned channel,
                               std::vector<std::int16_t> &samples, Mp3Info &info)
{
    samples.clear();

    std::vector<std::int16_t> interleaved;
    if (const auto status = decodeMp3(file, interleaved, info); !succeeded(status)) {
        return status;
    }
    if (channel >= info.channels) {
        return DecoderStatus::Failed;
    }

    samples.reserve(static_cast<std::size_t>(info.frames));
    for (std::size_t at = channel; at < interleaved.size(); at += info.channels) {
        samples.push_back(interleaved[at]);
    }
    return DecoderStatus::Ok;
}

}
