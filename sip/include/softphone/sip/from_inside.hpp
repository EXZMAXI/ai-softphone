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
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace softphone::sip {

void makePjlibReady();

struct AudioDeviceFacts {
    std::string name;
    unsigned inputs{0};
    unsigned outputs{0};
};

inline constexpr int kDefaultCapture = -1;
inline constexpr int kDefaultPlayback = -2;

[[nodiscard]] std::vector<AudioDeviceFacts> devicesInTheStack();

[[nodiscard]] std::optional<AudioDeviceFacts> deviceInTheStack(int id);

[[nodiscard]] std::optional<std::pair<int, int>> devicesTheStackIsPointedAt();

struct BytesMoved {
    long captured{0};
    long played{0};
};

[[nodiscard]] std::optional<BytesMoved> bytesThrough(int recordId, int playId,
                                                     std::chrono::milliseconds howLong);

}
