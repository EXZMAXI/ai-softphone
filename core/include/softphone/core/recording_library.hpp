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

#include <softphone/core/recordings.hpp>
#include <softphone/core/retention.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace softphone::core {

inline constexpr std::string_view kScratch = "scratch";

class RecordingLibrary {
public:

    struct Report {
        std::size_t removed{0};
        std::size_t stemsDropped{0};

        std::size_t stemsKept{0};

        std::size_t recovered{0};

        std::size_t finished{0};

        std::size_t abandoned{0};

        std::size_t filesDeleted{0};

        std::size_t filesLeft{0};

        std::uint64_t bytesBefore{0};
        std::uint64_t bytesAfter{0};
        bool pinnedOverTheCeiling{false};

        StoreStatus status{StoreStatus::Ok};
    };

    RecordingLibrary(IRecordingStore *store, std::filesystem::path root);

    RecordingLibrary(const RecordingLibrary &) = delete;
    RecordingLibrary &operator=(const RecordingLibrary &) = delete;
    RecordingLibrary(RecordingLibrary &&) = delete;
    RecordingLibrary &operator=(RecordingLibrary &&) = delete;
    ~RecordingLibrary() = default;

    void setPolicy(const RetentionPolicy &policy) noexcept
    {
        policy_ = policy;
    }

    [[nodiscard]] const RetentionPolicy &policy() const noexcept
    {
        return policy_;
    }

    [[nodiscard]] const std::filesystem::path &root() const noexcept
    {
        return root_;
    }

    Report sweep(std::chrono::system_clock::time_point now = std::chrono::system_clock::now());

    struct FileFacts {
        std::chrono::milliseconds duration{0};
        unsigned sampleRate{0};
        unsigned channels{0};
        bool readable{false};
    };

    using Prober = std::function<FileFacts(const std::filesystem::path &)>;

    void setProber(Prober probe)
    {
        probe_ = std::move(probe);
    }

    using Converter =
        std::function<bool(const std::filesystem::path &from, const std::filesystem::path &to)>;

    void setConverter(Converter convert)
    {
        convert_ = std::move(convert);
    }

    using StillWanted = std::function<bool(const std::string &recordingId)>;

    void setStillWanted(StillWanted wanted)
    {
        wanted_ = std::move(wanted);
    }

    Report recover();

    Report finish();

    struct Brought {

        enum class Outcome {

            Brought,

            NotThere,

            NotAudio,

            NoRoom,

            NotWritten,

            NoLibrary,
        };

        Recording recording;

        Outcome outcome{Outcome::Brought};

        [[nodiscard]] bool arrived() const noexcept
        {
            return outcome == Outcome::Brought;
        }
    };

    [[nodiscard]] Brought
    bring(const std::filesystem::path &file,
          std::chrono::system_clock::time_point now = std::chrono::system_clock::now());

    [[nodiscard]] StoreStatus remove(const std::string &id);

    [[nodiscard]] std::filesystem::path fileFor(const Recording &recording) const;

    [[nodiscard]] std::uint64_t freeBytes() const;

    [[nodiscard]] bool hasRoom() const;

private:

    [[nodiscard]] bool unlink(const std::string &relative);

    IRecordingStore *store_;
    std::filesystem::path root_;
    RetentionPolicy policy_;
    Prober probe_;
    Converter convert_;
    StillWanted wanted_;
};

}
