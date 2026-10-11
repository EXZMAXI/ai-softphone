// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "automations_view_model.hpp"

#include "taxonomy_view_model.hpp"
#include "templates_view_model.hpp"

#include <QCoreApplication>

#include <algorithm>
#include <array>
#include <utility>

namespace softphone::ui {
namespace {

constexpr const char *kActions[] = {"transcribe", "generate", "tag", "stop"};
constexpr const char *kFields[] = {"category", "tag",    "red_flag", "direction", "account",
                                   "duration", "number", "language", "hour",      "source"};

constexpr const char *kOperators[] = {"is",      "is_not",  "contains", "at_least",
                                      "at_most", "present", "absent"};

[[nodiscard]] QString textOf(std::string_view from)
{
    return QString::fromUtf8(from.data(), static_cast<qsizetype>(from.size()));
}

[[nodiscard]] QVariantList named(const char *const *from, std::size_t count,
                                 QString (*label)(const QString &))
{
    QVariantList out;
    for (std::size_t at = 0; at < count; ++at) {
        const auto id = QString::fromLatin1(from[at]);
        QVariantMap entry;
        entry["id"] = id;
        entry["label"] = label(id);
        out.push_back(entry);
    }
    return out;
}

}

AutomationsViewModel::AutomationsViewModel(QObject *parent) : QObject{parent} {}

QString AutomationsViewModel::nameFor(const QString &code, const QString &stored)
{

    static_cast<void>(code);
    return stored;
}

QString AutomationsViewModel::labelForAction(const QString &id)
{
    if (id == QLatin1String{"transcribe"}) {

        return QCoreApplication::translate("AutomationsViewModel", "transcribe it");
    }
    if (id == QLatin1String{"tag"}) {
        return QCoreApplication::translate("AutomationsViewModel", "put a tag on it");
    }
    if (id == QLatin1String{"stop"}) {
        return QCoreApplication::translate("AutomationsViewModel", "stop, and run no more rules");
    }
    return QCoreApplication::translate("AutomationsViewModel", "ask a model");
}

QString AutomationsViewModel::labelForField(const QString &id)
{
    struct Named {
        const char *id;
        const char *label;
    };

    static const Named kNames[] = {
        {"category", QT_TRANSLATE_NOOP("AutomationsViewModel", "the category")},
        {"tag", QT_TRANSLATE_NOOP("AutomationsViewModel", "the tags")},
        {"red_flag", QT_TRANSLATE_NOOP("AutomationsViewModel", "the red flags")},
        {"direction", QT_TRANSLATE_NOOP("AutomationsViewModel", "the direction")},
        {"account", QT_TRANSLATE_NOOP("AutomationsViewModel", "the account")},
        {"duration", QT_TRANSLATE_NOOP("AutomationsViewModel", "the length in seconds")},
        {"number", QT_TRANSLATE_NOOP("AutomationsViewModel", "the number")},
        {"language", QT_TRANSLATE_NOOP("AutomationsViewModel", "the language spoken")},
        {"hour", QT_TRANSLATE_NOOP("AutomationsViewModel", "the hour it started")},
        {"source", QT_TRANSLATE_NOOP("AutomationsViewModel", "where it came from")},
    };
    for (const auto &named : kNames) {
        if (id == QLatin1String{named.id}) {
            return QCoreApplication::translate("AutomationsViewModel", named.label);
        }
    }
    return id;
}

QString AutomationsViewModel::labelForOperator(const QString &id)
{
    struct Named {
        const char *id;
        const char *label;
    };

    static const Named kNames[] = {
        {"is", QT_TRANSLATE_NOOP("AutomationsViewModel", "is")},
        {"is_not", QT_TRANSLATE_NOOP("AutomationsViewModel", "is not")},
        {"contains", QT_TRANSLATE_NOOP("AutomationsViewModel", "contains")},
        {"at_least", QT_TRANSLATE_NOOP("AutomationsViewModel", "is at least")},
        {"at_most", QT_TRANSLATE_NOOP("AutomationsViewModel", "is at most")},
        {"present", QT_TRANSLATE_NOOP("AutomationsViewModel", "there is one")},
        {"absent", QT_TRANSLATE_NOOP("AutomationsViewModel", "there is none")},
    };
    for (const auto &named : kNames) {
        if (id == QLatin1String{named.id}) {
            return QCoreApplication::translate("AutomationsViewModel", named.label);
        }
    }
    return id;
}

QVariantList AutomationsViewModel::actions() const
{
    return named(kActions, std::size(kActions), &AutomationsViewModel::labelForAction);
}

QVariantList AutomationsViewModel::fields() const
{
    return named(kFields, std::size(kFields), &AutomationsViewModel::labelForField);
}

QVariantList AutomationsViewModel::operators() const
{
    return named(kOperators, std::size(kOperators), &AutomationsViewModel::labelForOperator);
}

QVariantList AutomationsViewModel::operatorsFor(const QString &field) const
{

    struct Allowed {
        const char *field;
        std::array<const char *, 4> ops;
    };

    static const Allowed kAllowed[] = {
        {"category", {"is", "is_not", "present", "absent"}},
        {"tag", {"present", "absent", nullptr, nullptr}},
        {"red_flag", {"present", "absent", nullptr, nullptr}},
        {"direction", {"is", "is_not", nullptr, nullptr}},
        {"account", {"is", "is_not", nullptr, nullptr}},
        {"language", {"is", "is_not", "present", "absent"}},
        {"number", {"is", "is_not", "contains", nullptr}},
        {"duration", {"at_least", "at_most", nullptr, nullptr}},
        {"hour", {"at_least", "at_most", "is", nullptr}},
        {"source", {"is", "is_not", nullptr, nullptr}},
    };

    QVariantList out;
    for (const auto &allowed : kAllowed) {
        if (field != QLatin1String{allowed.field}) {
            continue;
        }
        for (const auto *op : allowed.ops) {
            if (op == nullptr) {
                break;
            }
            const auto id = QString::fromLatin1(op);
            QVariantMap entry;
            entry["id"] = id;
            entry["label"] = labelForOperator(id);
            out.push_back(entry);
        }
        return out;
    }
    return named(kOperators, std::size(kOperators), &AutomationsViewModel::labelForOperator);
}

QVariantList AutomationsViewModel::valuesFor(const QString &field) const
{
    const auto entries = [](const std::vector<core::TaxonomyEntry> &from) {
        QVariantList out;
        for (const auto &one : from) {
            if (one.code.empty() || one.retired()) {
                continue;
            }
            QVariantMap entry;
            entry["id"] = QString::fromStdString(one.code);

            entry["label"] =
                one.title.empty()
                    ? QString::fromStdString(one.code)
                    : TaxonomyViewModel::titleFor(QString::fromStdString(one.code),
                                                  QString::fromStdString(one.title), one.builtIn);
            out.push_back(entry);
        }
        return out;
    };

    if (field == QLatin1String{"category"}) {
        return entries(categories_);
    }
    if (field == QLatin1String{"tag"}) {
        return entries(tags_);
    }
    if (field == QLatin1String{"red_flag"}) {
        return entries(redFlags_);
    }
    if (field == QLatin1String{"direction"}) {
        QVariantList out;
        QVariantMap in;
        in["id"] = QStringLiteral("in");

        in["label"] = tr("incoming");
        out.push_back(in);
        QVariantMap outgoing;
        outgoing["id"] = QStringLiteral("out");
        outgoing["label"] = tr("outgoing");
        out.push_back(outgoing);
        return out;
    }
    if (field == QLatin1String{"source"}) {

        QVariantList out;
        QVariantMap own;
        own["id"] = QStringLiteral("native");

        own["label"] = tr("this phone");
        out.push_back(own);
        QVariantMap other;
        other["id"] = QStringLiteral("external");
        other["label"] = tr("another application");
        out.push_back(other);
        return out;
    }
    if (field == QLatin1String{"account"}) {
        QVariantList out;
        for (const auto &[id, label] : accounts_) {
            QVariantMap entry;
            entry["id"] = id;
            entry["label"] = label.isEmpty() ? id : label;
            out.push_back(entry);
        }
        return out;
    }
    return {};
}

QString AutomationsViewModel::valueKindFor(const QString &field, const QString &op) const
{

    const auto isList = field == QLatin1String{"tag"} || field == QLatin1String{"red_flag"};
    const auto asksOnly = op == QLatin1String{"present"} || op == QLatin1String{"absent"};
    if (asksOnly && !isList) {
        return QStringLiteral("none");
    }

    if (field == QLatin1String{"duration"} || field == QLatin1String{"hour"}) {
        return QStringLiteral("number");
    }

    if (isList || field == QLatin1String{"category"} || field == QLatin1String{"direction"} ||
        field == QLatin1String{"account"} || field == QLatin1String{"source"}) {
        return QStringLiteral("choice");
    }
    return QStringLiteral("text");
}

QVariantList AutomationsViewModel::prompts() const
{
    QVariantList out;
    for (const auto &prompt : prompts_) {
        if (!prompt.enabled || prompt.code.empty()) {

            continue;
        }
        QVariantMap entry;
        entry["id"] = QString::fromStdString(prompt.code);
        entry["label"] = TemplatesViewModel::nameFor(QString::fromStdString(prompt.code),
                                                     QString::fromStdString(prompt.name));
        out.push_back(entry);
    }
    return out;
}

QString AutomationsViewModel::sentenceOf(const core::Automation &row) const
{

    QStringList clauses;
    for (const auto &one : row.conditions) {
        const auto field = labelForField(textOf(core::toString(one.field)));
        const auto op = labelForOperator(textOf(core::toString(one.op)));

        if (one.op == core::ConditionOp::Present || one.op == core::ConditionOp::Absent) {
            if (one.value.empty()) {

                clauses.push_back(tr("%1: %2").arg(field).arg(op));
                continue;
            }
        }

        const auto value = QString::fromStdString(one.value);
        const auto choices = valuesFor(textOf(core::toString(one.field)));
        const auto named =
            std::find_if(choices.begin(), choices.end(), [&value](const QVariant &choice) {
                return choice.toMap().value("id").toString() == value;
            });
        const auto shown =
            named == choices.end() ? value : named->toMap().value("label").toString();

        clauses.push_back(tr("%1 %2 %3").arg(field).arg(op).arg(shown));
    }

    QString what = labelForAction(textOf(core::toString(row.action)));
    if (row.action == core::Action::Generate && !row.templateCode.empty()) {
        const auto prompt =
            std::find_if(prompts_.begin(), prompts_.end(), [&row](const core::PromptTemplate &one) {
                return one.code == row.templateCode;
            });
        const auto name = prompt == prompts_.end()
                              ? QString::fromStdString(row.templateCode)
                              : TemplatesViewModel::nameFor(QString::fromStdString(prompt->code),
                                                            QString::fromStdString(prompt->name));

        what = tr("%1: %2").arg(what).arg(name);
    }
    if (row.action == core::Action::Tag && !row.tagCode.empty()) {

        what = tr("%1: %2").arg(what).arg(QString::fromStdString(row.tagCode));
    }

    if (clauses.isEmpty()) {
        return what;
    }

    return tr("If %1, %2").arg(clauses.join(tr(", and "))).arg(what);
}

void AutomationsViewModel::retranslated()
{
    Q_EMIT rowsChanged();
    Q_EMIT promptsChanged();
    Q_EMIT draftChanged();
}

QVariantList AutomationsViewModel::rows() const
{
    QVariantList out;
    out.reserve(static_cast<qsizetype>(rows_.size()));
    for (const auto &row : rows_) {
        QVariantMap entry;
        entry["id"] = QString::fromStdString(row.id);
        entry["code"] = QString::fromStdString(row.code);
        entry["name"] = nameFor(QString::fromStdString(row.code), QString::fromStdString(row.name));
        entry["enabled"] = row.enabled;
        entry["builtIn"] = row.builtIn;
        entry["sentence"] = sentenceOf(row);
        out.push_back(entry);
    }
    return out;
}

QVariantList AutomationsViewModel::rowsNow() const
{
    return rows();
}

const core::Automation *AutomationsViewModel::find(const QString &id) const
{
    const auto wanted = id.toStdString();
    const auto found =
        std::find_if(rows_.begin(), rows_.end(),
                     [&wanted](const core::Automation &row) { return row.id == wanted; });
    return found == rows_.end() ? nullptr : &*found;
}

QVariantList AutomationsViewModel::conditions() const
{
    QVariantList out;
    for (const auto &one : draft_.conditions) {
        QVariantMap entry;
        entry["field"] = textOf(core::toString(one.field));
        entry["op"] = textOf(core::toString(one.op));
        entry["value"] = QString::fromStdString(one.value);
        out.push_back(entry);
    }
    return out;
}

bool AutomationsViewModel::complete() const
{
    if (draft_.name.trimmed().isEmpty()) {
        return false;
    }
    if (draft_.action == QLatin1String{"generate"} && draft_.templateCode.isEmpty()) {
        return false;
    }
    if (draft_.action == QLatin1String{"tag"} && draft_.tagCode.trimmed().isEmpty()) {
        return false;
    }
    return true;
}

QString AutomationsViewModel::sentence() const
{
    return sentenceOf(asRow());
}

QString AutomationsViewModel::sentenceFor(const QString &id) const
{
    const auto *row = find(id);
    return row == nullptr ? QString{} : sentenceOf(*row);
}

void AutomationsViewModel::setName(const QString &value)
{
    if (draft_.name == value) {
        return;
    }
    draft_.name = value;
    Q_EMIT draftChanged();
}

void AutomationsViewModel::setAction(const QString &value)
{
    if (draft_.action == value) {
        return;
    }
    draft_.action = value;
    Q_EMIT draftChanged();
}

void AutomationsViewModel::setTemplateCode(const QString &value)
{
    if (draft_.templateCode == value) {
        return;
    }
    draft_.templateCode = value;
    Q_EMIT draftChanged();
}

void AutomationsViewModel::setTagCode(const QString &value)
{
    if (draft_.tagCode == value) {
        return;
    }
    draft_.tagCode = value;
    Q_EMIT draftChanged();
}

void AutomationsViewModel::setSwitchedOn(bool value)
{
    if (draft_.enabled == value) {
        return;
    }
    draft_.enabled = value;
    Q_EMIT draftChanged();
}

void AutomationsViewModel::addCondition()
{
    draft_.conditions.push_back({core::ConditionField::Category, core::ConditionOp::Is, {}});
    Q_EMIT conditionsChanged();
    Q_EMIT draftChanged();
}

void AutomationsViewModel::removeCondition(int at)
{
    if (at < 0 || at >= static_cast<int>(draft_.conditions.size())) {
        return;
    }
    draft_.conditions.erase(draft_.conditions.begin() + at);
    Q_EMIT conditionsChanged();
    Q_EMIT draftChanged();
}

void AutomationsViewModel::settle(core::Condition &clause) const
{

    const auto field = textOf(core::toString(clause.field));

    const auto allowed = operatorsFor(field);
    const auto has = [&allowed](const QString &id) {
        return std::any_of(allowed.begin(), allowed.end(), [&id](const QVariant &one) {
            return one.toMap().value("id").toString() == id;
        });
    };
    if (!allowed.isEmpty() && !has(textOf(core::toString(clause.op)))) {
        const auto first = allowed.front().toMap().value("id").toString().toStdString();
        clause.op = core::parseConditionOp(first).value_or(clause.op);
    }

    if (clause.value.empty()) {
        return;
    }
    const auto choices = valuesFor(field);
    const auto wanted = QString::fromStdString(clause.value);
    const auto known = std::any_of(choices.begin(), choices.end(), [&wanted](const QVariant &one) {
        return one.toMap().value("id").toString() == wanted;
    });
    if (!known) {
        clause.value.clear();
    }
}

void AutomationsViewModel::setConditionField(int at, const QString &field)
{
    if (at < 0 || at >= static_cast<int>(draft_.conditions.size())) {
        return;
    }
    const auto parsed = core::parseConditionField(field.toStdString());
    if (!parsed.has_value()) {
        return;
    }
    auto &clause = draft_.conditions.at(static_cast<std::size_t>(at));
    if (clause.field == *parsed) {
        return;
    }
    clause.field = *parsed;
    settle(clause);

    Q_EMIT conditionsChanged();
    Q_EMIT draftChanged();
}

void AutomationsViewModel::setConditionOperator(int at, const QString &op)
{
    if (at < 0 || at >= static_cast<int>(draft_.conditions.size())) {
        return;
    }
    if (const auto parsed = core::parseConditionOp(op.toStdString()); parsed.has_value()) {
        draft_.conditions.at(static_cast<std::size_t>(at)).op = *parsed;

        Q_EMIT conditionsChanged();
        Q_EMIT draftChanged();
    }
}

void AutomationsViewModel::setConditionValue(int at, const QString &value)
{
    if (at < 0 || at >= static_cast<int>(draft_.conditions.size())) {
        return;
    }
    auto &clause = draft_.conditions.at(static_cast<std::size_t>(at));
    if (QString::fromStdString(clause.value) == value) {
        return;
    }

    clause.value = value.toStdString();

    Q_EMIT draftChanged();
}

void AutomationsViewModel::openOn(const core::Automation *row)
{
    draft_ = Draft{};

    if (row != nullptr) {
        draft_.name = nameFor(QString::fromStdString(row->code), QString::fromStdString(row->name));
        draft_.action = textOf(core::toString(row->action));
        draft_.templateCode = QString::fromStdString(row->templateCode);
        draft_.tagCode = QString::fromStdString(row->tagCode);
        draft_.enabled = row->enabled;
        draft_.conditions = row->conditions;
        draft_.code = QString::fromStdString(row->code);
        draft_.builtIn = row->builtIn;
        draft_.ordinal = row->ordinal;
    } else {

        draft_.ordinal = rows_.empty() ? 0 : rows_.back().ordinal + 10;
    }

    editingId_ = row != nullptr ? QString::fromStdString(row->id) : QString{};
    editing_ = true;

    Q_EMIT editingChanged();
    Q_EMIT conditionsChanged();
    Q_EMIT draftChanged();
}

void AutomationsViewModel::add()
{
    openOn(nullptr);
}

void AutomationsViewModel::edit(const QString &id)
{
    if (const auto *row = find(id); row != nullptr) {
        openOn(row);
    }
}

void AutomationsViewModel::cancel()
{
    if (!editing_) {
        return;
    }
    editing_ = false;
    editingId_.clear();
    draft_ = Draft{};
    Q_EMIT editingChanged();
    Q_EMIT conditionsChanged();
    Q_EMIT draftChanged();
}

core::Automation AutomationsViewModel::asRow() const
{
    core::Automation row;
    if (const auto *existing = find(editingId_); existing != nullptr) {
        row = *existing;
    }

    row.name = draft_.name.trimmed().toStdString();
    row.action = core::parseAction(draft_.action.toStdString()).value_or(row.action);
    row.templateCode = draft_.templateCode.toStdString();
    row.tagCode = draft_.tagCode.trimmed().toStdString();
    row.enabled = draft_.enabled;
    row.code = draft_.code.toStdString();
    row.builtIn = draft_.builtIn;
    row.ordinal = draft_.ordinal;

    row.conditions = draft_.conditions;
    for (auto &one : row.conditions) {
        one.value = QString::fromStdString(one.value).trimmed().toStdString();
    }
    return row;
}

void AutomationsViewModel::save()
{
    if (!editing_ || !complete()) {
        return;
    }
    const auto row = asRow();
    cancel();
    if (save_) {
        save_(row);
    }
}

void AutomationsViewModel::remove(const QString &id)
{
    if (editingId_ == id) {
        cancel();
    }
    if (remove_) {
        remove_(id);
    }
}

void AutomationsViewModel::toggle(const QString &id, bool on)
{
    const auto *row = find(id);
    if (row == nullptr || row->enabled == on || !save_) {
        return;
    }
    auto changed = *row;
    changed.enabled = on;
    save_(std::move(changed));
}

void AutomationsViewModel::moveBy(const QString &id, int by)
{
    const auto wanted = id.toStdString();
    const auto found =
        std::find_if(rows_.begin(), rows_.end(),
                     [&wanted](const core::Automation &row) { return row.id == wanted; });
    if (found == rows_.end() || !save_) {
        return;
    }

    using Distance = std::vector<core::Automation>::difference_type;
    const auto at = std::distance(rows_.begin(), found);
    const auto to = at + static_cast<Distance>(by);
    if (to < 0 || to >= static_cast<Distance>(rows_.size())) {
        return;
    }

    auto one = *found;
    auto other = rows_.at(static_cast<std::size_t>(to));
    std::swap(one.ordinal, other.ordinal);
    save_(std::move(one));
    save_(std::move(other));
}

QVariantMap AutomationsViewModel::rowFor(const QString &id) const
{
    const auto *row = find(id);
    if (row == nullptr) {
        return {};
    }
    QVariantMap entry;
    entry["id"] = QString::fromStdString(row->id);
    entry["name"] = QString::fromStdString(row->name);
    entry["builtIn"] = row->builtIn;
    return entry;
}

void AutomationsViewModel::setAutomations(std::vector<core::Automation> rows)
{
    rows_ = std::move(rows);
    Q_EMIT rowsChanged();
    Q_EMIT draftChanged();
}

void AutomationsViewModel::setRecognisers(std::vector<core::ProviderConfig> recognisers)
{
    recognisers_ = std::move(recognisers);
    Q_EMIT draftChanged();
}

void AutomationsViewModel::setModels(std::vector<core::ProviderConfig> models)
{
    models_ = std::move(models);
    Q_EMIT draftChanged();
}

void AutomationsViewModel::setVocabulary(std::vector<core::TaxonomyEntry> categories,
                                         std::vector<core::TaxonomyEntry> tags,
                                         std::vector<core::TaxonomyEntry> redFlags)
{
    categories_ = std::move(categories);
    tags_ = std::move(tags);
    redFlags_ = std::move(redFlags);

    Q_EMIT conditionsChanged();
    Q_EMIT draftChanged();
}

void AutomationsViewModel::setAccounts(std::vector<std::pair<QString, QString>> accounts)
{
    accounts_ = std::move(accounts);
    Q_EMIT conditionsChanged();
    Q_EMIT draftChanged();
}

void AutomationsViewModel::setPrompts(std::vector<core::PromptTemplate> rows)
{
    prompts_ = std::move(rows);
    Q_EMIT promptsChanged();
    Q_EMIT rowsChanged();
}

void AutomationsViewModel::setActions(Save save, Remove remove, Plain refresh)
{
    save_ = std::move(save);
    remove_ = std::move(remove);
    refresh_ = std::move(refresh);
    if (refresh_) {
        refresh_();
    }
}

}
