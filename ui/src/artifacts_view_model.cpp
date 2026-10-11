// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "artifacts_view_model.hpp"

#include "row_marks.hpp"

#include <softphone/core/exchange_log.hpp>

#include <QCoreApplication>
#include <QDateTime>
#include <QLocale>
#include <QStringList>

#include <algorithm>
#include <chrono>
#include <utility>

namespace softphone::ui {
namespace {

constexpr const char *kOrder[] = {
    "summary",  "super_summary", "action_items", "score", "categorise", "topics",
    "entities", "red_flags",     "translation",  "qa",    "custom",
};

[[nodiscard]] int rankOf(const std::string &kind)
{
    int at = 0;
    for (const auto *known : kOrder) {
        if (kind == known) {
            return at;
        }
        ++at;
    }
    return at;
}

[[nodiscard]] QString kindText(core::ArtifactKind kind)
{
    const auto text = core::toString(kind);
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

[[nodiscard]] std::vector<QString>
markedNames(const std::vector<core::Artifact> &rows,
            const std::vector<std::pair<QString, QString>> &providerNames)
{
    std::vector<QString> base;
    std::vector<QString> languages;
    std::vector<QString> entries;
    std::vector<QString> models;
    std::vector<QString> moments;
    base.reserve(rows.size());
    for (const auto &row : rows) {
        base.push_back(ArtifactsViewModel::nameOf(row));
        languages.push_back(QString::fromStdString(row.language));
        entries.push_back(providerNameOf(row.providerId, row.providerType, providerNames));
        models.push_back(QString::fromStdString(row.model));
        moments.push_back(whenMark(row.requestedAt));
    }
    return toldApart(base, {languages, entries, models, moments});
}

}

ArtifactsViewModel::ArtifactsViewModel(QObject *parent) : QObject{parent} {}

QString ArtifactsViewModel::labelForKind(const QString &kind)
{

    struct Named {
        const char *kind;
        const char *label;
    };

    static const Named kNames[] = {

        {"summary", QT_TRANSLATE_NOOP("ArtifactsViewModel", "Summary")},

        {"super_summary", QT_TRANSLATE_NOOP("ArtifactsViewModel", "In a nutshell")},

        {"action_items", QT_TRANSLATE_NOOP("ArtifactsViewModel", "Actions")},

        {"score", QT_TRANSLATE_NOOP("ArtifactsViewModel", "Quality")},

        {"categorise", QT_TRANSLATE_NOOP("ArtifactsViewModel", "Category")},

        {"topics", QT_TRANSLATE_NOOP("ArtifactsViewModel", "Topics")},

        {"entities", QT_TRANSLATE_NOOP("ArtifactsViewModel", "Mentioned")},

        {"tags", QT_TRANSLATE_NOOP("ArtifactsViewModel", "Tags")},

        {"red_flags", QT_TRANSLATE_NOOP("ArtifactsViewModel", "Red flags")},

        {"translation", QT_TRANSLATE_NOOP("ArtifactsViewModel", "Translation")},

        {"qa", QT_TRANSLATE_NOOP("ArtifactsViewModel", "Question")},

        {"realtime", QT_TRANSLATE_NOOP("ArtifactsViewModel", "Suggestions")},
    };

    for (const auto &named : kNames) {
        if (kind == QLatin1String{named.kind}) {
            return QCoreApplication::translate("ArtifactsViewModel", named.label);
        }
    }

    return QCoreApplication::translate("ArtifactsViewModel", "Other");
}

QVariantList ArtifactsViewModel::tabs() const
{

    std::vector<const core::Artifact *> shown;
    shown.reserve(rows_.size());
    for (const auto &row : rows_) {
        shown.push_back(&row);
    }

    std::stable_sort(shown.begin(), shown.end(),
                     [](const core::Artifact *a, const core::Artifact *b) {
                         if (a->requestedAt != b->requestedAt) {
                             return a->requestedAt < b->requestedAt;
                         }
                         return rankOf(std::string{core::toString(a->kind)}) <
                                rankOf(std::string{core::toString(b->kind)});
                     });

    QVariantList out;
    for (const auto *row : shown) {
        QVariantMap entry;
        entry["id"] = QString::fromStdString(row->id);
        entry["kind"] = kindText(row->kind);
        entry["label"] = labelOf(*row);
        entry["working"] = row->status == core::ArtifactStatus::Running ||
                           row->status == core::ArtifactStatus::Queued;
        entry["failed"] = row->status == core::ArtifactStatus::Failed;
        out.push_back(entry);
    }
    return out;
}

QString ArtifactsViewModel::nameOf(const core::Artifact &row)
{

    if (!row.question.empty()) {
        return QString::fromStdString(row.question);
    }

    if ((row.kind == core::ArtifactKind::Custom || row.kind == core::ArtifactKind::Realtime) &&
        !row.templateName.empty()) {
        return QString::fromStdString(row.templateName);
    }
    return labelForKind(kindText(row.kind));
}

void ArtifactsViewModel::setProviderNames(std::vector<std::pair<QString, QString>> names)
{
    if (names == providerNames_) {
        return;
    }
    providerNames_ = std::move(names);

    if (!rows_.empty()) {
        Q_EMIT rowsChanged();
    }
}

QString ArtifactsViewModel::labelOf(const core::Artifact &row) const
{

    const auto labels = markedNames(rows_, providerNames_);
    for (std::size_t at = 0; at < rows_.size() && at < labels.size(); ++at) {
        if (rows_[at].id == row.id) {
            return labels[at];
        }
    }
    return nameOf(row);
}

QVariantList ArtifactsViewModel::tabsNow() const
{
    return tabs();
}

const core::Artifact *ArtifactsViewModel::current() const
{
    if (openId_.isEmpty()) {
        return nullptr;
    }
    for (const auto &row : rows_) {
        if (QString::fromStdString(row.id) == openId_) {
            return &row;
        }
    }
    return nullptr;
}

QString ArtifactsViewModel::openKind() const
{
    const auto *row = current();
    return row == nullptr ? QString{} : kindText(row->kind);
}

QString ArtifactsViewModel::question() const
{
    const auto *row = current();
    return row == nullptr ? QString{} : QString::fromStdString(row->question);
}

QString ArtifactsViewModel::content() const
{
    const auto *row = current();
    return row == nullptr ? QString{} : QString::fromStdString(row->content);
}

QString ArtifactsViewModel::shape() const
{
    const auto *row = current();
    if (row == nullptr) {
        return {};
    }
    const auto text = core::toString(row->shape);
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

QString ArtifactsViewModel::structured() const
{
    const auto *row = current();
    return row == nullptr ? QString{} : QString::fromStdString(row->structured);
}

bool ArtifactsViewModel::working() const
{
    const auto *row = current();
    return row != nullptr && (row->status == core::ArtifactStatus::Running ||
                              row->status == core::ArtifactStatus::Queued);
}

QString ArtifactsViewModel::failure() const
{
    const auto *row = current();
    if (row == nullptr || row->status != core::ArtifactStatus::Failed) {
        return {};
    }
    return QString::fromStdString(row->error);
}

QString ArtifactsViewModel::provenance() const
{
    const auto *row = current();
    if (row == nullptr) {
        return {};
    }

    if (row->costMicros == 0) {
        return {};
    }
    return QLocale{}.toString(static_cast<double>(row->costMicros) / 1'000'000.0, 'f', 4);
}

QString ArtifactsViewModel::madeBy() const
{
    const auto *row = current();
    if (row == nullptr) {
        return {};
    }

    QStringList parts;

    const auto who = providerNameOf(row->providerId, row->providerType, providerNames_);
    if (!who.isEmpty()) {
        parts << who;
    }
    if (!row->model.empty()) {
        parts << QString::fromStdString(row->model);
    }

    return parts.join(tr(" \u00b7 "));
}

QVariantList ArtifactsViewModel::choices() const
{
    QVariantList out;
    for (const auto &prompt : templates_) {
        if (!prompt.enabled) {
            continue;
        }
        QVariantMap entry;
        entry["id"] = QString::fromStdString(prompt.id);
        entry["label"] = QString::fromStdString(prompt.name);
        entry["kind"] = kindText(prompt.kind);

        entry["asks"] = core::asksAQuestion(prompt);
        out.push_back(entry);
    }
    return out;
}

QString ArtifactsViewModel::refusal() const
{
    if (!haveModel_) {

        return tr("No language model is set up yet. Add one in Settings, under Processing.");
    }
    if (choices().isEmpty()) {
        return tr("Every prompt is switched off. Turn one on in Settings, under Processing.");
    }
    return {};
}

bool ArtifactsViewModel::exchangeKept() const
{
    const auto *row = current();
    return row != nullptr && core::exchange::kept(row->id);
}

QString ArtifactsViewModel::exchangeAsText() const
{
    const auto *row = current();
    return row == nullptr ? QString{} : QString::fromStdString(core::exchange::readBack(row->id));
}

QString ArtifactsViewModel::readingKindOf(const core::Artifact &row)
{
    if (row.kind == core::ArtifactKind::Qa) {

        return {};
    }
    return row.templateCode.empty() ? kindText(row.kind) : QString::fromStdString(row.templateCode);
}

void ArtifactsViewModel::open(const QString &artifactId)
{
    if (openId_ == artifactId) {
        return;
    }
    openId_ = artifactId;

    if (const auto *row = current(); row != nullptr) {
        following_ = readingKindOf(*row);
    } else if (artifactId.isEmpty()) {
        following_.clear();
    }

    wanting_.clear();
    Q_EMIT openChanged();
}

void ArtifactsViewModel::run(const QString &templateId, const QString &question)
{
    if (!run_) {
        return;
    }

    known_.clear();
    known_.reserve(rows_.size());
    for (const auto &row : rows_) {
        known_.push_back(row.id);
    }
    wanting_ = templateId;

    run_(templateId, question);
}

void ArtifactsViewModel::forget()
{
    const auto *row = current();
    if (row == nullptr || !forget_) {
        return;
    }
    forget_(QString::fromStdString(row->id));
}

QVariantMap ArtifactsViewModel::openRow() const
{
    const auto *row = current();
    if (row == nullptr) {
        return {};
    }
    QVariantMap entry;
    entry["id"] = QString::fromStdString(row->id);
    entry["kind"] = kindText(row->kind);
    entry["label"] = nameOf(*row);
    entry["name"] = QString::fromStdString(row->templateName);
    return entry;
}

QString ArtifactsViewModel::openName() const
{

    const auto *row = current();
    return row == nullptr ? QString{} : labelOf(*row);
}

QString ArtifactsViewModel::openAsText() const
{
    const auto *row = current();
    if (row == nullptr) {
        return {};
    }

    QString text;
    if (!row->question.empty()) {
        text += QString::fromStdString(row->question) + QStringLiteral("\n\n");
    }
    if (const auto read = openAnswer(); read.has_value()) {
        text += QString::fromStdString(core::renderAnswer(*read, words()));
    } else {
        text += QString::fromStdString(row->content);
    }
    if (!text.endsWith(QLatin1Char{'\n'})) {
        text += QLatin1Char{'\n'};
    }
    return text;
}

QString ArtifactsViewModel::exportName() const
{
    const auto stem = stem_.isEmpty() ? QStringLiteral("conversation") : stem_;

    const auto *row = current();
    auto what = row == nullptr ? QString{} : nameOf(*row);

    static const QString kForbidden = QStringLiteral("/\\:*?\"<>|");
    for (auto &letter : what) {
        if (kForbidden.contains(letter)) {
            letter = QLatin1Char{' '};
        }
    }
    what = what.simplified();
    if (what.size() > 60) {
        what = what.left(60).trimmed();
    }
    if (what.isEmpty()) {
        what = tr("answer");
    }
    return stem + QStringLiteral(" — ") + what + QStringLiteral(".txt");
}

void ArtifactsViewModel::saveTo(const QUrl &target)
{
    if (!save_ || !target.isValid() || current() == nullptr) {
        return;
    }

    save_(target.toLocalFile(), openAsText());
}

void ArtifactsViewModel::setStem(const QString &stem)
{
    stem_ = stem;
}

void ArtifactsViewModel::setArtifacts(std::vector<core::Artifact> rows)
{
    rows_ = std::move(rows);

    if (!wanting_.isEmpty()) {
        for (const auto &row : rows_) {
            if (QString::fromStdString(row.templateId) != wanting_) {
                continue;
            }
            if (std::find(known_.begin(), known_.end(), row.id) != known_.end()) {
                continue;
            }
            openId_ = QString::fromStdString(row.id);
            wanting_.clear();
            known_.clear();
            break;
        }
    }

    if (current() == nullptr) {
        openId_.clear();
        if (!following_.isEmpty()) {
            for (const auto &row : rows_) {
                if (readingKindOf(row) == following_) {
                    openId_ = QString::fromStdString(row.id);
                    break;
                }
            }
        }
    }

    if (const auto *row = current(); row != nullptr) {
        following_ = readingKindOf(*row);
    }

    Q_EMIT rowsChanged();
    Q_EMIT openChanged();
}

void ArtifactsViewModel::setTemplates(std::vector<core::PromptTemplate> rows, bool haveModel)
{
    templates_ = std::move(rows);
    haveModel_ = haveModel;
    Q_EMIT choicesChanged();
}

void ArtifactsViewModel::setVocabulary(Names names)
{
    names_ = std::move(names);
}

QString ArtifactsViewModel::severityWord(core::Severity severity)
{
    switch (severity) {
    case core::Severity::Low:

        return tr("low");
    case core::Severity::Medium:

        return tr("medium");
    case core::Severity::High:

        return tr("high");
    }
    const auto name = core::toString(severity);
    return QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size()));
}

QString ArtifactsViewModel::verdictWord(const QString &verdict)
{
    if (verdict == QStringLiteral("pass")) {

        return tr("pass");
    }
    if (verdict == QStringLiteral("weak")) {

        return tr("weak");
    }
    if (verdict == QStringLiteral("fail")) {

        return tr("fail");
    }
    if (verdict == QStringLiteral("n/a")) {

        return tr("n/a");
    }

    return verdict;
}

QString ArtifactsViewModel::severityText(const QString &severity) const
{
    const auto read = core::parseSeverity(severity.toStdString());
    return read.has_value() ? severityWord(*read) : severity;
}

QString ArtifactsViewModel::verdictText(const QString &verdict) const
{
    return verdictWord(verdict);
}

core::AnswerWords ArtifactsViewModel::words() const
{
    core::AnswerWords words;
    if (names_) {
        words.titleOf = [this](std::string_view kind, std::string_view code) {
            return names_(QString::fromUtf8(kind.data(), static_cast<qsizetype>(kind.size())),
                          QString::fromUtf8(code.data(), static_cast<qsizetype>(code.size())))
                .toStdString();
        };
    }

    words.severityOf = [](core::Severity severity) { return severityWord(severity).toStdString(); };
    words.verdictOf = [](std::string_view verdict) {
        return verdictWord(
                   QString::fromUtf8(verdict.data(), static_cast<qsizetype>(verdict.size())))
            .toStdString();
    };

    words.labelKind = openKind() == QStringLiteral("tags") ? "tag" : "category";
    return words;
}

std::optional<core::Answer> ArtifactsViewModel::openAnswer() const
{
    const auto *row = current();
    if (row == nullptr || row->structured.empty()) {
        return std::nullopt;
    }

    auto read = core::readAnswer(row->shape, row->structured);
    if (!read.ok()) {
        return std::nullopt;
    }
    return read;
}

QString ArtifactsViewModel::entryAsText(int index) const
{
    const auto read = openAnswer();
    if (!read.has_value()) {
        return {};
    }
    return QString::fromStdString(core::renderAnswerEntry(*read, index, words()));
}

void ArtifactsViewModel::setActions(Run run, Forget forget, Save save, Plain refresh)
{
    run_ = std::move(run);
    forget_ = std::move(forget);
    save_ = std::move(save);
    refresh_ = std::move(refresh);
    if (refresh_) {
        refresh_();
    }
}

}
