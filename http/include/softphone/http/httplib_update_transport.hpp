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

#include <softphone/core/updates.hpp>

#include <chrono>
#include <filesystem>

namespace softphone::http {

class HttplibUpdateTransport final : public core::IUpdateTransport {
public:

    static constexpr auto kDefaultTimeout = std::chrono::seconds{10};

    static constexpr std::size_t kManifestCeiling = 256 * 1024;

    explicit HttplibUpdateTransport(std::chrono::seconds timeout = kDefaultTimeout,
                                    std::filesystem::path caBundle = {});
    ~HttplibUpdateTransport() override;

    core::Fetched fetch(const std::string &url) override;

    bool download(const std::string &url, const std::filesystem::path &into,
                  const std::function<bool(std::uint64_t, std::uint64_t)> &progress,
                  std::string &complaint) override;

private:
    std::chrono::seconds timeout_;

    std::filesystem::path caBundle_;
};

}
