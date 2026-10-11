// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "transcript_view_model.hpp"

#include "row_marks.hpp"

#include <QVariantMap>

#include <algorithm>
#include <chrono>
#include <utility>
#include <vector>

namespace softphone::ui {
namespace {

void splitWords(const core::TranscriptSegment &segment, QStringList &words,
                std::vector<std::pair<qint64, qint64>> &times)
{
    words.reserve(static_cast<qsizetype>(segment.words.size()));
    times.reserve(segment.words.size());
    for (const auto &word : segment.words) {
        words.push_back(QString::fromStdString(word.text));
        times.emplace_back(word.start.count(), word.end.count());
    }
}

void placeWords(const QString &text, const QStringList &words, QList<int> &at, QList<int> &to)
{
    at.reserve(words.size());
    to.reserve(words.size());

    qsizetype from = 0;
    for (const auto &word : words) {
        const auto wanted = word.trimmed();
        const auto found = wanted.isEmpty() ? qsizetype{-1} : text.indexOf(wanted, from);
        if (found < 0) {
            at.push_back(-1);
            to.push_back(-1);
            continue;
        }
        const auto ends = found + wanted.size();
        at.push_back(static_cast<int>(found));
        to.push_back(static_cast<int>(ends));
        from = ends;
    }
}

[[nodiscard]] QString trimmed(const std::string &text)
{
    return QString::fromStdString(text).trimmed();
}

}

TranscriptViewModel::TranscriptViewModel(QObject *parent) : QObject{parent} {}

QVariantList TranscriptViewModel::lines() const
{
    QVariantList rows;
    rows.reserve(static_cast<qsizetype>(shown_.segments.size()));
    for (const auto &line : shown_.segments) {
        QVariantMap row;
        row["ordinal"] = line.ordinal;
        row["atMs"] = line.startMs;
        row["endMs"] = line.endMs;
        row["voice"] = line.voice;
        row["who"] = line.who;
        row["speaker"] = line.speaker;
        row["text"] = line.text;

        row["words"] = line.words;
        row["wordAt"] = QVariant::fromValue(line.wordAt);
        row["wordTo"] = QVariant::fromValue(line.wordTo);
        row["near"] = line.voice == QStringLiteral("near");
        rows.push_back(row);
    }
    return rows;
}

QVariantList TranscriptViewModel::linesNow() const
{
    return lines();
}

QVariantList TranscriptViewModel::choices() const
{
    QVariantList rows;
    rows.reserve(static_cast<qsizetype>(choices_.size()));
    for (const auto &choice : choices_) {
        QVariantMap row;
        row["id"] = choice.id;
        row["label"] = choice.label;
        row["primary"] = choice.primary;
        row["failed"] = choice.failed;
        rows.push_back(row);
    }
    return rows;
}

QString TranscriptViewModel::provenance() const
{
    if (shown_.id.isEmpty()) {
        return {};
    }
    if (shown_.model.isEmpty()) {
        return shown_.provider;
    }

    return tr("%1 · %2").arg(shown_.provider, shown_.model);
}

qint64 TranscriptViewModel::startOf(int line) const
{
    if (line < 0 || line >= static_cast<int>(shown_.segments.size())) {
        return 0;
    }
    return shown_.segments[static_cast<std::size_t>(line)].startMs;
}

void TranscriptViewModel::choose(const QString &transcriptId)
{
    if (transcriptId == shown_.id) {
        return;
    }
    rebuild(transcriptId);
    if (choose_) {
        choose_(transcriptId);
    }
}

void TranscriptViewModel::makePrimary()
{
    if (makePrimary_ && !shown_.id.isEmpty() && !shown_.primary) {
        makePrimary_();
    }
}

void TranscriptViewModel::transcribe()
{
    transcribeWith({});
}

void TranscriptViewModel::transcribeWith(const QString &providerId)
{
    if (transcribe_) {
        transcribe_(providerId);
    }
}

void TranscriptViewModel::forget()
{
    if (forget_ && !shown_.id.isEmpty()) {
        forget_();
    }
}

QString TranscriptViewModel::exportName(int format) const
{
    const auto stem = exportStem_.isEmpty() ? QStringLiteral("transcript") : exportStem_;
    if (format == kRawFormat) {
        return stem + QStringLiteral(".json");
    }
    const auto which = static_cast<core::TranscriptFormat>(format);
    return stem + QString::fromUtf8(core::extensionFor(which).data(),
                                    static_cast<qsizetype>(core::extensionFor(which).size()));
}

void TranscriptViewModel::exportTo(const QUrl &target, int format)
{
    if (shown_.id.isEmpty() || !target.isValid()) {
        return;
    }

    if (format == kRawFormat) {
        if (copyRaw_ && !shown_.rawPath.isEmpty()) {
            copyRaw_(shown_.rawPath, target.toLocalFile());
        }
        return;
    }

    if (!save_) {
        return;
    }
    const auto found = std::find_if(all_.begin(), all_.end(), [this](const core::Transcript &one) {
        return QString::fromStdString(one.id) == shown_.id;
    });
    if (found == all_.end()) {
        return;
    }
    const auto text =
        core::renderTranscript(*found, static_cast<core::TranscriptFormat>(format), names());
    save_(target.toLocalFile(), QString::fromStdString(text));
}

QString TranscriptViewModel::openAsText() const
{
    if (shown_.id.isEmpty()) {
        return {};
    }
    const auto found = std::find_if(all_.begin(), all_.end(), [this](const core::Transcript &one) {
        return QString::fromStdString(one.id) == shown_.id;
    });
    if (found == all_.end()) {
        return {};
    }
    return QString::fromStdString(
        core::renderTranscript(*found, core::TranscriptFormat::Text, names()));
}

void TranscriptViewModel::refuse(Refusal why)
{
    QString said;
    switch (why) {
    case Refusal::None:
        break;
    case Refusal::NoProvider:

        said = tr("No recogniser is set up, or the one chosen is switched off. Settings → "
                  "Transcription.");
        break;
    case Refusal::NoAudio:

        said = tr("The sound of this conversation is no longer here, so there is nothing to send.");
        break;
    case Refusal::Failed:

        said = tr("This could not be queued. The recordings database may be unwritable.");
        break;
    }

    if (said == notice_) {
        return;
    }
    notice_ = said;
    Q_EMIT noticeChanged();
}

void TranscriptViewModel::setProviderNames(std::vector<std::pair<QString, QString>> names)
{
    if (names == providerNames_) {
        return;
    }
    providerNames_ = std::move(names);
    if (!all_.empty()) {

        setTranscripts(all_, shown_.id);
    }
}

core::TranscriptNames TranscriptViewModel::names() const
{
    core::TranscriptNames names;

    names.nearSide = tr("You").toStdString();
    names.farSide =
        party_.isEmpty()

            ? tr("Them").toStdString()
            : party_.toStdString();

    names.mixed = tr("Everybody").toStdString();
    for (const auto &[id, name] : stemNames_) {
        names.stems.emplace_back(id.toStdString(), name.toStdString());
    }
    return names;
}

QString TranscriptViewModel::labelFor(const QString &voice) const
{
    return QString::fromStdString(names().forVoice(voice.toStdString()));
}

void TranscriptViewModel::setTranscripts(std::vector<core::Transcript> all, const QString &show)
{
    if (!all.empty() && !notice_.isEmpty()) {

        notice_.clear();
        Q_EMIT noticeChanged();
    }
    all_ = std::move(all);

    choices_.clear();
    choices_.reserve(all_.size());

    const auto languageOf = [](const core::Transcript &one) {
        return one.detectedLanguage.empty() ? one.requestedLanguage : one.detectedLanguage;
    };

    std::vector<QString> base;
    std::vector<QString> live;
    std::vector<QString> models;
    std::vector<QString> languages;
    std::vector<QString> moments;
    base.reserve(all_.size());

    for (auto one = all_.rbegin(); one != all_.rend(); ++one) {
        Choice choice;
        choice.id = QString::fromStdString(one->id);
        choice.primary = one->isPrimary;
        choice.failed = one->status == core::TranscriptStatus::Failed;

        base.push_back(providerNameOf(one->providerId, one->providerType, providerNames_));

        live.push_back(one->source == "realtime" ? tr("live") : QString{});
        models.push_back(QString::fromStdString(one->model));
        languages.push_back(QString::fromStdString(languageOf(*one)));

        moments.push_back(whenMark(one->requestedAt));

        choices_.push_back(std::move(choice));
    }

    const auto labels = toldApart(base, {live, models, languages, moments});
    for (std::size_t at = 0; at < choices_.size() && at < labels.size(); ++at) {
        choices_[at].label = labels[at];
    }

    auto wanted = show;
    if (wanted.isEmpty()) {

        const auto record = std::find_if(all_.begin(), all_.end(),
                                         [](const core::Transcript &one) { return one.isPrimary; });
        if (record != all_.end()) {
            wanted = QString::fromStdString(record->id);
        } else if (!all_.empty()) {
            wanted = QString::fromStdString(all_.front().id);
        }
    }

    rebuild(wanted);
    Q_EMIT choicesChanged();
}

void TranscriptViewModel::rebuild(const QString &show)
{
    shown_ = Shown{};
    karaoke_ = false;

    const auto found = std::find_if(all_.begin(), all_.end(), [&show](const core::Transcript &one) {
        return QString::fromStdString(one.id) == show;
    });
    if (found == all_.end()) {
        currentLine_ = -1;
        currentWord_ = -1;
        Q_EMIT linesChanged();
        Q_EMIT playheadChanged();
        return;
    }

    shown_.id = QString::fromStdString(found->id);
    shown_.primary = found->isPrimary;

    shown_.provider = providerNameOf(found->providerId, found->providerType, providerNames_);
    shown_.model = QString::fromStdString(found->model);
    shown_.language = QString::fromStdString(
        found->detectedLanguage.empty() ? found->requestedLanguage : found->detectedLanguage);
    shown_.error = QString::fromStdString(found->error);
    shown_.rawPath = QString::fromStdString(found->rawPath);
    shown_.status = found->status;

    shown_.segments.reserve(found->segments.size());
    for (const auto &segment : found->segments) {
        Line line;
        line.ordinal = segment.ordinal;
        line.startMs = segment.start.count();
        line.endMs = segment.end.count();
        line.voice = QString::fromStdString(segment.voice);
        line.who = labelFor(line.voice);
        line.speaker = QString::fromStdString(segment.speaker);
        line.text = trimmed(segment.editedText.empty() ? segment.text : segment.editedText);
        splitWords(segment, line.words, line.wordTimes);
        placeWords(line.text, line.words, line.wordAt, line.wordTo);
        karaoke_ = karaoke_ || !line.words.isEmpty();
        shown_.segments.push_back(std::move(line));
    }

    locate();
    Q_EMIT linesChanged();
}

void TranscriptViewModel::clear()
{
    if (!notice_.isEmpty()) {
        notice_.clear();
        Q_EMIT noticeChanged();
    }
    all_.clear();
    choices_.clear();
    shown_ = Shown{};
    positionMs_ = 0;
    currentLine_ = -1;
    currentWord_ = -1;
    karaoke_ = false;
    Q_EMIT linesChanged();
    Q_EMIT choicesChanged();
    Q_EMIT playheadChanged();
}

void TranscriptViewModel::setSubject(const QString &exportStem, const QString &party,
                                     std::vector<std::pair<QString, QString>> stemNames)
{
    exportStem_ = exportStem;
    party_ = party;
    stemNames_ = std::move(stemNames);
    if (!shown_.id.isEmpty()) {
        rebuild(shown_.id);
    }
}

void TranscriptViewModel::setPosition(qint64 positionMs)
{
    if (positionMs_ == positionMs) {
        return;
    }
    positionMs_ = positionMs;

    const auto wasLine = currentLine_;
    const auto wasWord = currentWord_;
    locate();
    if (currentLine_ != wasLine || currentWord_ != wasWord) {
        Q_EMIT playheadChanged();
    }
}

void TranscriptViewModel::locate()
{
    currentLine_ = -1;
    currentWord_ = -1;

    for (std::size_t at = 0; at < shown_.segments.size(); ++at) {
        if (shown_.segments[at].startMs > positionMs_) {
            break;
        }
        currentLine_ = static_cast<int>(at);
    }
    if (currentLine_ < 0) {
        return;
    }

    const auto &line = shown_.segments[static_cast<std::size_t>(currentLine_)];
    if (positionMs_ > line.endMs) {

        return;
    }
    for (std::size_t at = 0; at < line.wordTimes.size(); ++at) {
        const auto &[from, to] = line.wordTimes[at];
        if (positionMs_ >= from && positionMs_ < to) {
            currentWord_ = static_cast<int>(at);
            return;
        }
    }
}

void TranscriptViewModel::setWorking(bool working)
{
    if (working_ == working) {
        return;
    }
    working_ = working;
    Q_EMIT workingChanged();
}

void TranscriptViewModel::setActions(Ask transcribe, Choose choose, Plain makePrimary, Plain forget,
                                     Save save, CopyRaw copyRaw)
{
    transcribe_ = std::move(transcribe);
    choose_ = std::move(choose);
    makePrimary_ = std::move(makePrimary);
    forget_ = std::move(forget);
    save_ = std::move(save);
    copyRaw_ = std::move(copyRaw);
}

}
