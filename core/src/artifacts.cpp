// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/artifacts.hpp>

#include <nlohmann/json.hpp>

#include <optional>

namespace softphone::core {

std::string_view toString(ArtifactStatus status) noexcept
{
    switch (status) {
    case ArtifactStatus::Queued:
        return "queued";
    case ArtifactStatus::Running:
        return "running";
    case ArtifactStatus::Done:
        return "done";
    case ArtifactStatus::Failed:
        return "failed";
    }
    return "queued";
}

std::optional<ArtifactStatus> parseArtifactStatus(std::string_view text) noexcept
{
    if (text == "queued") {
        return ArtifactStatus::Queued;
    }
    if (text == "running") {
        return ArtifactStatus::Running;
    }
    if (text == "done") {
        return ArtifactStatus::Done;
    }
    if (text == "failed") {
        return ArtifactStatus::Failed;
    }
    return std::nullopt;
}

std::optional<Artifact> asRealtimeSuggestions(const std::string &recordingId,
                                              const std::string &transcriptId,
                                              const std::vector<PromptedAnswer> &answers,
                                              const PromptTemplate &prompt)
{
    if (answers.empty()) {

        return std::nullopt;
    }

    Artifact artifact;
    artifact.recordingId = recordingId;
    artifact.transcriptId = transcriptId;

    artifact.templateId = prompt.id;
    artifact.templateCode = prompt.code;
    artifact.templateName = prompt.name;
    artifact.templateVersion = prompt.version;

    artifact.kind = ArtifactKind::Realtime;

    artifact.shape = AnswerShape::Moments;
    artifact.status = ArtifactStatus::Done;
    artifact.language = prompt.language;

    nlohmann::json moments = nlohmann::json::array();
    for (const auto &answer : answers) {
        if (answer.text.empty()) {
            continue;
        }
        if (!artifact.content.empty()) {
            artifact.content += "\n\n";
        }
        artifact.content += "[" + std::to_string(answer.at.count()) + " ms] " + answer.text;

        nlohmann::json one;
        one["at_ms"] = answer.at.count();
        one["text"] = answer.text;
        moments.push_back(std::move(one));
    }
    if (moments.empty()) {

        return std::nullopt;
    }
    artifact.structured = nlohmann::json{{"moments", std::move(moments)}}.dump(2);

    artifact.turnFrom = static_cast<int>(answers.front().from);
    artifact.turnTo = static_cast<int>(answers.back().to);

    const auto now = std::chrono::system_clock::now();
    artifact.requestedAt = now;
    artifact.completedAt = now;
    return artifact;
}

}
