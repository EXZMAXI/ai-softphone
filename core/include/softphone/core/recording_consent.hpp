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
#include <functional>
#include <string>
#include <vector>

namespace softphone::core {

enum class NoticeProgress {

    Began,

    Given,

    Failed,
};

class IConsentVoice {
public:
    IConsentVoice() = default;
    IConsentVoice(const IConsentVoice &) = delete;
    IConsentVoice &operator=(const IConsentVoice &) = delete;
    IConsentVoice(IConsentVoice &&) = delete;
    IConsentVoice &operator=(IConsentVoice &&) = delete;
    virtual ~IConsentVoice() = default;

    virtual void announceTo(const std::string &noticeId, const std::vector<std::string> &callIds,
                            const std::vector<std::string> &files, bool intoTheRecording,
                            std::function<void(NoticeProgress)> report) = 0;

    virtual void beepEvery(const std::string &noticeId, const std::vector<std::string> &callIds,
                           const std::string &file, std::chrono::seconds interval,
                           bool intoTheRecording) = 0;

    virtual void stopAnnouncing(const std::string &noticeId) = 0;
};

}
