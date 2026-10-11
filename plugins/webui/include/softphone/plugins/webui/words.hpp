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

#include <span>
#include <string_view>
#include <utility>

namespace softphone::plugins::webui {

struct Words {
    std::string_view recordings;
    std::string_view when;
    std::string_view who;
    std::string_view length;
    std::string_view number;
    std::string_view parties;
    std::string_view format;
    std::string_view transcript;

    std::string_view unknownParty;

    std::string_view nothingRecorded;
    std::string_view notTranscribed;
    std::string_view notHere;
    std::string_view noSuchRecording;
    std::string_view notAvailable;

    std::string_view noLibrary;
    std::string_view couldNotRead;

    std::string_view soundGone;

    std::string_view back;

    enum class Date {

        YearFirst,

        DayFirst,
    };
    Date date{Date::YearFirst};

    std::string_view tag;
};

[[nodiscard]] const Words *wordsIn(std::string_view tag);

[[nodiscard]] const Words &wordsFor(std::string_view header, const Words *phone = nullptr);

[[nodiscard]] std::span<const Words> everyLanguage();

}
