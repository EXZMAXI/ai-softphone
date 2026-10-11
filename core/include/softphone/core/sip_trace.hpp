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

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

enum class SipDirection {

    Incoming,

    Outgoing,
};

[[nodiscard]] std::string_view toString(SipDirection direction) noexcept;

[[nodiscard]] std::string headerInMessage(std::string_view message, std::string_view name);

struct SipMessage {

    std::uint64_t sequence{0};

    std::chrono::system_clock::time_point at{};
    SipDirection direction{SipDirection::Incoming};

    std::string transport;

    std::string localAddress;
    std::string remoteAddress;

    std::string summary;

    std::string callId;

    std::string text;
};

class ISipTraceSink {
public:
    ISipTraceSink() = default;
    ISipTraceSink(const ISipTraceSink &) = delete;
    ISipTraceSink &operator=(const ISipTraceSink &) = delete;
    ISipTraceSink(ISipTraceSink &&) = delete;
    ISipTraceSink &operator=(ISipTraceSink &&) = delete;
    virtual ~ISipTraceSink() = default;

    virtual void onSipMessage(SipMessage message) = 0;
};

class SipTrace final : public ISipTraceSink {
public:
    static constexpr std::size_t kDefaultMessages = 500;
    static constexpr std::size_t kDefaultBytes = 4 * 1024 * 1024;

    explicit SipTrace(std::size_t maxMessages = kDefaultMessages,
                      std::size_t maxBytes = kDefaultBytes);

    void onSipMessage(SipMessage message) override;

    [[nodiscard]] std::vector<SipMessage> messages() const;

    [[nodiscard]] std::size_t size() const;

    [[nodiscard]] std::size_t dropped() const;

    void clear();

private:
    void trim();

    mutable std::mutex mutex_;
    std::uint64_t nextSequence_{1};
    std::vector<SipMessage> messages_;
    std::size_t maxMessages_;
    std::size_t maxBytes_;
    std::size_t bytes_{0};
    std::size_t dropped_{0};
};

}
