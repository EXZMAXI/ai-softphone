// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/recording_library.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>
#include <softphone/core/recording_controller.hpp>

#include <softphone/core/uuid.hpp>

#include <algorithm>
#include <chrono>
#include <set>
#include <string>
#include <system_error>
#include <vector>

namespace softphone::core {

RecordingLibrary::RecordingLibrary(IRecordingStore *store, std::filesystem::path root)
    : store_{store}, root_{std::move(root)}
{}

std::filesystem::path RecordingLibrary::fileFor(const Recording &recording) const
{
    return recording.filePath.empty() ? std::filesystem::path{} : root_ / recording.filePath;
}

bool RecordingLibrary::unlink(const std::string &relative)
{
    if (relative.empty()) {
        return false;
    }

    std::error_code failed;
    if (std::filesystem::remove(root_ / relative, failed)) {
        return true;
    }

    if (failed) {
        Logger::instance().log(LogLevel::Warning, "recording",
                               "could not delete " + asUtf8(root_ / relative) + ": " +
                                   failed.message());
        return false;
    }

    return false;
}

RecordingLibrary::Report RecordingLibrary::sweep(std::chrono::system_clock::time_point now)
{
    if (store_ == nullptr) {
        Report nothing;
        nothing.status = StoreStatus::Unavailable;
        return nothing;
    }

    Report report;

    std::vector<Recording> everything;
    report.status = store_->recordings({}, everything);
    if (!succeeded(report.status)) {
        return report;
    }

    std::erase_if(everything, [](const Recording &row) { return row.unfinished; });

    const auto verdict = sweepRecordings(everything, policy_, now);
    report.bytesBefore = verdict.bytes;
    report.bytesAfter = verdict.bytesAfter;
    report.pinnedOverTheCeiling = verdict.pinnedOverTheCeiling;

    for (const std::string &id : verdict.dropAudio) {

        std::vector<std::string> files;
        if (const auto status = store_->dropAudio(id, files); !succeeded(status)) {
            report.status = status;
            continue;
        }

        for (const std::string &file : files) {
            if (unlink(file)) {
                ++report.filesDeleted;
            } else {
                ++report.filesLeft;
            }
        }

        static_cast<void>(store_->noteAccess(id, RecordingAccess::Expired, "retention"));
        ++report.removed;
    }

    for (const std::string &id : verdict.dropStems) {

        if (wanted_ && wanted_(id)) {
            ++report.stemsKept;
            continue;
        }

        std::vector<std::string> files;
        if (const auto status = store_->dropStems(id, files); !succeeded(status)) {
            report.status = status;
            continue;
        }
        if (files.empty()) {
            continue;
        }

        for (const std::string &file : files) {
            if (unlink(file)) {
                ++report.filesDeleted;
            } else {
                ++report.filesLeft;
            }
        }

        static_cast<void>(store_->noteAccess(id, RecordingAccess::Expired, "stems"));
        ++report.stemsDropped;
    }

    if (report.removed > 0 || report.stemsDropped > 0) {
        Logger::instance().log(LogLevel::Info, "recording",
                               "retention took " + std::to_string(report.removed) +
                                   " recording(s) and " + std::to_string(report.stemsDropped) +
                                   " set(s) of stems");
    }
    if (report.pinnedOverTheCeiling) {

        Logger::instance().log(LogLevel::Warning, "recording",
                               "the recordings are over the size limit and what is left is "
                               "pinned; nothing more can be taken");
    }

    return report;
}

RecordingLibrary::Report RecordingLibrary::finish()
{
    Report report;

    if (store_ == nullptr) {
        report.status = StoreStatus::Unavailable;
        return report;
    }

    std::vector<Recording> everything;
    RecordingQuery query;

    query.includeDeleted = false;
    report.status = store_->recordings(query, everything);
    if (!succeeded(report.status)) {
        return report;
    }

    for (Recording &row : everything) {
        if (!row.unfinished) {
            continue;
        }

        if (!probe_) {
            continue;
        }

        const auto file = root_ / row.filePath;
        std::error_code failed;
        const auto facts = row.filePath.empty() ? FileFacts{} : probe_(file);

        if (!facts.readable || facts.duration.count() == 0) {

            std::vector<std::string> orphaned;
            if (const auto status = store_->removeRecording(row.id, orphaned); succeeded(status)) {
                ++report.abandoned;
            } else {
                report.status = status;
            }
            continue;
        }

        row.duration = facts.duration;
        if (facts.sampleRate > 0) {
            row.sampleRate = facts.sampleRate;
        }
        if (facts.channels > 0) {
            row.channels = facts.channels;
            row.channelLayout = facts.channels > 1 ? "near-far" : "mixed";
        }
        row.bytes = static_cast<std::uint64_t>(std::filesystem::file_size(file, failed));
        if (failed) {
            row.bytes = 0;
        }

        row.hasGaps = true;
        row.unfinished = false;

        if (const auto status = store_->save(row); !succeeded(status)) {
            report.status = status;
            continue;
        }
        ++report.finished;
    }

    if (report.finished > 0 || report.abandoned > 0) {
        Logger::instance().log(LogLevel::Info, "recording",
                               "closed " + std::to_string(report.finished) +
                                   " recording(s) a previous run left open, and dropped " +
                                   std::to_string(report.abandoned) + " with nothing in them");
    }

    return report;
}

RecordingLibrary::Report RecordingLibrary::recover()
{
    Report report;

    if (store_ == nullptr) {
        report.status = StoreStatus::Unavailable;
        return report;
    }

    if (!probe_) {
        return report;
    }

    std::vector<Recording> everything;
    core::RecordingQuery query;

    query.includeDeleted = true;
    report.status = store_->recordings(query, everything);
    if (!succeeded(report.status)) {
        return report;
    }

    std::set<std::string> known;
    std::set<std::string> identifiers;
    for (const Recording &recording : everything) {
        if (!recording.filePath.empty()) {
            known.insert(recording.filePath);
        }
        for (const RecordingStem &stem : recording.stems) {
            if (!stem.filePath.empty()) {
                known.insert(stem.filePath);
            }
        }

        identifiers.insert(recording.id);
    }

    const auto claimed = [&](const std::filesystem::path &file) {
        return identifiers.count(file.stem().string()) > 0 ||
               identifiers.count(file.parent_path().filename().string()) > 0;
    };

    std::error_code failed;
    if (!std::filesystem::is_directory(root_, failed)) {
        return report;
    }

    for (auto entry =
             std::filesystem::recursive_directory_iterator{
                 root_, std::filesystem::directory_options::skip_permission_denied, failed};
         entry != std::filesystem::recursive_directory_iterator{}; entry.increment(failed)) {
        if (failed) {
            break;
        }

        if (entry->is_directory(failed) && entry->path().filename() == kScratch) {
            entry.disable_recursion_pending();
            continue;
        }

        if (!entry->is_regular_file(failed) || entry->path().extension() != ".mp3") {
            continue;
        }

        const auto relative =
            std::filesystem::relative(entry->path(), root_, failed).generic_string();
        if (failed || relative.empty() || known.count(relative) > 0 || claimed(entry->path())) {
            continue;
        }

        const auto facts = probe_(entry->path());
        if (!facts.readable || facts.duration.count() == 0) {

            continue;
        }

        Recording found;
        found.id = makeUuid();
        found.source = RecordingSource::Native;
        found.filePath = relative;
        found.duration = facts.duration;
        found.sampleRate = facts.sampleRate;
        found.channels = facts.channels;
        found.channelLayout = facts.channels > 1 ? "near-far" : "mixed";
        found.bytes = static_cast<std::uint64_t>(entry->file_size(failed));

        const auto written = std::filesystem::last_write_time(entry->path(), failed);
        if (!failed) {

#ifdef _MSC_VER
            const auto asSystem = std::chrono::clock_cast<std::chrono::system_clock>(written);
#else
            const auto asSystem = std::chrono::file_clock::to_sys(written);
#endif
            const auto when =
                std::chrono::time_point_cast<std::chrono::system_clock::duration>(asSystem);
            found.startedAt = when - facts.duration;
        } else {
            found.startedAt = std::chrono::system_clock::now();
        }

        found.hasGaps = true;

        if (const auto status = store_->save(found); !succeeded(status)) {
            report.status = status;
            continue;
        }

        static_cast<void>(store_->noteAccess(found.id, RecordingAccess::Created, "recovered"));
        ++report.recovered;
    }

    if (report.recovered > 0) {
        Logger::instance().log(LogLevel::Info, "recording",
                               "found " + std::to_string(report.recovered) +
                                   " recording(s) on the disk that no row knew about");
    }

    return report;
}

RecordingLibrary::Brought RecordingLibrary::bring(const std::filesystem::path &file,
                                                  std::chrono::system_clock::time_point now)
{
    using Outcome = Brought::Outcome;

    Brought answer;

    if (store_ == nullptr || !probe_) {
        answer.outcome = Outcome::NoLibrary;
        return answer;
    }

    std::error_code failed;
    if (!std::filesystem::is_regular_file(file, failed) || failed) {
        answer.outcome = Outcome::NotThere;
        return answer;
    }

    if (!hasRoom()) {
        answer.outcome = Outcome::NoRoom;
        return answer;
    }

    const auto facts = probe_(file);
    if (!facts.readable || facts.duration.count() == 0) {

        answer.outcome = Outcome::NotAudio;
        return answer;
    }

    Recording brought;
    brought.id = makeUuid();
    brought.source = RecordingSource::Imported;

    brought.startedAt = now;
    brought.duration = facts.duration;
    brought.sampleRate = facts.sampleRate;
    brought.channels = facts.channels;

    brought.channelLayout = "mixed";

    brought.consent = ConsentPosture::Silent;

    brought.remoteName = asUtf8(file.stem());

    brought.filePath = RecordingController::relativePath(brought.id, now);
    const auto destination = root_ / brought.filePath;
    const auto where = asUtf8(destination);

    std::filesystem::create_directories(destination.parent_path(), failed);
    if (failed) {
        const LogAttribute fields[] = {{"path", where}};
        Logger::instance().log(LogLevel::Warning, "recordings",
                               "the imported conversation had nowhere to go", fields);
        answer.outcome = Outcome::NotWritten;
        return answer;
    }

    const auto converted = convert_ && convert_(file, destination);
    if (!converted) {
        std::filesystem::copy_file(file, destination,
                                   std::filesystem::copy_options::overwrite_existing, failed);
        if (failed) {
            const LogAttribute fields[] = {{"path", where}};
            Logger::instance().log(LogLevel::Warning, "recordings",
                                   "the imported conversation could not be copied in", fields);
            answer.outcome = Outcome::NotWritten;
            return answer;
        }
    }

    if (!std::filesystem::exists(destination, failed)) {
        const LogAttribute fields[] = {{"path", where}};
        Logger::instance().log(LogLevel::Warning, "recordings",
                               "the imported conversation was converted to nothing", fields);
        answer.outcome = Outcome::NotWritten;
        return answer;
    }

    const auto size = std::filesystem::file_size(destination, failed);
    brought.bytes = failed ? 0 : static_cast<std::uint64_t>(size);

    if (const auto saved = store_->save(brought); !succeeded(saved)) {

        std::error_code ignored;
        std::filesystem::remove(destination, ignored);

        const LogAttribute fields[] = {{"path", where}, {"status", toString(saved)}};
        Logger::instance().log(LogLevel::Warning, "recordings",
                               "the imported conversation could not be written down", fields);
        answer.outcome = Outcome::NotWritten;
        return answer;
    }

    {
        const LogAttribute fields[] = {{"recording", brought.id}, {"name", brought.remoteName}};
        Logger::instance().log(LogLevel::Info, "recordings", "a conversation was imported", fields);
    }

    answer.recording = std::move(brought);
    return answer;
}

StoreStatus RecordingLibrary::remove(const std::string &id)
{
    if (store_ == nullptr) {
        return StoreStatus::Unavailable;
    }

    std::vector<std::string> files;
    const auto status = store_->removeRecording(id, files);
    if (!succeeded(status)) {
        return status;
    }

    for (const std::string &file : files) {
        static_cast<void>(unlink(file));
    }
    return StoreStatus::Ok;
}

std::uint64_t RecordingLibrary::freeBytes() const
{
    std::error_code failed;

    auto where = root_;
    while (!where.empty() && !std::filesystem::exists(where, failed)) {
        const auto above = where.parent_path();
        if (above == where) {
            break;
        }
        where = above;
    }

    const auto room = std::filesystem::space(where, failed);
    if (failed) {

        return 0;
    }
    return room.available;
}

bool RecordingLibrary::hasRoom() const
{
    const auto floor =
        static_cast<std::uint64_t>(std::max(0, policy_.freeSpaceFloorMegabytes)) * 1024 * 1024;
    return freeBytes() > floor;
}

}
