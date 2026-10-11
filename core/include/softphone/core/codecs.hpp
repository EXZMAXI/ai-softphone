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

#include <softphone/core/settings.hpp>

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

struct CodecInfo {

    std::string id;

    std::string name;

    unsigned clockRate{0};

    unsigned channels{1};

    [[nodiscard]] bool wideband() const noexcept
    {
        return clockRate > 8000;
    }

    friend bool operator==(const CodecInfo &, const CodecInfo &) = default;
};

[[nodiscard]] CodecInfo describeCodec(std::string_view id);

[[nodiscard]] std::vector<CodecChoice> planCodecs(std::span<const CodecChoice> chosen,
                                                  std::span<const CodecInfo> available);

[[nodiscard]] bool offeredByDefault(const CodecInfo &codec);

class ICodecBackend {
public:
    ICodecBackend() = default;
    ICodecBackend(const ICodecBackend &) = delete;
    ICodecBackend &operator=(const ICodecBackend &) = delete;
    ICodecBackend(ICodecBackend &&) = delete;
    ICodecBackend &operator=(ICodecBackend &&) = delete;
    virtual ~ICodecBackend() = default;

    [[nodiscard]] virtual std::vector<CodecInfo> codecs() = 0;

    virtual void applyCodecs(std::span<const CodecChoice> wanted) = 0;
};

}
