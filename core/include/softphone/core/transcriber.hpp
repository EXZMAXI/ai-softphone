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

#include <softphone/core/asr.hpp>
#include <softphone/core/asr_plan.hpp>
#include <softphone/core/call_log.hpp>
#include <softphone/core/transcripts.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace softphone::core {

class Transcriber {
public:
    struct Request {
        std::string recordingId;

        AsrSubject subject;

        std::string providerId;
        AsrEndpoint endpoint;
        AsrOptions options;
    };

    struct Outcome {
        Transcript transcript;

        AsrStatus status{AsrStatus::Ok};
        std::string error;

        bool retry{false};

        struct Kept {

            std::string voice;

            std::int64_t offsetMs{0};

            std::string body;
        };

        std::vector<Kept> raw;
    };

    Transcriber(IAsrProvider &provider, AudioPreparer prepare,
                WallClock clock = defaultWallClock());

    Transcriber(const Transcriber &) = delete;
    Transcriber &operator=(const Transcriber &) = delete;
    Transcriber(Transcriber &&) = delete;
    Transcriber &operator=(Transcriber &&) = delete;
    ~Transcriber() = default;

    [[nodiscard]] Outcome run(const Request &request, const AsrCancel &cancelled);

private:
    IAsrProvider &provider_;
    AudioPreparer prepare_;
    WallClock clock_;
};

}
