// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/recordings.hpp>

namespace softphone::core {

std::string_view toString(RecordingSource source) noexcept
{
    switch (source) {
    case RecordingSource::Native:
        return "native";
    case RecordingSource::External:
        return "external";
    case RecordingSource::Imported:
        return "imported";
    }
    return "native";
}

std::optional<RecordingSource> parseRecordingSource(std::string_view text) noexcept
{
    if (text == "native") {
        return RecordingSource::Native;
    }
    if (text == "external") {
        return RecordingSource::External;
    }
    if (text == "imported") {
        return RecordingSource::Imported;
    }
    return std::nullopt;
}

std::string_view toString(ConsentPosture posture) noexcept
{
    switch (posture) {
    case ConsentPosture::Announce:
        return "announce";
    case ConsentPosture::Tone:
        return "tone";
    case ConsentPosture::Confirm:
        return "confirm";
    case ConsentPosture::Silent:
        return "silent";
    }
    return "announce";
}

std::optional<ConsentPosture> parseConsentPosture(std::string_view text) noexcept
{
    if (text == "announce") {
        return ConsentPosture::Announce;
    }
    if (text == "tone") {
        return ConsentPosture::Tone;
    }
    if (text == "confirm") {
        return ConsentPosture::Confirm;
    }
    if (text == "silent") {
        return ConsentPosture::Silent;
    }
    return std::nullopt;
}

std::string_view toString(RecordingAccess what) noexcept
{
    switch (what) {
    case RecordingAccess::Created:
        return "created";
    case RecordingAccess::Played:
        return "played";
    case RecordingAccess::Exported:
        return "exported";
    case RecordingAccess::Deleted:
        return "deleted";
    case RecordingAccess::Expired:
        return "expired";
    }
    return "played";
}

void keepWhatAPersonChanged(const Recording &stored, Recording &writing)
{
    writing.title = stored.title;
    writing.pinned = stored.pinned;

    if (writing.source != RecordingSource::Native) {
        writing.remoteName = stored.remoteName;
    }
}

}
