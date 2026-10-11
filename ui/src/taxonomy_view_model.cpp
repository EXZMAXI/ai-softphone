// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "taxonomy_view_model.hpp"

#include <QCoreApplication>
#include <QVariantMap>

#include <algorithm>
#include <chrono>
#include <utility>

namespace softphone::ui {
namespace {

[[nodiscard]] QString kindText(core::TaxonomyKind kind)
{
    return QString::fromUtf8(core::toString(kind).data(),
                             static_cast<qsizetype>(core::toString(kind).size()));
}

}

TaxonomyViewModel::TaxonomyViewModel(QObject *parent) : QObject{parent} {}

QString TaxonomyViewModel::titleFor(const QString &code, const QString &stored, bool builtIn)
{

    static_cast<void>(code);
    static_cast<void>(builtIn);
    return stored;
}

QString TaxonomyViewModel::descriptionFor(const QString &code, const QString &stored, bool builtIn)
{
    static_cast<void>(code);
    static_cast<void>(builtIn);
    return stored;
}

QVariantList TaxonomyViewModel::rowsOf(core::TaxonomyKind kind) const
{
    QVariantList out;
    for (const auto &row : rows_) {
        if (row.kind != kind) {
            continue;
        }
        if (row.retired() && !showRetired_) {
            continue;
        }

        const auto code = QString::fromStdString(row.code);
        QVariantMap entry;
        entry["id"] = QString::fromStdString(row.id);
        entry["kind"] = kindText(row.kind);
        entry["code"] = code;
        entry["title"] = titleFor(code, QString::fromStdString(row.title), row.builtIn);
        entry["description"] =
            descriptionFor(code, QString::fromStdString(row.description), row.builtIn);
        entry["severity"] =
            row.severity.has_value()
                ? QString::fromUtf8(core::toString(*row.severity).data(),
                                    static_cast<qsizetype>(core::toString(*row.severity).size()))
                : QString{};
        entry["builtIn"] = row.builtIn;
        entry["retired"] = row.retired();
        entry["used"] = inUse(row.kind, code);
        out.push_back(entry);
    }
    return out;
}

QVariantList TaxonomyViewModel::categories() const
{
    return rowsOf(core::TaxonomyKind::Category);
}

QVariantList TaxonomyViewModel::tags() const
{
    return rowsOf(core::TaxonomyKind::Tag);
}

QVariantList TaxonomyViewModel::redFlags() const
{
    return rowsOf(core::TaxonomyKind::RedFlag);
}

QVariantList TaxonomyViewModel::instructions() const
{
    return rowsOf(core::TaxonomyKind::Instruction);
}

QVariantList TaxonomyViewModel::rowsNow(const QString &kind) const
{
    return rowsOf(
        core::parseTaxonomyKind(kind.toStdString()).value_or(core::TaxonomyKind::Category));
}

const core::TaxonomyEntry *TaxonomyViewModel::find(const QString &id) const
{
    const auto wanted = id.toStdString();
    const auto found =
        std::find_if(rows_.begin(), rows_.end(),
                     [&wanted](const core::TaxonomyEntry &row) { return row.id == wanted; });
    return found == rows_.end() ? nullptr : &*found;
}

bool TaxonomyViewModel::inUse(core::TaxonomyKind kind, const QString &code) const
{
    switch (kind) {
    case core::TaxonomyKind::Category:
        return usedCategories_.contains(code);
    case core::TaxonomyKind::Tag:
        return usedTags_.contains(code);
    case core::TaxonomyKind::RedFlag:
        return usedFlags_.contains(code);
    case core::TaxonomyKind::Instruction:

        return false;
    }
    return false;
}

bool TaxonomyViewModel::codeLocked() const
{
    if (editingId_.isEmpty()) {
        return false;
    }
    const auto *row = find(editingId_);
    return row != nullptr && inUse(row->kind, QString::fromStdString(row->code));
}

bool TaxonomyViewModel::complete() const
{
    return refusal().isEmpty();
}

QString TaxonomyViewModel::refusal() const
{
    const auto code = draft_.code.trimmed();
    if (code.isEmpty() || draft_.title.trimmed().isEmpty()) {

        return {};
    }
    if (!core::validTaxonomyCode(code.toStdString())) {
        return tr("A code is lowercase letters, digits and hyphens — sales, vip, "
                  "card-number-read-aloud.");
    }

    const auto clash =
        std::any_of(rows_.begin(), rows_.end(), [this, &code](const core::TaxonomyEntry &row) {
            return row.kind == draft_.kind && QString::fromStdString(row.code) == code &&
                   QString::fromStdString(row.id) != editingId_;
        });
    if (clash) {
        return tr("Another entry in this list already answers to that code.");
    }
    return {};
}

QString TaxonomyViewModel::categoryTitle(const QString &code) const
{
    const auto wanted = code.toStdString();
    for (const auto &row : rows_) {
        if (row.kind == core::TaxonomyKind::Category && row.code == wanted) {
            return titleFor(code, QString::fromStdString(row.title), row.builtIn);
        }
    }
    return code;
}

QString TaxonomyViewModel::titleOf(const QString &kind, const QString &code) const
{
    const auto wanted = core::parseTaxonomyKind(kind.toStdString());
    if (!wanted.has_value()) {
        return code;
    }
    const auto text = code.toStdString();
    for (const auto &row : rows_) {
        if (row.kind == *wanted && row.code == text) {
            return titleFor(code, QString::fromStdString(row.title), row.builtIn);
        }
    }
    return code;
}

QString TaxonomyViewModel::severityOf(const QString &code) const
{
    const auto text = code.toStdString();
    for (const auto &row : rows_) {
        if (row.kind == core::TaxonomyKind::RedFlag && row.code == text &&
            row.severity.has_value()) {
            const auto name = core::toString(*row.severity);
            return QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size()));
        }
    }
    return {};
}

void TaxonomyViewModel::retranslated()
{
    Q_EMIT rowsChanged();
    Q_EMIT draftChanged();
}

void TaxonomyViewModel::setShowRetired(bool value)
{
    if (showRetired_ == value) {
        return;
    }
    showRetired_ = value;
    Q_EMIT rowsChanged();
}

void TaxonomyViewModel::setRetired(bool value)
{
    if (draft_.retired == value) {
        return;
    }
    draft_.retired = value;
    Q_EMIT draftChanged();
}

void TaxonomyViewModel::setCode(const QString &value)
{
    if (draft_.code == value) {
        return;
    }

    draft_.code = value;
    Q_EMIT draftChanged();
}

void TaxonomyViewModel::setTitle(const QString &value)
{
    if (draft_.title == value) {
        return;
    }
    draft_.title = value;
    Q_EMIT draftChanged();
}

void TaxonomyViewModel::setDescription(const QString &value)
{
    if (draft_.description == value) {
        return;
    }
    draft_.description = value;
    Q_EMIT draftChanged();
}

void TaxonomyViewModel::setSeverity(const QString &value)
{
    if (draft_.severity == value) {
        return;
    }
    draft_.severity = value;
    Q_EMIT draftChanged();
}

void TaxonomyViewModel::openOn(const core::TaxonomyEntry *row, core::TaxonomyKind kind)
{
    draft_ = Draft{};
    draft_.kind = row != nullptr ? row->kind : kind;

    if (row != nullptr) {
        const auto code = QString::fromStdString(row->code);
        draft_.code = code;
        draft_.title = titleFor(code, QString::fromStdString(row->title), row->builtIn);
        draft_.description =
            descriptionFor(code, QString::fromStdString(row->description), row->builtIn);
        draft_.builtIn = row->builtIn;
        draft_.retired = row->retired();
        if (row->severity.has_value()) {
            draft_.severity =
                QString::fromUtf8(core::toString(*row->severity).data(),
                                  static_cast<qsizetype>(core::toString(*row->severity).size()));
        }

        draft_.openedTitle = draft_.title;
        draft_.openedDescription = draft_.description;
    }

    editingId_ = row != nullptr ? QString::fromStdString(row->id) : QString{};
    editingKind_ = kindText(draft_.kind);
    editing_ = true;

    Q_EMIT editingChanged();
    Q_EMIT draftChanged();
}

void TaxonomyViewModel::add(const QString &kind)
{
    openOn(nullptr,
           core::parseTaxonomyKind(kind.toStdString()).value_or(core::TaxonomyKind::Category));
}

void TaxonomyViewModel::edit(const QString &id)
{
    if (const auto *row = find(id); row != nullptr) {
        openOn(row, row->kind);
    }
}

void TaxonomyViewModel::cancel()
{
    if (!editing_) {
        return;
    }
    editing_ = false;
    editingId_.clear();
    editingKind_.clear();
    draft_ = Draft{};
    Q_EMIT editingChanged();
    Q_EMIT draftChanged();
}

core::TaxonomyEntry TaxonomyViewModel::asRow() const
{
    core::TaxonomyEntry row;
    if (const auto *existing = find(editingId_); existing != nullptr) {
        row = *existing;
    }

    row.kind = draft_.kind;
    row.code = draft_.code.trimmed().toStdString();
    row.title = draft_.title.trimmed().toStdString();
    row.description = draft_.description.trimmed().toStdString();
    row.severity = draft_.kind == core::TaxonomyKind::RedFlag
                       ? core::parseSeverity(draft_.severity.toStdString())
                       : std::nullopt;

    if (draft_.retired && !row.retiredAt.has_value()) {
        row.retiredAt = std::chrono::system_clock::now();
    } else if (!draft_.retired) {
        row.retiredAt.reset();
    }

    if (draft_.builtIn && (draft_.title.trimmed() != draft_.openedTitle.trimmed() ||
                           draft_.description.trimmed() != draft_.openedDescription.trimmed())) {
        row.builtIn = false;
    }

    return row;
}

void TaxonomyViewModel::save()
{
    if (!editing_ || !complete()) {
        return;
    }
    if (draft_.code.trimmed().isEmpty() || draft_.title.trimmed().isEmpty()) {
        return;
    }

    const auto row = asRow();
    cancel();
    if (save_) {
        save_(row);
    }
}

void TaxonomyViewModel::remove(const QString &id)
{
    if (editingId_ == id) {
        cancel();
    }
    if (remove_) {
        remove_(id);
    }
}

QVariantMap TaxonomyViewModel::rowFor(const QString &id) const
{
    const auto *row = find(id);
    if (row == nullptr) {
        return {};
    }
    const auto code = QString::fromStdString(row->code);

    QVariantMap entry;
    entry["id"] = QString::fromStdString(row->id);
    entry["kind"] = kindText(row->kind);
    entry["code"] = code;
    entry["title"] = titleFor(code, QString::fromStdString(row->title), row->builtIn);
    entry["used"] = inUse(row->kind, code);
    entry["retired"] = row->retired();
    return entry;
}

void TaxonomyViewModel::setEntries(std::vector<core::TaxonomyEntry> rows,
                                   QStringList usedCategories, QStringList usedTags,
                                   QStringList usedFlags)
{
    rows_ = std::move(rows);
    usedCategories_ = std::move(usedCategories);
    usedTags_ = std::move(usedTags);
    usedFlags_ = std::move(usedFlags);
    Q_EMIT rowsChanged();
    Q_EMIT draftChanged();
}

void TaxonomyViewModel::restoreEverything()
{
    if (restoreAll_) {
        cancel();
        restoreAll_();
    }
}

void TaxonomyViewModel::setActions(Save save, Remove remove, Plain refresh, Plain restoreAll)
{
    restoreAll_ = std::move(restoreAll);
    save_ = std::move(save);
    remove_ = std::move(remove);
    refresh_ = std::move(refresh);
    if (refresh_) {
        refresh_();
    }
}

}
