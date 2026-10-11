// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "store_commands.hpp"

#include <softphone/core/recording_library.hpp>

#include <softphone/core/store.hpp>
#include <softphone/core/time_text.hpp>

#include <array>
#include <charconv>
#include <chrono>
#include <ctime>
#include <ostream>
#include <vector>

namespace softphone::cli {
namespace {

[[nodiscard]] bool parseId(std::string_view text, std::string &out)
{
    if (text.empty()) {
        return false;
    }
    out = std::string{text};
    return true;
}

[[nodiscard]] int codeFor(core::StoreStatus status)
{
    return status == core::StoreStatus::Ok ? 0 : 2;
}

[[nodiscard]] std::string localTime(std::chrono::system_clock::time_point instant)
{
    const std::tm broken = core::localParts(instant);

    std::array<char, 32> text{};
    const auto written = std::strftime(text.data(), text.size(), "%Y-%m-%d %H:%M", &broken);
    return std::string{text.data(), written};
}

}

namespace {

[[nodiscard]] StoreCommand parseHistory(std::span<const std::string_view> rest)
{
    StoreCommand command;
    command.kind = StoreCommand::Kind::HistoryList;

    if (!rest.empty() &&
        (rest.front() == "--help" || rest.front() == "-h" || rest.front() == "help")) {
        command.kind = StoreCommand::Kind::Help;
        return command;
    }

    if (!rest.empty() && rest.front() == "clear") {
        command.kind = StoreCommand::Kind::HistoryClear;
        if (rest.size() != 1) {
            command.problem = "clear-takes-nothing";
        }
        return command;
    }

    if (!rest.empty() && rest.front() == "forget") {
        command.kind = StoreCommand::Kind::HistoryForget;
        if (rest.size() != 2 || !parseId(rest[1], command.id)) {
            command.problem = "forget-wants-one-identifier";
        }
        return command;
    }

    auto flags = rest;
    if (!flags.empty() && flags.front() == "list") {
        flags = flags.subspan(1);
    }

    for (std::size_t index = 0; index < flags.size(); ++index) {
        const auto flag = flags[index];
        if (flag == "--missed") {
            command.query.missedOnly = true;
        } else if (flag == "--declined") {

            command.query.declinedOnly = true;
        } else if (flag == "--incoming") {
            command.query.direction = core::CallDirection::Incoming;
        } else if (flag == "--outgoing") {
            command.query.direction = core::CallDirection::Outgoing;
        } else if (flag == "--limit" && index + 1 < flags.size()) {
            const auto text = flags[++index];
            int limit = 0;
            const auto *first = text.data();
            const auto parsed = std::from_chars(first, first + text.size(), limit);
            if (parsed.ec != std::errc{} || parsed.ptr != first + text.size() || limit <= 0 ||
                limit > 100000) {
                command.problem = "limit-wants-a-number";
                return command;
            }
            command.query.limit = limit;
        } else if (flag == "--number" && index + 1 < flags.size()) {

            command.query.numberKey = core::numberKey(flags[++index]);
        } else {
            command.problem = "unknown-option";
            return command;
        }
    }

    return command;
}

[[nodiscard]] StoreCommand parseRecordings(std::span<const std::string_view> args)
{
    StoreCommand command;
    command.kind = StoreCommand::Kind::RecordingsList;

    if (!args.empty() &&
        (args.front() == "--help" || args.front() == "-h" || args.front() == "help")) {
        command.kind = StoreCommand::Kind::Help;
        return command;
    }

    if (!args.empty() && args.front() == "forget") {
        command.kind = StoreCommand::Kind::RecordingsForget;
        if (args.size() != 2 || !parseId(args[1], command.id)) {
            command.problem = "forget-wants-one-identifier";
        }
        return command;
    }

    if (!args.empty() && (args.front() == "pin" || args.front() == "unpin")) {
        command.kind = args.front() == "pin" ? StoreCommand::Kind::RecordingsPin
                                             : StoreCommand::Kind::RecordingsUnpin;
        if (args.size() != 2 || !parseId(args[1], command.id)) {
            command.problem = "wants-one-identifier";
        }
        return command;
    }

    if (!args.empty() && args.front() == "sweep") {
        command.kind = StoreCommand::Kind::RecordingsSweep;
        if (args.size() > 1) {
            command.problem = "too-many-arguments";
        }
        return command;
    }

    if (!args.empty() && args.front() == "log") {
        command.kind = StoreCommand::Kind::RecordingsLog;
        if (args.size() == 2 && !parseId(args[1], command.id)) {
            command.problem = "log-wants-one-identifier";
        } else if (args.size() > 2) {
            command.problem = "too-many-arguments";
        }
        return command;
    }

    const auto flags = !args.empty() && args.front() == "list" ? args.subspan(1) : args;

    for (std::size_t index = 0; index < flags.size(); ++index) {
        const auto flag = flags[index];
        if (flag == "--deleted") {
            command.recordings.includeDeleted = true;
        } else if (flag == "--number" && index + 1 < flags.size()) {
            command.recordings.numberKey = core::numberKey(flags[++index]);
        } else if (flag == "--call" && index + 1 < flags.size()) {
            command.recordings.callId = std::string{flags[++index]};
        } else if (flag == "--limit" && index + 1 < flags.size()) {
            const auto text = flags[++index];
            int limit = 0;
            const auto *first = text.data();
            const auto parsed = std::from_chars(first, first + text.size(), limit);
            if (parsed.ec != std::errc{} || parsed.ptr != first + text.size() || limit <= 0 ||
                limit > 100000) {
                command.problem = "limit-wants-a-number";
                return command;
            }
            command.recordings.limit = static_cast<std::size_t>(limit);
        } else {
            command.problem = "unknown-option";
            return command;
        }
    }

    return command;
}

}

StoreCommand parseStoreCommand(std::span<const std::string_view> args)
{
    StoreCommand command;
    if (args.empty()) {
        return command;
    }
    if (args.front() == "history") {
        return parseHistory(args.subspan(1));
    }
    if (args.front() == "recordings") {
        return parseRecordings(args.subspan(1));
    }
    if (args.front() != "contacts") {
        return command;
    }

    const auto rest = args.subspan(1);
    if (rest.empty() || rest.front() == "list") {
        command.kind = StoreCommand::Kind::ContactsList;
        return command;
    }

    if (rest.front() == "--help" || rest.front() == "-h" || rest.front() == "help") {
        command.kind = StoreCommand::Kind::Help;
        return command;
    }

    if (rest.front() == "add") {
        command.kind = StoreCommand::Kind::ContactsAdd;
        if (rest.size() < 3) {
            command.problem = "add-wants-a-name-and-a-number";
            return command;
        }
        command.name = std::string{rest[1]};
        command.number = std::string{rest[2]};
        if (rest.size() > 3) {

            command.accountAddress = std::string{rest[3]};
        }
        if (rest.size() > 4) {
            command.problem = "too-many-arguments";
        }
        return command;
    }

    if (rest.front() == "remove") {
        command.kind = StoreCommand::Kind::ContactsRemove;
        if (rest.size() != 2 || !parseId(rest[1], command.id)) {
            command.problem = "remove-wants-one-identifier";
        }
        return command;
    }

    command.kind = StoreCommand::Kind::ContactsList;
    command.problem = "unknown-command";
    return command;
}

void printStoreUsage(std::ostream &out)
{
    out << "Usage: ai-softphone-cli contacts [list]\n"
           "       ai-softphone-cli contacts add <name> <number> [user@domain]\n"
           "       ai-softphone-cli contacts remove <id>\n"
           "       ai-softphone-cli history [list] [--missed] [--declined] "
           "[--incoming|--outgoing]\n"
           "                             [--number <number>] [--limit <n>]\n"
           "       ai-softphone-cli history forget <id>\n"
           "       ai-softphone-cli history clear\n"
           "       ai-softphone-cli recordings [list] [--number <number>]\n"
           "                                   [--call <id>] [--limit <n>] [--deleted]\n"
           "       ai-softphone-cli recordings forget <id>\n"
           "       ai-softphone-cli recordings log [<id>]\n"
           "       ai-softphone-cli recordings pin <id>\n"
           "       ai-softphone-cli recordings unpin <id>\n"
           "       ai-softphone-cli recordings sweep\n\n"
           "The address book and the call log, from a terminal. The third\n"
           "argument to contacts add is the line to call the person on, written\n"
           "as an address; without it the phone dials from whichever line it is\n"
           "dialling from.\n\n"
           "A history row is: id, when it started, which way it went, what\n"
           "became of it, how long the conversation lasted, the number, the\n"
           "name, the line, and — outgoing — what was actually dialled.\n\n"
           "Nothing prunes the history. `clear` is the only forgetting there\n"
           "is, and it is deliberate rather than automatic.\n\n"
           "A recordings row is: id, when it started, which way it went, how\n"
           "long it is, the number, the name, the file, and a mark for gaps.\n"
           "`forget` deletes the audio and every stem with it and leaves a\n"
           "row saying so — nothing of the conversation survives, and the\n"
           "deletion is still something a server can be told about. `log`\n"
           "prints what has been done to a recording, which the specification\n"
           "asks for and which a deletion is itself an entry in.\n\n"
           "`pin` exempts a recording from every retention rule; it still\n"
           "counts towards the size limit, because it is still on the disk.\n"
           "`sweep` applies the rules from the settings file now rather than\n"
           "waiting for the phone to do it, and says what it took.\n\n"
           "Exit codes: 0 done, 1 usage, 2 the store would not answer.\n";
}

int runStoreCommand(const StoreCommand &command, core::IContactStore &contacts,
                    core::ICallLog &history, core::IRecordingStore &recordings,
                    const std::filesystem::path &library, std::ostream &out, std::ostream &error)
{
    if (!command.valid()) {
        error << "ai-softphone-cli: " << command.problem << "\n\n";
        printStoreUsage(error);
        return 1;
    }

    switch (command.kind) {
    case StoreCommand::Kind::None:
        return 1;

    case StoreCommand::Kind::Help:
        printStoreUsage(out);
        return 0;

    case StoreCommand::Kind::ContactsList: {
        std::vector<core::Contact> found;
        if (const auto status = contacts.contacts(found); status != core::StoreStatus::Ok) {
            error << "ai-softphone-cli: contacts unavailable: " << toString(status) << "\n";
            return codeFor(status);
        }
        for (const auto &contact : found) {
            out << contact.id << '\t' << contact.name << '\t' << contact.number;
            if (!contact.accountAddress.empty()) {
                out << '\t' << contact.accountAddress;
            }
            out << '\n';
        }
        out << found.size() << " contact" << (found.size() == 1 ? "" : "s") << '\n';
        return 0;
    }

    case StoreCommand::Kind::ContactsAdd: {
        core::Contact contact;
        contact.name = command.name;
        contact.number = command.number;
        contact.accountAddress = command.accountAddress;

        if (const auto problem = validate(contact); problem != core::ContactProblem::None) {
            error << "ai-softphone-cli: " << toString(problem) << "\n";
            return 1;
        }

        if (const auto status = contacts.save(contact); status != core::StoreStatus::Ok) {
            error << "ai-softphone-cli: not saved: " << toString(status) << "\n";
            return codeFor(status);
        }

        out << contact.id << '\t' << contact.name << '\t' << contact.number << '\n';
        return 0;
    }

    case StoreCommand::Kind::ContactsRemove: {
        const auto status = contacts.removeContact(command.id);
        if (status == core::StoreStatus::Missing) {
            error << "ai-softphone-cli: no contact " << command.id << "\n";
            return 1;
        }
        if (status != core::StoreStatus::Ok) {
            error << "ai-softphone-cli: not removed: " << core::toString(status) << "\n";
            return codeFor(status);
        }
        out << "removed " << command.id << '\n';
        return 0;
    }

    case StoreCommand::Kind::HistoryList: {
        std::vector<core::CallRecord> found;
        if (const auto status = history.calls(command.query, found);
            status != core::StoreStatus::Ok) {
            error << "ai-softphone-cli: history unavailable: " << toString(status) << "\n";
            return codeFor(status);
        }
        for (const auto &row : found) {
            out << row.id << '\t' << localTime(row.startedAt) << '\t' << toString(row.direction)
                << '\t' << toString(outcome(row)) << '\t' << row.duration.count() << "s\t"
                << row.number << '\t' << row.name << '\t' << row.accountAddress;
            if (!row.dialedNumber.empty() && row.dialedNumber != row.number) {

                out << "\tdialled " << row.dialedNumber;
            }
            out << '\n';
        }
        out << found.size() << " call" << (found.size() == 1 ? "" : "s") << '\n';
        return 0;
    }

    case StoreCommand::Kind::HistoryForget: {
        const auto status = history.removeCall(command.id);
        if (status == core::StoreStatus::Missing) {
            error << "ai-softphone-cli: no call " << command.id << "\n";
            return 1;
        }
        if (status != core::StoreStatus::Ok) {
            error << "ai-softphone-cli: not removed: " << core::toString(status) << "\n";
            return codeFor(status);
        }
        out << "forgot " << command.id << '\n';
        return 0;
    }

    case StoreCommand::Kind::RecordingsList: {
        std::vector<core::Recording> found;
        if (const auto status = recordings.recordings(command.recordings, found);
            status != core::StoreStatus::Ok) {
            error << "ai-softphone-cli: recordings unavailable: " << toString(status) << "\n";
            return codeFor(status);
        }
        for (const auto &row : found) {
            out << row.id << '\t' << localTime(row.startedAt) << '\t' << toString(row.direction)
                << '\t' << (row.duration.count() / 1000) << "s\t" << row.bytes << "B\t"
                << row.remoteNumber << '\t' << row.remoteName << '\t'
                << (row.isDeleted() ? std::string{"(deleted)"} : row.filePath);
            if (row.hasGaps) {
                out << "\tgaps";
            }
            if (!row.stems.empty()) {
                out << '\t' << row.stems.size() << " stems";
            }
            out << '\n';
        }
        out << found.size() << " recording" << (found.size() == 1 ? "" : "s") << '\n';
        return 0;
    }

    case StoreCommand::Kind::RecordingsForget: {
        std::vector<std::string> files;
        const auto status = recordings.removeRecording(command.id, files);
        if (status == core::StoreStatus::Missing) {
            error << "ai-softphone-cli: no recording " << command.id << "\n";
            return 1;
        }
        if (status != core::StoreStatus::Ok) {
            error << "ai-softphone-cli: not removed: " << core::toString(status) << "\n";
            return codeFor(status);
        }

        int taken = 0;
        for (const auto &relative : files) {
            std::error_code failed;
            if (std::filesystem::remove(library / relative, failed)) {
                ++taken;
            } else if (failed) {
                error << "ai-softphone-cli: " << (library / relative).string() << ": "
                      << failed.message() << "\n";
            }
        }

        out << "forgot " << command.id << ", " << taken << " file" << (taken == 1 ? "" : "s")
            << " deleted\n";
        return 0;
    }

    case StoreCommand::Kind::RecordingsPin:
    case StoreCommand::Kind::RecordingsUnpin: {
        core::Recording row;
        const auto found = recordings.recording(command.id, row);
        if (found == core::StoreStatus::Missing) {
            error << "ai-softphone-cli: no recording " << command.id << "\n";
            return 1;
        }
        if (found != core::StoreStatus::Ok) {
            error << "ai-softphone-cli: recordings unavailable: " << toString(found) << "\n";
            return codeFor(found);
        }

        row.pinned = command.kind == StoreCommand::Kind::RecordingsPin;
        if (const auto status = recordings.save(row); status != core::StoreStatus::Ok) {
            error << "ai-softphone-cli: not saved: " << toString(status) << "\n";
            return codeFor(status);
        }

        out << (row.pinned ? "pinned " : "unpinned ") << command.id << '\n';
        return 0;
    }

    case StoreCommand::Kind::RecordingsSweep: {
        core::RecordingLibrary shelf{&recordings, library};
        shelf.setPolicy(command.retention);

        const auto report = shelf.sweep();
        if (report.status != core::StoreStatus::Ok) {
            error << "ai-softphone-cli: the sweep did not finish: " << toString(report.status)
                  << "\n";
            return codeFor(report.status);
        }

        out << "took " << report.removed << " recording" << (report.removed == 1 ? "" : "s")
            << ", dropped stems from " << report.stemsDropped << ", deleted " << report.filesDeleted
            << " file" << (report.filesDeleted == 1 ? "" : "s") << '\n';
        out << report.bytesBefore / (1024 * 1024) << " MB before, "
            << report.bytesAfter / (1024 * 1024) << " MB after\n";

        if (report.filesLeft > 0) {

            error << "ai-softphone-cli: " << report.filesLeft << " file(s) could not be deleted\n";
        }
        if (report.pinnedOverTheCeiling) {
            error << "ai-softphone-cli: over the size limit, and what is left is pinned\n";
        }
        return 0;
    }

    case StoreCommand::Kind::RecordingsLog: {
        std::vector<core::AccessEntry> log;
        if (const auto status = recordings.accessLog(command.id, log);
            status != core::StoreStatus::Ok) {
            error << "ai-softphone-cli: access log unavailable: " << toString(status) << "\n";
            return codeFor(status);
        }
        for (const auto &entry : log) {
            out << localTime(entry.at) << '\t' << toString(entry.what) << '\t' << entry.recordingId
                << '\t' << entry.detail << '\n';
        }
        out << log.size() << " entr" << (log.size() == 1 ? "y" : "ies") << '\n';
        return 0;
    }

    case StoreCommand::Kind::HistoryClear: {
        const auto status = history.clearCalls();
        if (status != core::StoreStatus::Ok) {
            error << "ai-softphone-cli: not cleared: " << toString(status) << "\n";
            return codeFor(status);
        }
        out << "history cleared\n";
        return 0;
    }
    }

    return 1;
}

}
