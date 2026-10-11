// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "jobs_view_model.hpp"

#include <QVariantMap>

#include <algorithm>
#include <utility>

namespace softphone::ui {

JobsViewModel::JobsViewModel(QObject *parent) : QObject{parent} {}

QVariantList JobsViewModel::rows() const
{
    QVariantList out;
    out.reserve(static_cast<qsizetype>(rows_.size()));
    for (const auto &entry : rows_) {
        QVariantMap row;
        row["id"] = entry.id;
        row["recordingId"] = entry.recordingId;
        row["party"] = entry.party;
        row["startedAtMs"] = entry.startedAtMs;
        row["state"] =
            QString::fromUtf8(core::toString(entry.state).data(),
                              static_cast<qsizetype>(core::toString(entry.state).size()));
        row["kind"] = QString::fromUtf8(core::toString(entry.kind).data(),
                                        static_cast<qsizetype>(core::toString(entry.kind).size()));
        row["kindLabel"] = entry.kind == core::JobKind::Transcribe

                               ? tr("transcribing")
                               : tr("asking a model");
        row["running"] = entry.state == core::JobState::Running;
        row["failed"] = entry.state == core::JobState::Failed;
        row["attempts"] = static_cast<int>(entry.attempts);
        row["error"] = entry.error;

        row["sinceMs"] = entry.sinceMs;
        out.push_back(row);
    }
    return out;
}

QVariantList JobsViewModel::rowsNow() const
{
    return rows();
}

QString JobsViewModel::summary() const
{

    const auto reading = tr("%n transcription(s) under way", nullptr, transcribing_);

    const auto asking = tr("%n answer(s) under way", nullptr, generating_);

    const auto gave = tr("%n failed", nullptr, failed_);

    const auto both = [](const QString &left, const QString &right) {
        return tr("%1 · %2").arg(left, right);
    };

    QString said;
    if (transcribing_ > 0) {
        said = reading;
    }
    if (generating_ > 0) {
        said = said.isEmpty() ? asking : both(said, asking);
    }
    if (failed_ > 0) {
        said = said.isEmpty() ? gave : both(said, gave);
    }
    return said;
}

void JobsViewModel::cancel(const QString &jobId)
{
    if (cancel_) {
        cancel_(jobId);
    }
}

void JobsViewModel::dismiss(const QString &jobId)
{
    if (dismiss_) {
        dismiss_(jobId);
    }
}

void JobsViewModel::dismissAll()
{
    if (!dismiss_) {
        return;
    }

    std::vector<QString> going;
    for (const auto &entry : rows_) {
        if (entry.state == core::JobState::Failed) {
            going.push_back(entry.id);
        }
    }
    for (const auto &id : going) {
        dismiss_(id);
    }
}

bool JobsViewModel::transcribing(const QString &recordingId) const
{
    if (recordingId.isEmpty()) {
        return false;
    }
    for (const auto &entry : rows_) {
        if (entry.kind == core::JobKind::Transcribe && entry.recordingId == recordingId &&
            !core::finished(entry.state)) {
            return true;
        }
    }
    return false;
}

void JobsViewModel::setJobs(std::vector<Entry> rows)
{
    rows_ = std::move(rows);

    working_ = 0;
    failed_ = 0;
    transcribing_ = 0;
    generating_ = 0;
    workingSince_ = 0;
    for (const auto &entry : rows_) {
        if (entry.state == core::JobState::Failed) {
            ++failed_;
        } else if (!core::finished(entry.state)) {
            ++working_;
            if (entry.kind == core::JobKind::Transcribe) {
                ++transcribing_;
            } else {
                ++generating_;
            }

            if (entry.queuedAtMs > 0 && (workingSince_ == 0 || entry.queuedAtMs < workingSince_)) {
                workingSince_ = entry.queuedAtMs;
            }
        }
    }

    Q_EMIT rowsChanged();
}

void JobsViewModel::setActions(Action cancel, Action dismiss, Plain refresh)
{
    cancel_ = std::move(cancel);
    dismiss_ = std::move(dismiss);
    refresh_ = std::move(refresh);
}

}
