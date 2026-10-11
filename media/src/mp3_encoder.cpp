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

#include <lame.h>

#include <algorithm>
#include <cstdio>
#include <system_error>

namespace softphone::media {
namespace {

[[nodiscard]] std::size_t roomFor(std::size_t frames) noexcept
{
    return (frames * 5) / 4 + 7200;
}

}

struct Mp3Encoder::Codec {
    lame_global_flags *lame{nullptr};
    std::FILE *out{nullptr};

    ~Codec()
    {
        if (lame != nullptr) {
            lame_close(lame);
        }
        if (out != nullptr) {
            static_cast<void>(std::fclose(out));
        }
    }
};

Mp3Encoder::Mp3Encoder() = default;

Mp3Encoder::~Mp3Encoder() = default;

core::EncoderStatus Mp3Encoder::open(const std::filesystem::path &file,
                                     const core::EncoderSettings &settings)
{
    if (settings.channels < 1 || settings.channels > 2 || settings.sampleRate == 0) {
        return core::EncoderStatus::Failed;
    }

    auto codec = std::make_unique<Codec>();

    if (file.has_parent_path() && !file.parent_path().empty()) {
        std::error_code failed;
        std::filesystem::create_directories(file.parent_path(), failed);
        if (failed && !std::filesystem::is_directory(file.parent_path())) {
            return core::EncoderStatus::Unwritable;
        }
    }

#ifdef _WIN32
    codec->out = _wfopen(file.c_str(), L"w+b");
#else
    codec->out = std::fopen(file.c_str(), "w+b");
#endif
    if (codec->out == nullptr) {
        return core::EncoderStatus::Unwritable;
    }

    codec->lame = lame_init();
    if (codec->lame == nullptr) {
        return core::EncoderStatus::Failed;
    }

    static_cast<void>(lame_set_in_samplerate(codec->lame, static_cast<int>(settings.sampleRate)));
    static_cast<void>(lame_set_out_samplerate(codec->lame, static_cast<int>(settings.sampleRate)));
    static_cast<void>(lame_set_num_channels(codec->lame, static_cast<int>(settings.channels)));

    static_cast<void>(lame_set_mode(codec->lame, settings.channels == 1 ? MONO : DUAL_CHANNEL));

    static_cast<void>(lame_set_VBR(codec->lame, vbr_abr));
    static_cast<void>(lame_set_VBR_mean_bitrate_kbps(
        codec->lame, static_cast<int>(settings.bitratePerChannel * settings.channels)));

    static_cast<void>(lame_set_quality(codec->lame, 3));

    if (lame_init_params(codec->lame) < 0) {
        return core::EncoderStatus::Failed;
    }

    codec_ = std::move(codec);
    file_ = file;
    bytes_ = 0;
    finished_ = false;
    return core::EncoderStatus::Ok;
}

core::EncoderStatus Mp3Encoder::write(std::span<const std::int16_t> left,
                                      std::span<const std::int16_t> right)
{
    if (!codec_ || finished_) {
        return core::EncoderStatus::Failed;
    }
    if (left.empty()) {
        return core::EncoderStatus::Ok;
    }

    if (right.size() < left.size()) {
        silence_.assign(left.size(), 0);
        right = std::span<const std::int16_t>{silence_};
    }

    scratch_.resize(roomFor(left.size()));

    const int written = lame_encode_buffer(
        codec_->lame, const_cast<short *>(left.data()), const_cast<short *>(right.data()),
        static_cast<int>(left.size()), scratch_.data(), static_cast<int>(scratch_.size()));
    if (written < 0) {
        return core::EncoderStatus::Failed;
    }
    if (written == 0) {
        return core::EncoderStatus::Ok;
    }

    const std::size_t put =
        std::fwrite(scratch_.data(), 1, static_cast<std::size_t>(written), codec_->out);
    if (put != static_cast<std::size_t>(written)) {
        return core::EncoderStatus::Unwritable;
    }

    bytes_ += put;
    return core::EncoderStatus::Ok;
}

core::EncoderStatus Mp3Encoder::finish()
{
    if (!codec_) {
        return core::EncoderStatus::Failed;
    }
    if (finished_) {
        return core::EncoderStatus::Ok;
    }
    finished_ = true;

    core::EncoderStatus status = core::EncoderStatus::Ok;

    scratch_.resize(roomFor(0));
    const int written =
        lame_encode_flush(codec_->lame, scratch_.data(), static_cast<int>(scratch_.size()));
    if (written > 0) {
        const std::size_t put =
            std::fwrite(scratch_.data(), 1, static_cast<std::size_t>(written), codec_->out);
        if (put != static_cast<std::size_t>(written)) {
            status = core::EncoderStatus::Unwritable;
        }
        bytes_ += put;
    } else if (written < 0) {
        status = core::EncoderStatus::Failed;
    }

    if (std::fflush(codec_->out) != 0) {
        status = core::EncoderStatus::Unwritable;
    } else {
        lame_mp3_tags_fid(codec_->lame, codec_->out);
    }

    if (std::fclose(codec_->out) != 0 && status == core::EncoderStatus::Ok) {
        status = core::EncoderStatus::Unwritable;
    }
    codec_->out = nullptr;

    std::error_code failed;
    const auto size = std::filesystem::file_size(file_, failed);
    if (!failed) {
        bytes_ = size;
    }

    return status;
}

}
