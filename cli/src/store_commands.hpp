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

#include <softphone/core/call_log.hpp>
#include <softphone/core/contacts.hpp>
#include <softphone/core/recordings.hpp>
#include <softphone/core/retention.hpp>

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <span>
#include <string>
#include <string_view>

namespace softphone::cli {

struct StoreCommand {
    enum class Kind {

        None,
        Help,
        ContactsList,
        ContactsAdd,
        ContactsRemove,
        HistoryList,
        HistoryForget,
        HistoryClear,
        RecordingsList,
        RecordingsForget,
        RecordingsLog,
        RecordingsPin,
        RecordingsUnpin,
        RecordingsSweep,
    };

    Kind kind{Kind::None};

    std::string name;
    std::string number;
    std::string accountAddress;

    std::string id;

    core::CallLogQuery query;

    core::RecordingQuery recordings;

    core::RetentionPolicy retention;

    std::string problem;

    [[nodiscard]] bool wanted() const noexcept
    {
        return kind != Kind::None;
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return problem.empty();
    }
};

[[nodiscard]] StoreCommand parseStoreCommand(std::span<const std::string_view> args);

int runStoreCommand(const StoreCommand &command, core::IContactStore &contacts,
                    core::ICallLog &history, core::IRecordingStore &recordings,
                    const std::filesystem::path &library, std::ostream &out, std::ostream &error);

void printStoreUsage(std::ostream &out);

}
