// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "templates_view_model.hpp"

#include "taxonomy_view_model.hpp"

#include <QCoreApplication>

#include <algorithm>
#include <utility>

namespace softphone::ui {
namespace {

constexpr const char *kKinds[] = {
    "summary", "super_summary", "action_items", "score", "categorise", "tags",     "red_flags",
    "topics",  "entities",      "translation",  "qa",    "custom",     "realtime",
};

constexpr const char *kShapes[] = {
    "plain", "list", "labels", "score", "rubric", "flags", "qa", "json",
};

constexpr const char *kPolicies[] = {"as_spoken", "interface", "fixed"};

[[nodiscard]] std::vector<std::pair<std::string, std::string>> paramsFrom(const QString &text)
{
    std::vector<std::pair<std::string, std::string>> params;
    for (const auto &line : text.split(QLatin1Char{'\n'}, Qt::SkipEmptyParts)) {
        const auto at = line.indexOf(QLatin1Char{'='});
        if (at <= 0) {
            continue;
        }
        const auto name = line.left(at).trimmed();
        if (!name.isEmpty()) {
            params.emplace_back(name.toStdString(), line.mid(at + 1).trimmed().toStdString());
        }
    }
    return params;
}

[[nodiscard]] QString paramsText(const std::vector<std::pair<std::string, std::string>> &params)
{
    QStringList lines;
    lines.reserve(static_cast<qsizetype>(params.size()));
    for (const auto &[name, value] : params) {
        lines.push_back(QString::fromStdString(name) + QStringLiteral(" = ") +
                        QString::fromStdString(value));
    }
    return lines.join(QLatin1Char{'\n'});
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

TemplatesViewModel::TemplatesViewModel(QObject *parent) : QObject{parent} {}

QString TemplatesViewModel::labelForKind(const QString &id)
{

    struct Named {
        const char *id;
        const char *label;
    };

    static const Named kNames[] = {
        {"summary", QT_TRANSLATE_NOOP("TemplatesViewModel", "Summary")},
        {"super_summary", QT_TRANSLATE_NOOP("TemplatesViewModel", "One-line summary")},
        {"action_items", QT_TRANSLATE_NOOP("TemplatesViewModel", "Action items")},
        {"score", QT_TRANSLATE_NOOP("TemplatesViewModel", "Assessment")},
        {"categorise", QT_TRANSLATE_NOOP("TemplatesViewModel", "Category")},
        {"tags", QT_TRANSLATE_NOOP("TemplatesViewModel", "Tags")},
        {"red_flags", QT_TRANSLATE_NOOP("TemplatesViewModel", "Red flags")},
        {"topics", QT_TRANSLATE_NOOP("TemplatesViewModel", "Topics")},
        {"entities", QT_TRANSLATE_NOOP("TemplatesViewModel", "Names and numbers")},
        {"translation", QT_TRANSLATE_NOOP("TemplatesViewModel", "Translation")},
        {"qa", QT_TRANSLATE_NOOP("TemplatesViewModel", "A question about the call")},

        {"realtime", QT_TRANSLATE_NOOP("TemplatesViewModel", "A live prompter")},
    };
    for (const auto &named : kNames) {
        if (id == QLatin1String{named.id}) {
            return QCoreApplication::translate("TemplatesViewModel", named.label);
        }
    }

    return QCoreApplication::translate("TemplatesViewModel", "Something of your own");
}

QString TemplatesViewModel::labelForShape(const QString &id)
{
    struct Named {
        const char *id;
        const char *label;
    };

    static const Named kNames[] = {
        {"plain", QT_TRANSLATE_NOOP("TemplatesViewModel", "A paragraph")},
        {"list", QT_TRANSLATE_NOOP("TemplatesViewModel", "A list of things to do")},
        {"labels", QT_TRANSLATE_NOOP("TemplatesViewModel", "Codes from a dictionary")},
        {"score", QT_TRANSLATE_NOOP("TemplatesViewModel", "A score, with reasons")},
        {"rubric", QT_TRANSLATE_NOOP("TemplatesViewModel", "Criteria judged one by one")},
        {"flags", QT_TRANSLATE_NOOP("TemplatesViewModel", "Occurrences, with the words and the "
                                                          "moment")},
        {"qa", QT_TRANSLATE_NOOP("TemplatesViewModel", "An answer, with what it rests on")},
        {"json", QT_TRANSLATE_NOOP("TemplatesViewModel", "JSON of your own shape")},
    };
    for (const auto &named : kNames) {
        if (id == QLatin1String{named.id}) {
            return QCoreApplication::translate("TemplatesViewModel", named.label);
        }
    }
    return id;
}

QString TemplatesViewModel::labelForPolicy(const QString &id)
{
    if (id == QLatin1String{"interface"}) {

        return QCoreApplication::translate("TemplatesViewModel", "The language of this program");
    }
    if (id == QLatin1String{"fixed"}) {
        return QCoreApplication::translate("TemplatesViewModel", "One language, always");
    }
    return QCoreApplication::translate("TemplatesViewModel", "Whatever was spoken");
}

QVariantList TemplatesViewModel::kinds() const
{

    auto out = named(kKinds, std::size(kKinds), &TemplatesViewModel::labelForKind);
    out.removeIf([](const QVariant &one) {
        return one.toMap()["id"].toString() == QLatin1String{"realtime"};
    });
    return out;
}

QString TemplatesViewModel::shapeTitle(const QString &shape) const
{
    const auto parsed = core::parseAnswerShape(shape.toStdString());
    if (!parsed.has_value()) {
        return labelForShape(shape);
    }
    const auto wanted = std::string{core::instructionCode(*parsed)};
    const auto found =
        std::find_if(instructionEntries_.begin(), instructionEntries_.end(),
                     [&wanted](const core::TaxonomyEntry &one) { return one.code == wanted; });
    if (found == instructionEntries_.end() || found->title.empty()) {
        return labelForShape(shape);
    }
    return TaxonomyViewModel::titleFor(QString::fromStdString(found->code),
                                       QString::fromStdString(found->title), found->builtIn);
}

QVariantList TemplatesViewModel::shapes() const
{
    QVariantList out;
    for (std::size_t at = 0; at < std::size(kShapes); ++at) {
        const auto id = QString::fromLatin1(kShapes[at]);
        QVariantMap one;
        one["id"] = id;
        one["label"] = shapeTitle(id);
        out.push_back(one);
    }
    return out;
}

QVariantList TemplatesViewModel::policies() const
{
    return named(kPolicies, std::size(kPolicies), &TemplatesViewModel::labelForPolicy);
}

QStringList TemplatesViewModel::placeholders() const
{
    QStringList out;
    for (const auto &name : core::knownPlaceholders()) {
        out.push_back(QStringLiteral("{{") + QString::fromStdString(name) + QStringLiteral("}}"));
    }
    return out;
}

QString TemplatesViewModel::nameFor(const QString &code, const QString &stored)
{

    static_cast<void>(code);
    return stored;
}

void TemplatesViewModel::retranslated()
{
    Q_EMIT rowsChanged();
    Q_EMIT draftChanged();
}

QVariantList TemplatesViewModel::rows() const
{
    return rowsOfKind(false);
}

QVariantList TemplatesViewModel::realtimeRows() const
{
    return rowsOfKind(true);
}

QVariantList TemplatesViewModel::rowsOfKind(bool realtime) const
{
    QVariantList out;
    out.reserve(static_cast<qsizetype>(rows_.size()));
    for (const auto &row : rows_) {
        if (core::isRealtime(row) != realtime) {
            continue;
        }
        const auto kind =
            QString::fromUtf8(core::toString(row.kind).data(),
                              static_cast<qsizetype>(core::toString(row.kind).size()));
        const auto shape =
            QString::fromUtf8(core::toString(row.shape).data(),
                              static_cast<qsizetype>(core::toString(row.shape).size()));

        QVariantMap entry;
        entry["id"] = QString::fromStdString(row.id);
        entry["code"] = QString::fromStdString(row.code);
        entry["name"] = nameFor(QString::fromStdString(row.code), QString::fromStdString(row.name));
        entry["kind"] = kind;
        entry["kindLabel"] = labelForKind(kind);
        entry["shape"] = shape;
        entry["shapeLabel"] = shapeTitle(shape);
        entry["enabled"] = row.enabled;
        entry["builtIn"] = row.builtIn;

        const auto shipped = core::shippedTemplate(row.code, shippedIn());
        entry["edited"] = !row.code.empty() && shipped.has_value() &&
                          (shipped->body != row.body || shipped->system != row.system ||
                           shipped->shape != row.shape);
        out.push_back(entry);
    }
    return out;
}

QVariantList TemplatesViewModel::rowsNow() const
{
    return rows();
}

const core::PromptTemplate *TemplatesViewModel::find(const QString &id) const
{
    const auto wanted = id.toStdString();
    const auto found =
        std::find_if(rows_.begin(), rows_.end(),
                     [&wanted](const core::PromptTemplate &row) { return row.id == wanted; });
    return found == rows_.end() ? nullptr : &*found;
}

std::string TemplatesViewModel::shippedIn() const
{
    return contentLanguage_.toStdString();
}

void TemplatesViewModel::setContentLanguage(QString language)
{
    if (language.isEmpty() || language == contentLanguage_) {
        return;
    }
    contentLanguage_ = std::move(language);

    Q_EMIT rowsChanged();
    Q_EMIT draftChanged();
}

bool TemplatesViewModel::complete() const
{

    if (!realtime() && draft_.body.trimmed().isEmpty()) {
        return false;
    }
    return !draft_.name.trimmed().isEmpty() && refusal().isEmpty();
}

QString TemplatesViewModel::refusal() const
{

    auto row = asRow();
    core::PromptContext nothing;

    const auto rendered = core::renderPrompt(row, nothing, {});
    if (rendered.unknown.empty()) {
        return {};
    }
    return tr("There is no such placeholder as {{%1}}. What there is, is under the box.")
        .arg(QString::fromStdString(rendered.unknown.front()));
}

bool TemplatesViewModel::restorable() const
{
    if (draft_.code.isEmpty()) {
        return false;
    }
    const auto shipped = core::shippedTemplate(draft_.code.toStdString(), shippedIn());
    if (!shipped.has_value()) {
        return false;
    }
    return QString::fromStdString(shipped->body) != draft_.body ||
           QString::fromStdString(shipped->system) != draft_.system;
}

void TemplatesViewModel::setName(const QString &value)
{
    if (draft_.name == value) {
        return;
    }

    draft_.name = value;
    Q_EMIT draftChanged();
}

void TemplatesViewModel::setKind(const QString &value)
{
    if (draft_.kind == value) {
        return;
    }
    draft_.kind = value;
    Q_EMIT draftChanged();
}

void TemplatesViewModel::setShape(const QString &value)
{
    if (draft_.shape == value) {
        return;
    }
    draft_.shape = value;
    Q_EMIT draftChanged();
}

void TemplatesViewModel::setSystem(const QString &value)
{
    if (draft_.system == value) {
        return;
    }
    draft_.system = value;
    Q_EMIT draftChanged();
}

void TemplatesViewModel::setBody(const QString &value)
{
    if (draft_.body == value) {
        return;
    }
    draft_.body = value;
    Q_EMIT draftChanged();
}

void TemplatesViewModel::setPolicy(const QString &value)
{
    if (draft_.policy == value) {
        return;
    }
    draft_.policy = value;
    Q_EMIT draftChanged();
}

void TemplatesViewModel::setLanguage(const QString &value)
{
    if (draft_.language == value) {
        return;
    }
    draft_.language = value;
    Q_EMIT draftChanged();
}

void TemplatesViewModel::setParams(const QString &value)
{
    if (draft_.params == value) {
        return;
    }
    draft_.params = value;
    Q_EMIT draftChanged();
}

void TemplatesViewModel::setSwitchedOn(bool value)
{
    if (draft_.enabled == value) {
        return;
    }
    draft_.enabled = value;
    Q_EMIT draftChanged();
}

namespace {

[[nodiscard]] QString asQt(std::string_view text)
{
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

}

void TemplatesViewModel::setRealtimeNear(bool value)
{
    if (draft_.realtimeNear == value) {
        return;
    }
    draft_.realtimeNear = value;
    Q_EMIT draftChanged();
}

void TemplatesViewModel::setAsrProviderId(const QString &value)
{
    if (draft_.asrProviderId == value) {
        return;
    }
    draft_.asrProviderId = value;
    Q_EMIT draftChanged();
}

void TemplatesViewModel::setTurnPolicy(const QString &value)
{
    if (draft_.turnPolicy == value) {
        return;
    }
    draft_.turnPolicy = value;
    Q_EMIT draftChanged();
}

QVariantList TemplatesViewModel::turnPolicies() const
{
    QVariantList out;

    const auto add = [&out](const char *id, const QString &label) {
        QVariantMap entry;
        entry.insert(QStringLiteral("id"), QString::fromLatin1(id));
        entry.insert(QStringLiteral("label"), label);
        out.push_back(entry);
    };

    add("provider", tr("The recogniser decides"));

    add("silence", tr("After a pause"));

    add("manual", tr("Only when I ask"));

    return out;
}

QVariantList TemplatesViewModel::recognisers() const
{
    QVariantList out;

    QVariantMap standing;
    standing.insert(QStringLiteral("id"), QString{});

    standing.insert(QStringLiteral("label"), tr("Default"));
    out.push_back(standing);

    for (const auto &one : recognisers_) {
        if (!one.enabled) {
            continue;
        }
        QVariantMap entry;
        entry.insert(QStringLiteral("id"), QString::fromStdString(one.id));
        const auto named = one.name.empty() ? one.type : one.name;
        entry.insert(QStringLiteral("label"), QString::fromStdString(named));
        out.push_back(entry);
    }
    return out;
}

bool TemplatesViewModel::asksAModel() const
{
    return !draft_.body.trimmed().isEmpty();
}

void TemplatesViewModel::setRecognisers(std::vector<core::ProviderConfig> recognisers)
{
    recognisers_ = std::move(recognisers);
    Q_EMIT recognisersChanged();
}

void TemplatesViewModel::openOn(const core::PromptTemplate *row)
{
    draft_ = Draft{};
    preview_.clear();
    previewProblem_.clear();

    if (row != nullptr) {
        draft_.name = nameFor(QString::fromStdString(row->code), QString::fromStdString(row->name));
        draft_.kind = QString::fromUtf8(core::toString(row->kind).data(),
                                        static_cast<qsizetype>(core::toString(row->kind).size()));
        draft_.shape = QString::fromUtf8(core::toString(row->shape).data(),
                                         static_cast<qsizetype>(core::toString(row->shape).size()));
        draft_.system = QString::fromStdString(row->system);
        draft_.body = QString::fromStdString(row->body);
        draft_.policy =
            QString::fromUtf8(core::toString(row->languagePolicy).data(),
                              static_cast<qsizetype>(core::toString(row->languagePolicy).size()));
        draft_.language = QString::fromStdString(row->language);
        draft_.params = paramsText(row->params);
        draft_.providerId = QString::fromStdString(row->providerId);
        draft_.instructions.clear();
        for (const auto &code : row->extraInstructions) {
            draft_.instructions.push_back(QString::fromStdString(code));
        }

        draft_.realtimeNear = row->realtimeNear;
        draft_.asrProviderId = QString::fromStdString(row->asrProviderId);
        draft_.turnPolicy = asQt(core::toString(row->turnPolicy));

        draft_.enabled = row->enabled;
        draft_.builtIn = row->builtIn;
        draft_.code = QString::fromStdString(row->code);
    }

    editingId_ = row != nullptr ? QString::fromStdString(row->id) : QString{};
    editing_ = true;

    Q_EMIT editingChanged();
    Q_EMIT draftChanged();
    Q_EMIT previewChanged();
}

void TemplatesViewModel::add()
{
    openOn(nullptr);

    draft_.body = QStringLiteral("{{transcript}}");
    Q_EMIT draftChanged();
}

void TemplatesViewModel::addRealtime()
{

    add();
    setKind(QStringLiteral("realtime"));

    setBody(QStringLiteral("{{reply}}"));

    setRealtimeNear(true);
}

void TemplatesViewModel::edit(const QString &id)
{
    if (const auto *row = find(id); row != nullptr) {
        openOn(row);
    }
}

void TemplatesViewModel::cancel()
{
    if (!editing_) {
        return;
    }
    editing_ = false;
    editingId_.clear();
    draft_ = Draft{};
    preview_.clear();
    previewProblem_.clear();
    Q_EMIT editingChanged();
    Q_EMIT draftChanged();
    Q_EMIT previewChanged();
}

core::PromptTemplate TemplatesViewModel::asRow() const
{
    core::PromptTemplate row;
    if (const auto *existing = find(editingId_); existing != nullptr) {
        row = *existing;
    }

    row.name = draft_.name.trimmed().toStdString();
    row.kind = core::parseArtifactKind(draft_.kind.toStdString()).value_or(row.kind);
    row.shape = core::parseAnswerShape(draft_.shape.toStdString()).value_or(row.shape);
    row.system = draft_.system.trimmed().toStdString();

    row.body = draft_.body.trimmed().toStdString();
    row.languagePolicy =
        core::parseLanguagePolicy(draft_.policy.toStdString()).value_or(row.languagePolicy);
    row.language = draft_.language.trimmed().toStdString();
    row.params = paramsFrom(draft_.params);
    row.providerId = draft_.providerId.toStdString();
    row.extraInstructions.clear();
    for (const auto &code : draft_.instructions) {
        row.extraInstructions.push_back(code.toStdString());
    }
    row.realtimeNear = draft_.realtimeNear;
    row.asrProviderId = draft_.asrProviderId.toStdString();
    row.turnPolicy =
        core::parseTurnPolicy(draft_.turnPolicy.toStdString()).value_or(row.turnPolicy);

    row.enabled = draft_.enabled;
    row.code = draft_.code.toStdString();
    row.builtIn = draft_.builtIn;

    row.realtimeMode = core::modeFor(row);

    if (editingId_.isEmpty()) {
        int after = -1;
        for (const auto &one : rows_) {
            if (core::isRealtime(one) == core::isRealtime(row)) {
                after = std::max(after, one.position);
            }
        }
        row.position = after + 1;
    }
    return row;
}

void TemplatesViewModel::save()
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

void TemplatesViewModel::remove(const QString &id)
{
    if (editingId_ == id) {
        cancel();
    }
    if (remove_) {
        remove_(id);
    }
}

void TemplatesViewModel::move(const QString &id, int delta)
{
    if (!order_ || delta == 0) {
        return;
    }

    const auto *row = find(id);
    if (row == nullptr) {
        return;
    }

    const bool realtime = core::isRealtime(*row);
    std::vector<const core::PromptTemplate *> family;
    for (const auto &one : rows_) {
        if (core::isRealtime(one) == realtime) {
            family.push_back(&one);
        }
    }

    const auto at =
        std::find_if(family.begin(), family.end(), [&id](const core::PromptTemplate *one) {
            return QString::fromStdString(one->id) == id;
        });
    if (at == family.end()) {
        return;
    }

    const auto from = static_cast<int>(std::distance(family.begin(), at));
    const auto to = from + (delta < 0 ? -1 : 1);
    if (to < 0 || to >= static_cast<int>(family.size())) {

        return;
    }
    std::swap(family[static_cast<std::size_t>(from)], family[static_cast<std::size_t>(to)]);

    std::vector<std::pair<std::string, int>> places;
    places.reserve(family.size());
    for (std::size_t place = 0; place < family.size(); ++place) {
        places.emplace_back(family[place]->id, static_cast<int>(place));
    }
    order_(std::move(places));
}

void TemplatesViewModel::duplicate()
{
    if (!editing_) {
        return;
    }

    auto copy = draft_;
    copy.code.clear();
    copy.builtIn = false;

    copy.name = tr("%1 (a copy)").arg(draft_.name);

    editingId_.clear();
    draft_ = std::move(copy);
    preview_.clear();
    previewProblem_.clear();

    Q_EMIT editingChanged();
    Q_EMIT draftChanged();
    Q_EMIT previewChanged();
}

void TemplatesViewModel::restore()
{
    const auto shipped = core::shippedTemplate(draft_.code.toStdString(), shippedIn());
    if (!shipped.has_value()) {
        return;
    }

    draft_.system = QString::fromStdString(shipped->system);
    draft_.body = QString::fromStdString(shipped->body);
    draft_.shape = QString::fromUtf8(core::toString(shipped->shape).data(),
                                     static_cast<qsizetype>(core::toString(shipped->shape).size()));
    draft_.name =
        nameFor(QString::fromStdString(shipped->code), QString::fromStdString(shipped->name));
    Q_EMIT draftChanged();
}

void TemplatesViewModel::askForPreview()
{
    if (askPreview_) {
        askPreview_(asRow());
    }
}

QVariantMap TemplatesViewModel::rowFor(const QString &id) const
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

void TemplatesViewModel::setTemplates(std::vector<core::PromptTemplate> rows)
{
    rows_ = std::move(rows);
    Q_EMIT rowsChanged();
    Q_EMIT draftChanged();
}

void TemplatesViewModel::setPreview(const QString &text, const QString &problem,
                                    const QString &source)
{
    preview_ = text;
    previewProblem_ = problem;
    previewSource_ = source;
    Q_EMIT previewChanged();
}

QVariantList TemplatesViewModel::models() const
{
    QVariantList out;

    QVariantMap standing;
    standing.insert(QStringLiteral("id"), QString{});

    standing.insert(QStringLiteral("label"), tr("Default"));
    out.push_back(standing);

    for (const auto &model : models_) {
        if (!model.enabled) {
            continue;
        }
        QVariantMap entry;
        entry.insert(QStringLiteral("id"), QString::fromStdString(model.id));

        const auto named = model.name.empty() ? model.model : model.name;
        entry.insert(QStringLiteral("label"), QString::fromStdString(named));
        out.push_back(entry);
    }
    return out;
}

void TemplatesViewModel::setModels(std::vector<core::ProviderConfig> models)
{
    models_ = std::move(models);
    Q_EMIT modelsChanged();
}

void TemplatesViewModel::setInstructionEntries(std::vector<core::TaxonomyEntry> entries)
{
    instructionEntries_ = std::move(entries);
    Q_EMIT draftChanged();
}

QString TemplatesViewModel::shapeInstructionTitle() const
{
    const auto shape = core::parseAnswerShape(draft_.shape.toStdString());
    if (!shape.has_value()) {
        return {};
    }
    const auto wanted = std::string{core::instructionCode(*shape)};
    const auto found =
        std::find_if(instructionEntries_.begin(), instructionEntries_.end(),
                     [&wanted](const core::TaxonomyEntry &one) { return one.code == wanted; });
    if (found == instructionEntries_.end()) {
        return {};
    }
    return TaxonomyViewModel::titleFor(QString::fromStdString(found->code),
                                       QString::fromStdString(found->title), found->builtIn);
}

QVariantList TemplatesViewModel::instructions() const
{

    const auto shape = core::parseAnswerShape(draft_.shape.toStdString());
    const auto sent =
        shape.has_value() ? std::string{core::instructionCode(*shape)} : std::string{};

    QVariantList out;
    for (const auto &entry : instructionEntries_) {
        if (entry.code == sent || entry.retired()) {
            continue;
        }
        const auto code = QString::fromStdString(entry.code);
        QVariantMap one;
        one["id"] = code;
        one["label"] =
            TaxonomyViewModel::titleFor(code, QString::fromStdString(entry.title), entry.builtIn);
        one["chosen"] = draft_.instructions.contains(code);
        out.push_back(one);
    }
    return out;
}

void TemplatesViewModel::toggleInstruction(const QString &code, bool wanted)
{
    const auto has = draft_.instructions.contains(code);
    if (has == wanted) {
        return;
    }
    if (wanted) {
        draft_.instructions.push_back(code);
    } else {
        draft_.instructions.removeAll(code);
    }
    Q_EMIT draftChanged();
}

void TemplatesViewModel::setProviderId(const QString &value)
{
    if (draft_.providerId == value) {
        return;
    }
    draft_.providerId = value;
    Q_EMIT draftChanged();
}

void TemplatesViewModel::restoreEverything()
{
    if (restoreAll_) {
        cancel();
        restoreAll_();
    }
}

void TemplatesViewModel::setActions(Save save, Remove remove, Preview preview, Plain refresh,
                                    Plain restoreAll, Order order)
{
    save_ = std::move(save);
    remove_ = std::move(remove);
    askPreview_ = std::move(preview);
    refresh_ = std::move(refresh);
    restoreAll_ = std::move(restoreAll);
    order_ = std::move(order);
    if (refresh_) {
        refresh_();
    }
}

}
