// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/sip_trace.hpp>

#include <algorithm>
#include <cctype>

#include <utility>

namespace softphone::core {

std::string_view toString(SipDirection direction) noexcept
{
    switch (direction) {
    case SipDirection::Incoming:
        return "in";
    case SipDirection::Outgoing:
        return "out";
    }
    return "unknown";
}

SipTrace::SipTrace(std::size_t maxMessages, std::size_t maxBytes)
    : maxMessages_{maxMessages}, maxBytes_{maxBytes}
{}

void SipTrace::onSipMessage(SipMessage message)
{
    const std::lock_guard lock{mutex_};

    message.sequence = nextSequence_++;

    bytes_ += message.text.size();
    messages_.push_back(std::move(message));
    trim();
}

void SipTrace::trim()
{

    while (!messages_.empty() && (messages_.size() > maxMessages_ || bytes_ > maxBytes_)) {
        bytes_ -= messages_.front().text.size();
        ++dropped_;
        messages_.erase(messages_.begin());
    }
}

std::vector<SipMessage> SipTrace::messages() const
{
    const std::lock_guard lock{mutex_};
    return messages_;
}

std::size_t SipTrace::size() const
{
    const std::lock_guard lock{mutex_};
    return messages_.size();
}

std::size_t SipTrace::dropped() const
{
    const std::lock_guard lock{mutex_};
    return dropped_;
}

void SipTrace::clear()
{
    const std::lock_guard lock{mutex_};
    messages_.clear();
    bytes_ = 0;
    dropped_ = 0;

}

std::string headerInMessage(std::string_view message, std::string_view name)
{
    std::size_t at = 0;
    while (at < message.size()) {
        auto end = message.find('\n', at);
        if (end == std::string_view::npos) {
            end = message.size();
        }
        auto line = message.substr(at, end - at);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        if (line.empty()) {

            return {};
        }
        if (line.size() > name.size() &&
            std::equal(name.begin(), name.end(), line.begin(), [](char want, char had) {
                return want == static_cast<char>(std::tolower(static_cast<unsigned char>(had)));
            })) {
            auto value = line.substr(name.size());
            while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
                value.remove_prefix(1);
            }
            while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
                value.remove_suffix(1);
            }
            return std::string{value};
        }
        at = end + 1;
    }
    return {};
}

}
