// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "providers_view_model.hpp"

#include <softphone/core/asr.hpp>
#include <softphone/core/http.hpp>
#include <softphone/core/llm_dialect.hpp>
#include <softphone/core/streaming_asr.hpp>

#include <QLocale>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <utility>

namespace softphone::ui {
namespace {

struct Kind {
    const char *id;
    const char *endpoint;
    const char *model;

    bool modelless;

    const char *countries;
};

constexpr const char *kYandexCountries = "RU BY KZ UZ KG TJ TM AM AZ";

constexpr Kind kRecognisers[] = {

    {"openai", "https://api.openai.com/v1", "gpt-4o-transcribe", false, ""},
    {"deepgram", "https://api.deepgram.com/v1", "nova-3", false, ""},
    {"elevenlabs", "https://api.elevenlabs.io/v1", "scribe_v1", false, ""},
    {"speechmatics", "https://asr.api.speechmatics.com/v2", "enhanced", false, ""},
    {"xai", "https://api.x.ai/v1", "", true, ""},
    {"yandex", "https://stt.api.cloud.yandex.net", "general", false, kYandexCountries},

    {"soniox", "https://api.soniox.com", "", false, ""},
    {"assemblyai", "https://api.assemblyai.com", "", false, ""},

    {"vosk", "", "", false, ""},
    {"whisper-live", "", "", false, ""},
    {"riva", "", "", false, ""},
};

constexpr Kind kModels[] = {
    {"openai", "http://127.0.0.1:11434/v1", "qwen2.5:7b-instruct", false, ""},
    {"anthropic", "https://api.anthropic.com/v1", "claude-sonnet-4-20250514", false, ""},

    {"xai", "https://api.x.ai/v1", "grok-4.6", false, ""},

    {"yandex", "https://ai.api.cloud.yandex.net/v1", "yandexgpt/latest", false, kYandexCountries},
};

[[nodiscard]] bool offeredIn(const Kind &kind, const QString &country)
{
    const QString countries = QString::fromLatin1(kind.countries);
    if (countries.isEmpty()) {
        return true;
    }
    return !country.isEmpty() && countries.split(QLatin1Char{' '}).contains(country);
}

[[nodiscard]] std::span<const Kind> typesFor(core::ProviderKind kind)
{
    return kind == core::ProviderKind::Llm ? std::span<const Kind>{kModels}
                                           : std::span<const Kind>{kRecognisers};
}

[[nodiscard]] const Kind *typeOf(std::span<const Kind> known, const QString &id)
{
    for (const auto &kind : known) {
        if (id == QLatin1String{kind.id}) {
            return &kind;
        }
    }
    return nullptr;
}

[[nodiscard]] std::int64_t priceMicros(const QString &text)
{
    const auto trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return 0;
    }
    bool ok = false;
    const auto value = trimmed.toDouble(&ok);
    if (!ok || value <= 0.0) {
        return 0;
    }
    return static_cast<std::int64_t>(std::llround(value * 1'000'000.0));
}

[[nodiscard]] QString priceText(std::int64_t micros)
{
    if (micros <= 0) {
        return {};
    }
    auto text = QString::number(static_cast<double>(micros) / 1'000'000.0, 'f', 6);
    while (text.endsWith(QLatin1Char{'0'})) {
        text.chop(1);
    }
    if (text.endsWith(QLatin1Char{'.'})) {
        text.chop(1);
    }
    return text;
}

[[nodiscard]] QString oneLine(const QString &secret)
{
    return QString::fromStdString(core::asOneLine(secret.trimmed().toStdString()));
}

[[nodiscard]] std::vector<std::pair<std::string, std::string>> paramsFrom(const QString &text)
{
    std::vector<std::pair<std::string, std::string>> params;
    const auto lines = text.split(QLatin1Char{'\n'}, Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        const auto at = line.indexOf(QLatin1Char{'='});
        if (at <= 0) {
            continue;
        }
        const auto name = line.left(at).trimmed();
        const auto value = line.mid(at + 1).trimmed();
        if (!name.isEmpty()) {
            params.emplace_back(name.toStdString(), value.toStdString());
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

[[nodiscard]] std::vector<std::pair<std::string, std::string>>
extrasOnly(const std::vector<std::pair<std::string, std::string>> &params,
           const std::vector<core::ProviderKnob> &knobs)
{
    std::vector<std::pair<std::string, std::string>> extras;
    for (const auto &one : params) {

        if (!core::isKnob(knobs, one.first) && !core::isDialectName(one.first)) {
            extras.push_back(one);
        }
    }
    return extras;
}

[[nodiscard]] QString kindName(core::KnobKind kind)
{
    switch (kind) {
    case core::KnobKind::Number:
        return QStringLiteral("number");
    case core::KnobKind::Decimal:
        return QStringLiteral("decimal");
    case core::KnobKind::Flag:
        return QStringLiteral("flag");
    case core::KnobKind::Text:
        break;
    }
    return QStringLiteral("text");
}

}

ProvidersViewModel::ProvidersViewModel(QObject *parent) : QObject{parent} {}

QVariantList ProvidersViewModel::rows() const
{
    QVariantList out;
    out.reserve(static_cast<qsizetype>(rows_.size()));
    for (const auto &row : rows_) {
        QVariantMap entry;
        entry["id"] = QString::fromStdString(row.id);
        entry["name"] = QString::fromStdString(row.name);
        entry["type"] = QString::fromStdString(row.type);
        entry["endpoint"] = QString::fromStdString(row.endpoint);
        entry["model"] = QString::fromStdString(row.model);
        entry["isDefault"] = row.isDefault;
        entry["isRealtimeDefault"] = row.isRealtimeDefault;
        entry["enabled"] = row.enabled;
        entry["hasSecret"] = row.hasSecret;

        const auto listening = kind_ == core::ProviderKind::Asr;
        entry["transcribes"] = listening && core::canTranscribeType(row.type);
        entry["listens"] = listening && core::canStreamType(row.type);
        out.push_back(entry);
    }
    return out;
}

QVariantList ProvidersViewModel::rowsNow() const
{
    return rows();
}

QString ProvidersViewModel::labelForType(const QString &id) const
{

    if (id == QLatin1String{"deepgram"}) {

        return tr("Deepgram");
    }
    if (id == QLatin1String{"elevenlabs"}) {

        return tr("ElevenLabs (Scribe)");
    }
    if (id == QLatin1String{"speechmatics"}) {

        return tr("Speechmatics");
    }
    if (id == QLatin1String{"yandex"}) {
        return kind_ == core::ProviderKind::Llm

                   ? tr("Yandex AI Studio (YandexGPT)")

                   : tr("Yandex SpeechKit");
    }
    if (id == QLatin1String{"xai"}) {

        return tr("xAI (Grok)");
    }
    if (id == QLatin1String{"soniox"}) {

        return tr("Soniox");
    }
    if (id == QLatin1String{"assemblyai"}) {

        return tr("AssemblyAI");
    }
    if (id == QLatin1String{"vosk"}) {

        return tr("Vosk (on your own machine)");
    }
    if (id == QLatin1String{"whisper-live"}) {

        return tr("WhisperLive (on your own machine)");
    }
    if (id == QLatin1String{"riva"}) {

        return tr("NVIDIA Riva (on your own machine)");
    }
    if (id == QLatin1String{"anthropic"}) {

        return tr("Anthropic (Claude)");
    }
    if (kind_ == core::ProviderKind::Llm) {

        return tr("OpenAI-compatible (Ollama, vLLM, OpenAI)");
    }

    return tr("OpenAI-compatible (Whisper, OpenAI)");
}

void ProvidersViewModel::setKind(core::ProviderKind kind)
{
    kind_ = kind;
}

void ProvidersViewModel::setCountry(const QString &country)
{
    if (country_ == country) {
        return;
    }
    country_ = country;
    Q_EMIT typesChanged();
}

QString ProvidersViewModel::kindText() const
{
    const auto text = core::toString(kind_);
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

QVariantList ProvidersViewModel::types() const
{
    QVariantList out;
    for (const auto &kind : typesFor(kind_)) {

        if (!offeredIn(kind, country_) && !(editing_ && draft_.type == QLatin1String{kind.id})) {
            continue;
        }
        QVariantMap entry;
        const auto id = QString::fromLatin1(kind.id);
        entry["id"] = id;
        entry["label"] = labelForType(id);
        entry["endpoint"] = QString::fromLatin1(kind.endpoint);
        entry["model"] = QString::fromLatin1(kind.model);

        const auto listening = kind_ == core::ProviderKind::Asr;
        entry["transcribes"] = listening && core::canTranscribeType(kind.id);
        entry["listens"] = listening && core::canStreamType(kind.id);
        out.push_back(entry);
    }
    return out;
}

QVariantList ProvidersViewModel::usable() const
{
    QVariantList out;
    for (const auto &row : rows_) {
        if (!row.enabled) {
            continue;
        }
        QVariantMap entry;
        entry["id"] = QString::fromStdString(row.id);
        entry["name"] = QString::fromStdString(row.name);
        entry["isDefault"] = row.isDefault;

        if (row.isDefault) {
            out.prepend(entry);
        } else {
            out.push_back(entry);
        }
    }
    return out;
}

bool ProvidersViewModel::complete() const
{
    return !draft_.name.trimmed().isEmpty() && !draft_.endpoint.trimmed().isEmpty() &&
           (!modelRequired() || !draft_.model.trimmed().isEmpty());
}

bool ProvidersViewModel::modelRequired() const
{

    return false;
}

bool ProvidersViewModel::needsModel() const
{

    const auto *kind = typeOf(typesFor(kind_), draft_.type);
    return kind == nullptr || !kind->modelless;
}

void ProvidersViewModel::setName(const QString &value)
{
    if (draft_.name == value) {
        return;
    }

    draft_.name = value;
    Q_EMIT draftChanged();
}

void ProvidersViewModel::setType(const QString &value)
{
    if (draft_.type == value) {
        return;
    }
    draft_.type = value;
    if (const auto *kind = typeOf(typesFor(kind_), value);
        kind != nullptr && editingId_.isEmpty()) {

        draft_.endpoint = QString::fromLatin1(kind->endpoint);

        draft_.realtimeEndpoint = liveAddress(draft_.type);

        if (kind_ == core::ProviderKind::Asr) {
            draft_.realtimeModel =
                QString::fromStdString(core::preferredModelFor(draft_.type.toStdString(), true));
        }
    }

    if (kind_ != core::ProviderKind::Llm && !needsModel()) {
        draft_.model.clear();
    }

    seedKnobs(paramsFrom(draft_.params));
    clearTest();
    Q_EMIT draftChanged();
}

void ProvidersViewModel::setEndpoint(const QString &value)
{
    if (draft_.endpoint == value) {
        return;
    }
    draft_.endpoint = value;
    clearTest();
    Q_EMIT draftChanged();
}

void ProvidersViewModel::setRealtimeEndpoint(const QString &value)
{
    if (draft_.realtimeEndpoint == value) {
        return;
    }
    draft_.realtimeEndpoint = value;
    clearTest();
    Q_EMIT draftChanged();
}

void ProvidersViewModel::setRealtimeModel(const QString &value)
{
    if (draft_.realtimeModel == value) {
        return;
    }
    draft_.realtimeModel = value;
    clearTest();
    Q_EMIT draftChanged();
}

void ProvidersViewModel::setLanguage(const QString &value)
{
    if (draft_.language == value) {
        return;
    }
    draft_.language = value;
    Q_EMIT draftChanged();
}

void ProvidersViewModel::setModel(const QString &value)
{
    if (draft_.model == value) {
        return;
    }
    draft_.model = value;
    Q_EMIT draftChanged();
}

void ProvidersViewModel::setRegion(const QString &value)
{
    if (draft_.region == value) {
        return;
    }
    draft_.region = value;
    Q_EMIT draftChanged();
}

QVariantList ProvidersViewModel::knobs() const
{
    const auto declared = core::knobsFor(kind_, draft_.type.toStdString());

    std::vector<std::pair<std::string, std::string>> corrections;
    corrections.reserve(draft_.learnt.size());
    for (const auto &[name, value] : draft_.learnt) {
        corrections.emplace_back(name.toStdString(), value.toStdString());
    }
    const auto wants = kind_ == core::ProviderKind::Llm
                           ? core::dialectFor(draft_.type.toStdString(),
                                              draft_.model.trimmed().toStdString(), corrections)
                           : core::LlmDialect{};

    QVariantList list;
    list.reserve(static_cast<qsizetype>(declared.size()));
    for (const auto &knob : declared) {
        QVariantMap one;
        one["name"] = QString::fromStdString(knob.name);
        one["kind"] = kindName(knob.kind);
        one["value"] = knobValue(QString::fromStdString(knob.name));
        one["fallback"] = QString::fromStdString(knob.fallback);

        one["clearable"] = knob.optional;

        one["sent"] = knob.sent;

        one["ceiling"] = wants.temperatureMax;

        one["accepted"] =
            !(kind_ == core::ProviderKind::Llm && knob.name == "temperature" && !wants.temperature);
        list.push_back(one);
    }
    return list;
}

QString ProvidersViewModel::knobValue(const QString &name) const
{
    const auto found = std::find_if(draft_.knobs.begin(), draft_.knobs.end(),
                                    [&name](const auto &one) { return one.first == name; });
    return found == draft_.knobs.end() ? QString{} : found->second;
}

void ProvidersViewModel::setKnob(const QString &name, const QString &value)
{
    const auto found = std::find_if(draft_.knobs.begin(), draft_.knobs.end(),
                                    [&name](const auto &one) { return one.first == name; });
    if (found == draft_.knobs.end() || found->second == value) {
        return;
    }
    found->second = value;
    clearTest();

    Q_EMIT draftChanged();
}

void ProvidersViewModel::seedKnobs(const std::vector<std::pair<std::string, std::string>> &stored)
{
    draft_.knobs.clear();
    for (const auto &knob : core::knobsFor(kind_, draft_.type.toStdString())) {
        draft_.knobs.emplace_back(
            QString::fromStdString(knob.name),
            QString::fromStdString(core::knobValue(stored, {knob}, knob.name)));
    }
    Q_EMIT knobsChanged();
}

void ProvidersViewModel::setParams(const QString &value)
{
    if (draft_.params == value) {
        return;
    }
    draft_.params = value;
    Q_EMIT draftChanged();
}

void ProvidersViewModel::setToken(const QString &value)
{
    if (draft_.token == value) {
        return;
    }
    draft_.token = value;
    clearTest();
    Q_EMIT draftChanged();
}

void ProvidersViewModel::setWaitMinutes(const QString &value)
{
    if (draft_.waitMinutes == value) {
        return;
    }

    draft_.waitMinutes = value;
    Q_EMIT draftChanged();
}

void ProvidersViewModel::setRealtimePrice(const QString &value)
{
    if (draft_.realtimePrice == value) {
        return;
    }

    draft_.realtimePrice = value;
    Q_EMIT draftChanged();
}

void ProvidersViewModel::setInPrice(const QString &value)
{
    if (draft_.inPrice == value) {
        return;
    }

    draft_.inPrice = value;
    Q_EMIT draftChanged();
}

void ProvidersViewModel::setOutPrice(const QString &value)
{
    if (draft_.outPrice == value) {
        return;
    }
    draft_.outPrice = value;
    Q_EMIT draftChanged();
}

void ProvidersViewModel::setIsDefault(bool value)
{
    if (draft_.isDefault == value) {
        return;
    }
    draft_.isDefault = value;
    Q_EMIT draftChanged();
}

void ProvidersViewModel::setIsRealtimeDefault(bool value)
{
    if (draft_.isRealtimeDefault == value) {
        return;
    }
    draft_.isRealtimeDefault = value;
    Q_EMIT draftChanged();
}

void ProvidersViewModel::setSwitchedOn(bool value)
{
    if (draft_.enabled == value) {
        return;
    }
    draft_.enabled = value;
    Q_EMIT draftChanged();
}

void ProvidersViewModel::add()
{
    openOn(nullptr);
}

void ProvidersViewModel::edit(const QString &id)
{
    const auto found =
        std::find_if(rows_.begin(), rows_.end(), [&id](const core::ProviderConfig &row) {
            return QString::fromStdString(row.id) == id;
        });
    openOn(found == rows_.end() ? nullptr : &*found);
}

void ProvidersViewModel::openOn(const core::ProviderConfig *row)
{
    draft_ = Draft{};
    clearTest();

    if (row == nullptr) {
        editingId_.clear();
        const auto &kind = typesFor(kind_).front();
        draft_.type = QString::fromLatin1(kind.id);
        draft_.endpoint = QString::fromLatin1(kind.endpoint);
        draft_.realtimeEndpoint = liveAddress(draft_.type);

        draft_.isDefault = rows_.empty();
        draft_.isRealtimeDefault = rows_.empty() && core::canStreamType(draft_.type.toStdString());
        seedKnobs({});
        draft_.learnt.clear();
    } else {
        editingId_ = QString::fromStdString(row->id);
        draft_.name = QString::fromStdString(row->name);
        draft_.type = QString::fromStdString(row->type);
        draft_.endpoint = QString::fromStdString(row->endpoint);

        draft_.realtimeEndpoint = row->realtimeEndpoint.empty()
                                      ? liveAddress(draft_.type)
                                      : QString::fromStdString(row->realtimeEndpoint);

        draft_.realtimeModel = QString::fromStdString(row->realtimeModel);
        draft_.language = QString::fromStdString(row->language);
        draft_.model = QString::fromStdString(row->model);
        draft_.region = QString::fromStdString(row->region);

        seedKnobs(row->params);
        draft_.learnt.clear();
        for (const auto &[name, value] : row->params) {
            if (core::isDialectName(name)) {
                draft_.learnt.emplace_back(QString::fromStdString(name),
                                           QString::fromStdString(value));
            }
        }
        draft_.params = paramsText(extrasOnly(row->params, core::knobsFor(kind_, row->type)));

        draft_.modelsSeen.clear();
        for (const auto &one : row->modelsSeen) {
            draft_.modelsSeen.push_back(QString::fromStdString(one));
        }
        draft_.hasSecret = row->hasSecret;
        draft_.isDefault = row->isDefault;
        draft_.isRealtimeDefault = row->isRealtimeDefault;
        draft_.enabled = row->enabled;
        draft_.waitMinutes =
            row->readTimeout > std::chrono::seconds::zero()
                ? QString::number(
                      std::chrono::duration_cast<std::chrono::minutes>(row->readTimeout).count())
                : QString{};
        draft_.inPrice = priceText(row->inPricePerMillion);
        draft_.realtimePrice = priceText(row->realtimePriceMicros);
        draft_.outPrice = priceText(row->outPricePerMillion);
    }

    editing_ = true;
    Q_EMIT editingChanged();
    Q_EMIT typesChanged();
    Q_EMIT draftChanged();
}

void ProvidersViewModel::cancel()
{
    if (!editing_) {
        return;
    }
    editing_ = false;
    editingId_.clear();
    draft_ = Draft{};
    clearTest();
    Q_EMIT editingChanged();
    Q_EMIT typesChanged();
    Q_EMIT draftChanged();
    Q_EMIT knobsChanged();
}

QString ProvidersViewModel::liveAddress(const QString &type) const
{
    if (kind_ != core::ProviderKind::Asr) {
        return {};
    }

    const auto published = core::liveAddressFor(type.toStdString());
    if (!published.empty()) {
        return QString::fromUtf8(published);
    }

    const auto territory = QLocale::territoryToCode(QLocale::system().territory());
    return QString::fromUtf8(
        core::liveAddressHere(type.toStdString(), territory.toLatin1().constData()));
}

QStringList ProvidersViewModel::offeredModels() const
{

    const auto seen = testModels_.isEmpty() ? draft_.modelsSeen : testModels_;

    if (kind_ != core::ProviderKind::Asr) {
        if (!seen.isEmpty()) {
            return seen;
        }
        QStringList documented;
        for (const auto &one : core::knownChatModels(draft_.type.toStdString())) {
            documented.push_back(QString::fromStdString(one));
        }
        return documented;
    }

    if (!seen.isEmpty()) {

        QStringList forFiles;
        for (const auto &one : seen) {
            if (!core::listensLive(one.toStdString())) {
                forFiles.push_back(one);
            }
        }
        if (!forFiles.isEmpty()) {
            return forFiles;
        }

    }

    QStringList offered;
    for (const auto &one : core::knownModels(draft_.type.toStdString(), false)) {
        offered.push_back(QString::fromStdString(one));
    }
    return offered;
}

QStringList ProvidersViewModel::offeredRealtimeModels() const
{
    if (kind_ != core::ProviderKind::Asr) {
        return {};
    }

    QStringList offered;
    const auto seen = testModels_.isEmpty() ? draft_.modelsSeen : testModels_;
    for (const auto &one : seen) {
        if (core::listensLive(one.toStdString())) {
            offered.push_back(one);
        }
    }
    for (const auto &one : core::knownModels(draft_.type.toStdString(), true)) {
        const auto documented = QString::fromStdString(one);
        if (!offered.contains(documented)) {
            offered.push_back(documented);
        }
    }
    return offered;
}

QString ProvidersViewModel::addressExample() const
{

    for (const auto &one : kind_ == core::ProviderKind::Asr ? std::span<const Kind>{kRecognisers}
                                                            : std::span<const Kind>{kModels}) {
        if (draft_.type == QString::fromUtf8(one.id) && *one.endpoint != '\0') {
            return QString::fromUtf8(one.endpoint);
        }
    }

    return kind_ == core::ProviderKind::Asr ? QStringLiteral("http://127.0.0.1:8000/v1")
                                            : QStringLiteral("http://127.0.0.1:11434/v1");
}

QString ProvidersViewModel::liveAddressExample() const
{
    if (kind_ != core::ProviderKind::Asr) {
        return {};
    }
    return QString::fromUtf8(core::liveAddressFor(draft_.type.toStdString()));
}

bool ProvidersViewModel::runsHere() const
{
    return kind_ == core::ProviderKind::Asr && core::runsOnThisMachine(draft_.type.toStdString());
}

QString ProvidersViewModel::liveModelExample() const
{
    if (kind_ != core::ProviderKind::Asr) {
        return {};
    }
    return QString::fromUtf8(core::liveModelFor(draft_.type.toStdString()));
}

bool ProvidersViewModel::canStream() const
{
    return kind_ == core::ProviderKind::Asr && core::canStreamType(draft_.type.toStdString());
}

bool ProvidersViewModel::canTranscribe() const
{
    return kind_ == core::ProviderKind::Asr && core::canTranscribeType(draft_.type.toStdString());
}

bool ProvidersViewModel::modelAnswersWithoutTimes() const
{
    if (kind_ != core::ProviderKind::Asr) {
        return false;
    }
    return !core::timesComeBack(
        core::answerShapeFor(draft_.type.toStdString(), draft_.model.trimmed().toStdString()));
}

core::ProviderConfig ProvidersViewModel::asRow() const
{
    core::ProviderConfig row;
    row.id = editingId_.toStdString();
    row.kind = kind_;
    row.type = draft_.type.trimmed().toStdString();
    row.name = draft_.name.trimmed().toStdString();
    row.endpoint = draft_.endpoint.trimmed().toStdString();
    row.realtimeEndpoint = draft_.realtimeEndpoint.trimmed().toStdString();
    row.realtimeModel = draft_.realtimeModel.trimmed().toStdString();
    row.language = draft_.language.trimmed().toStdString();
    row.model = draft_.model.trimmed().toStdString();
    row.region = draft_.region.trimmed().toStdString();

    for (const auto &knob : core::knobsFor(kind_, row.type)) {
        const auto value = knobValue(QString::fromStdString(knob.name));
        if (value.toStdString() != knob.fallback) {
            row.params.emplace_back(knob.name, value.toStdString());
        }
    }

    for (const auto &[name, value] : draft_.learnt) {
        row.params.emplace_back(name.toStdString(), value.toStdString());
    }
    for (auto &extra : paramsFrom(draft_.params)) {
        row.params.push_back(std::move(extra));
    }

    row.modelsSeen.clear();
    for (const auto &one : draft_.modelsSeen) {
        row.modelsSeen.push_back(one.toStdString());
    }
    row.hasSecret = draft_.hasSecret || !draft_.token.isEmpty();
    row.enabled = draft_.enabled;
    row.isDefault = draft_.isDefault;
    row.isRealtimeDefault = draft_.isRealtimeDefault;

    if (canStream()) {
        row.realtimePriceMicros = priceMicros(draft_.realtimePrice);
    }

    if (priced()) {

        bool understood = false;
        const auto minutes = draft_.waitMinutes.trimmed().toInt(&understood);
        row.readTimeout =
            understood && minutes > 0 ? std::chrono::minutes{minutes} : std::chrono::seconds{0};

        row.inPricePerMillion = priceMicros(draft_.inPrice);
        row.outPricePerMillion = priceMicros(draft_.outPrice);

    }
    return row;
}

void ProvidersViewModel::save()
{
    if (!editing_ || !complete() || !save_) {
        return;
    }
    save_(asRow(), oneLine(draft_.token));
    cancel();
}

void ProvidersViewModel::remove(const QString &id)
{
    if (remove_) {
        remove_(id);
    }
    if (editingId_ == id) {
        cancel();
    }
}

void ProvidersViewModel::makeDefault(const QString &id, bool live, bool wanted)
{
    const auto found = std::ranges::find_if(
        rows_, [&id](const core::ProviderConfig &one) { return one.id == id.toStdString(); });
    if (found == rows_.end() || !save_) {
        return;
    }

    auto row = *found;
    (live ? row.isRealtimeDefault : row.isDefault) = wanted;

    save_(row, {});
}

void ProvidersViewModel::test()
{
    if (!test_ || draft_.endpoint.trimmed().isEmpty()) {
        return;
    }
    testState_ = QStringLiteral("trying");

    testDetail_ = tr("Asking…");
    testModels_.clear();
    Q_EMIT testChanged();

    test_(asRow(), oneLine(draft_.token));
}

void ProvidersViewModel::setTestResult(const core::LlmProbe &probe)
{

    for (const auto &[name, value] : probe.learnt) {
        const auto key = QString::fromStdString(name);
        const auto found = std::find_if(draft_.learnt.begin(), draft_.learnt.end(),
                                        [&key](const auto &pair) { return pair.first == key; });
        if (found == draft_.learnt.end()) {
            draft_.learnt.emplace_back(key, QString::fromStdString(value));
        } else {
            found->second = QString::fromStdString(value);
        }
    }
    if (!probe.learnt.empty()) {

        Q_EMIT draftChanged();
        Q_EMIT knobsChanged();
    }

    core::AsrProbe asIf;
    asIf.reachable = probe.reachable;
    asIf.detail = probe.detail;
    asIf.models = probe.models;
    setTestResult(asIf);
}

void ProvidersViewModel::setTestResult(const core::AsrProbe &probe)
{
    testModels_.clear();
    for (const auto &model : probe.models) {
        testModels_.push_back(QString::fromStdString(model));
    }

    if (probe.reachable && !testModels_.isEmpty()) {
        draft_.modelsSeen = testModels_;
        Q_EMIT draftChanged();
    }

    if (probe.reachable) {
        testState_ = QStringLiteral("reached");

        testDetail_ =
            testModels_.isEmpty() && !needsModel()

                ? tr("Answered. This recogniser has no model to choose.")
                : testModels_.isEmpty()

                      ? tr("Answered. It does not list its models — type the name.")

                      : tr("Answered, and offers %n model(s).", nullptr,
                           static_cast<int>(testModels_.size()));
    } else {
        testState_ = QStringLiteral("refused");
        const auto said = QString::fromStdString(probe.detail);
        testDetail_ = said.isEmpty()

                          ? tr("Nothing answered.")

                          : tr("Refused: %1").arg(said);
    }
    Q_EMIT testChanged();
}

void ProvidersViewModel::clearTest()
{
    if (testState_ == QStringLiteral("idle") && testDetail_.isEmpty() && testModels_.isEmpty()) {
        return;
    }
    testState_ = QStringLiteral("idle");
    testDetail_.clear();
    testModels_.clear();
    Q_EMIT testChanged();
}

void ProvidersViewModel::setProviders(std::vector<core::ProviderConfig> rows, bool available)
{
    rows_ = std::move(rows);
    available_ = available;
    Q_EMIT rowsChanged();

    if (editing_ && !editingId_.isEmpty()) {
        const auto still =
            std::any_of(rows_.begin(), rows_.end(), [this](const core::ProviderConfig &row) {
                return QString::fromStdString(row.id) == editingId_;
            });
        if (!still) {
            cancel();
        }
    }
}

void ProvidersViewModel::setActions(Save save, Remove remove, Test test, Plain refresh)
{
    save_ = std::move(save);
    remove_ = std::move(remove);
    test_ = std::move(test);
    refresh_ = std::move(refresh);
}

}
