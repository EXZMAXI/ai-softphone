// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "prompter_view_model.hpp"

#include "export_names.hpp"

#include <QCoreApplication>

#include <algorithm>
#include <utility>

namespace softphone::ui {
namespace {

[[nodiscard]] QString asStamp(std::chrono::milliseconds at)
{
    const auto whole = at.count() / 1000;
    return QStringLiteral("%1:%2").arg(whole / 60).arg(whole % 60, 2, 10, QLatin1Char{'0'});
}

}

PrompterViewModel::PrompterViewModel(QObject *parent) : QObject{parent} {}

bool PrompterViewModel::available() const noexcept
{

    return configured() && (conversation_ || rehearsing_);
}

bool PrompterViewModel::configured() const noexcept
{

    return offered_ && recogniser_ && !templates_.empty();
}

QString PrompterViewModel::unavailable() const
{

    if (!offered_) {

        return tr("Prompting is switched off. Settings → Prompter.");
    }
    if (!recogniser_) {

        return tr("No recogniser here can listen while somebody is talking. Settings → "
                  "Transcription.");
    }
    if (templates_.empty()) {

        return tr("There is nothing to run. Settings → Prompter, and add an assistant.");
    }

    return {};
}

bool PrompterViewModel::asksByHand() const
{
    const auto chosen = std::ranges::find_if(templates_, [this](const core::PromptTemplate &one) {
        return QString::fromStdString(one.id) == chosen_;
    });
    return chosen != templates_.end() && chosen->turnPolicy == core::TurnPolicy::Manual;
}

bool PrompterViewModel::suggesting() const
{

    const auto chosen = std::ranges::find_if(templates_, [this](const core::PromptTemplate &one) {
        return QString::fromStdString(one.id) == chosen_;
    });
    return chosen == templates_.end() || core::wantsAModel(*chosen);
}

QVariantList PrompterViewModel::templates() const
{
    QVariantList out;
    out.reserve(static_cast<qsizetype>(templates_.size()));
    for (const auto &one : templates_) {
        QVariantMap entry;
        entry.insert(QStringLiteral("id"), QString::fromStdString(one.id));
        entry.insert(QStringLiteral("label"), QString::fromStdString(one.name));
        entry.insert(
            QStringLiteral("mode"),
            QString::fromUtf8(core::toString(one.realtimeMode).data(),
                              static_cast<qsizetype>(core::toString(one.realtimeMode).size())));
        out.push_back(entry);
    }
    return out;
}

QString PrompterViewModel::heading() const
{
    QStringList parts;
    if (!templateName_.isEmpty()) {
        parts << templateName_;
    }
    if (!recogniserName_.isEmpty()) {
        parts << recogniserName_;
    }
    if (!modelName_.isEmpty()) {
        parts << modelName_;
    }
    return parts.join(QStringLiteral(" · "));
}

QString PrompterViewModel::costLine() const
{
    if (recogniserMicros_ == 0 && modelMicros_ == 0) {
        return {};
    }

    const auto money = [](std::int64_t micros) {
        return QString::number(static_cast<double>(micros) / 1'000'000.0, 'f', 2);
    };

    const auto line =
        tr("%1 listening, %2 asking").arg(money(recogniserMicros_), money(modelMicros_));

    return estimated_ ? tr("%1 (estimated)").arg(line) : line;
}

QString PrompterViewModel::waitingLine() const
{
    if (!waiting_) {
        return {};
    }

    return tr("Waiting for the call to be answered.");
}

QString PrompterViewModel::trouble() const
{

    if (trouble_ == QStringLiteral("model-ceiling")) {

        return tr("This month's allowance for models is used up. The words carry on; the "
                  "prompting has stopped.");
    }
    if (trouble_ == QStringLiteral("recogniser-ceiling")) {

        return tr("This month's allowance for recognisers is used up. This prompter stopped at "
                  "the end of the last reply.");
    }
    if (trouble_ == QStringLiteral("recogniser-refused")) {

        return tr("The recogniser did not start listening. Check its live address and its "
                  "model under Settings → Transcription.");
    }
    if (trouble_ == QStringLiteral("recogniser-unavailable")) {

        return tr("The recogniser this prompt names cannot listen now: it is switched off, or "
                  "it cannot listen while somebody is talking. Choose another on the prompt, "
                  "under Settings → Prompter.");
    }
    if (trouble_ == QStringLiteral("no-recogniser")) {

        return tr("No recogniser here can listen as a conversation happens. Add one under "
                  "Settings → Transcription.");
    }
    if (trouble_ == QStringLiteral("nobody-was-told")) {

        return tr("The other party has to be told first. Start recording this conversation, "
                  "or change what Settings → Recording says about consent.");
    }
    if (trouble_ == QStringLiteral("model-refused")) {

        return tr("No suggestion came back this time. The words are still arriving.");
    }
    if (trouble_ == QStringLiteral("rehearsal-gone")) {

        return tr("That conversation is not in the library any more. Choose another.");
    }
    if (trouble_ == QStringLiteral("rehearsal-unreadable")) {

        return tr("There is no sound in that file this phone can read. It takes mp3 and "
                  "wav.");
    }
    return {};
}

QVariantList PrompterViewModel::said() const
{
    auto out = said_;

    if (!provisionalFar_.isEmpty()) {
        out.push_back(provisionalFar_);
    }
    if (!provisionalNear_.isEmpty()) {
        out.push_back(provisionalNear_);
    }
    return out;
}

QVariantList PrompterViewModel::answers() const
{
    return answers_;
}

void PrompterViewModel::setTemplates(std::vector<core::PromptTemplate> rows)
{

    std::vector<core::PromptTemplate> mine;
    for (auto &one : rows) {
        if (core::isRealtime(one) && one.enabled) {
            mine.push_back(std::move(one));
        }
    }

    templates_ = std::move(mine);

    if (!chosen_.isEmpty() &&
        std::ranges::none_of(templates_, [this](const core::PromptTemplate &one) {
            return QString::fromStdString(one.id) == chosen_;
        })) {
        chosen_.clear();
        Q_EMIT chosenChanged();
    }

    if (chosen_.isEmpty() && !templates_.empty()) {
        chosen_ = QString::fromStdString(templates_.front().id);
        Q_EMIT chosenChanged();
    }

    Q_EMIT templatesChanged();
    Q_EMIT readyChanged();
}

void PrompterViewModel::setReady(bool offered, bool conversation, bool recogniser)
{
    if (offered == offered_ && conversation == conversation_ && recogniser == recogniser_) {
        return;
    }
    offered_ = offered;
    conversation_ = conversation;
    recogniser_ = recogniser;
    Q_EMIT readyChanged();
}

void PrompterViewModel::setRunning(bool running)
{

    const bool stale = (running && waiting_) || asked_;
    if (running == running_ && !stale) {
        return;
    }
    running_ = running;
    waiting_ = waiting_ && !running;

    asked_ = false;
    Q_EMIT runningChanged();
}

void PrompterViewModel::setWaiting(bool waiting)
{
    if (waiting == waiting_) {
        return;
    }
    waiting_ = waiting;

    asked_ = false;
    Q_EMIT runningChanged();
}

QString PrompterViewModel::saidAsText() const
{
    QString out;
    for (const auto &row : said()) {
        const auto one = row.toMap();
        if (one.value(QStringLiteral("provisional")).toBool()) {
            continue;
        }
        out +=
            QStringLiteral("%1  %2  %3\n")
                .arg(one.value(QStringLiteral("at")).toString(),
                     sideWord(one.value(QStringLiteral("near")).toBool() ? QStringLiteral("near")
                                                                         : QStringLiteral("far")),
                     one.value(QStringLiteral("text")).toString());
    }
    return out;
}

QString PrompterViewModel::answersAsText() const
{
    QString out;
    for (const auto &row : answers_) {
        const auto one = row.toMap();
        out += QStringLiteral("%1  %2\n")
                   .arg(one.value(QStringLiteral("at")).toString(),
                        one.value(QStringLiteral("text")).toString());
    }
    return out;
}

void PrompterViewModel::setSubject(QString who)
{
    subject_ = std::move(who);
}

QString PrompterViewModel::exportName(bool asCsv) const
{

    const auto stem = conversationStem(QDateTime::currentDateTime(), subject_);
    return stem + (asCsv ? QStringLiteral(".csv") : QStringLiteral(".txt"));
}

void PrompterViewModel::setNames(QString templateName, QString recogniserName, QString modelName)
{
    templateName_ = std::move(templateName);
    recogniserName_ = std::move(recogniserName);
    modelName_ = std::move(modelName);
    Q_EMIT runningChanged();
}

void PrompterViewModel::heardPartial(bool near, QString text)
{
    QVariantMap row;
    row.insert(QStringLiteral("text"), std::move(text));
    row.insert(QStringLiteral("at"), QString{});
    row.insert(QStringLiteral("near"), near);
    row.insert(QStringLiteral("provisional"), true);
    row.insert(QStringLiteral("gap"), 0);

    if (near) {
        provisionalNear_ = std::move(row);
    } else {
        provisionalFar_ = std::move(row);
    }
    Q_EMIT saidChanged();
}

void PrompterViewModel::heardTurn(bool near, QString text, std::chrono::milliseconds at)
{
    QVariantMap row;
    row.insert(QStringLiteral("text"), std::move(text));
    row.insert(QStringLiteral("at"), asStamp(at));

    row.insert(QStringLiteral("atMs"), static_cast<qlonglong>(at.count()));
    row.insert(QStringLiteral("near"), near);
    row.insert(QStringLiteral("provisional"), false);

    row.insert(QStringLiteral("gap"), static_cast<int>(pendingGap_.count()));
    pendingGap_ = std::chrono::milliseconds::zero();

    auto where = said_.end();
    while (where != said_.begin() &&
           (where - 1)->toMap().value(QStringLiteral("atMs")).toLongLong() > at.count()) {
        --where;
    }
    said_.insert(where, std::move(row));

    if (near) {
        provisionalNear_.clear();
    } else {
        provisionalFar_.clear();
    }
    Q_EMIT saidChanged();
}

void PrompterViewModel::heardAnswer(QString text, int covers, std::chrono::milliseconds at)
{
    QVariantMap row;
    row.insert(QStringLiteral("text"), std::move(text));

    row.insert(QStringLiteral("at"), asStamp(at));

    row.insert(QStringLiteral("covers"), covers);
    answers_.push_back(std::move(row));
    Q_EMIT answersChanged();
}

void PrompterViewModel::heardGap(std::chrono::milliseconds lost)
{
    pendingGap_ += lost;
}

void PrompterViewModel::heardSpent(std::int64_t recogniserMicros, std::int64_t modelMicros,
                                   bool estimated)
{
    if (recogniserMicros == recogniserMicros_ && modelMicros == modelMicros_ &&
        estimated == estimated_) {
        return;
    }
    recogniserMicros_ = recogniserMicros;
    modelMicros_ = modelMicros;
    estimated_ = estimated;
    Q_EMIT spentChanged();
}

void PrompterViewModel::heardTrouble(QString what, QString said)
{

    if (what == trouble_ && said == troubleSaid_) {
        return;
    }
    trouble_ = std::move(what);
    troubleSaid_ = std::move(said);
    Q_EMIT troubleChanged();
}

void PrompterViewModel::clear()
{
    said_.clear();
    answers_.clear();
    provisionalFar_.clear();
    provisionalNear_.clear();
    pendingGap_ = std::chrono::milliseconds::zero();
    recogniserMicros_ = 0;
    modelMicros_ = 0;
    trouble_.clear();
    troubleSaid_.clear();

    Q_EMIT saidChanged();
    Q_EMIT answersChanged();
    Q_EMIT spentChanged();
    Q_EMIT troubleChanged();
}

QString PrompterViewModel::sideWord(const QString &side) const
{
    const auto voice = side == QLatin1String{"near"} ? core::Voice::Near : core::Voice::Far;
    const auto said = core::speakerWord(voice, language_.toStdString());
    return QString::fromUtf8(said.data(), static_cast<qsizetype>(said.size()));
}

void PrompterViewModel::setContentLanguage(QString language)
{
    if (language == language_) {
        return;
    }
    language_ = std::move(language);

    Q_EMIT saidChanged();
}

void PrompterViewModel::freshConversation()
{
    if (running_) {
        return;
    }
    clear();
}

void PrompterViewModel::setActions(Start start, Plain stop, Plain askNow, Plain forget, Export save)
{
    start_ = std::move(start);
    stop_ = std::move(stop);
    askNow_ = std::move(askNow);
    forget_ = std::move(forget);
    save_ = std::move(save);
}

void PrompterViewModel::setDispatch(Dispatch dispatch)
{
    dispatch_ = std::move(dispatch);
}

void PrompterViewModel::choose(const QString &templateId)
{
    if (templateId == chosen_) {
        return;
    }
    chosen_ = templateId;
    Q_EMIT chosenChanged();
}

void PrompterViewModel::start()
{
    if (begun() || !available() || chosen_.isEmpty() || !dispatch_ || !start_) {
        return;
    }

    asked_ = true;
    Q_EMIT runningChanged();

    trouble_.clear();
    troubleSaid_.clear();
    Q_EMIT troubleChanged();

    Q_EMIT started();
    dispatch_([this, which = chosen_] { start_(which); });
}

void PrompterViewModel::stop()
{
    if (!dispatch_ || !stop_) {
        return;
    }

    asked_ = false;
    running_ = false;
    waiting_ = false;
    Q_EMIT runningChanged();
    dispatch_([this] { stop_(); });
}

void PrompterViewModel::clearEverything()
{
    clear();

    if (dispatch_ && forget_) {
        dispatch_([this] { forget_(); });
    }
}

void PrompterViewModel::askNow()
{
    if (!running_ || !dispatch_ || !askNow_) {
        return;
    }
    dispatch_([this] { askNow_(); });
}

QString PrompterViewModel::asText() const
{
    QString out;

    out += heading();
    out += QStringLiteral("\n\n");

    out += tr("Said");
    out += QStringLiteral("\n");
    for (const auto &row : said()) {
        const auto one = row.toMap();
        if (one.value(QStringLiteral("provisional")).toBool()) {

            continue;
        }
        if (const auto lost = one.value(QStringLiteral("gap")).toInt(); lost > 0) {
            out += tr("    [%1 s missing]").arg(QString::number(lost / 1000.0, 'f', 1));
            out += QStringLiteral("\n");
        }
        out += QStringLiteral("  %1  %2\n")
                   .arg(one.value(QStringLiteral("at")).toString(),
                        one.value(QStringLiteral("text")).toString());
    }

    out += QStringLiteral("\n");

    out += tr("Answered");
    out += QStringLiteral("\n");
    for (const auto &row : answers_) {
        const auto one = row.toMap();
        out += QStringLiteral("  %1  %2\n")
                   .arg(one.value(QStringLiteral("at")).toString(),
                        one.value(QStringLiteral("text")).toString());
    }
    return out;
}

QString PrompterViewModel::asCsv() const
{

    QString out = QStringLiteral("column,at,covers,text\n");

    const auto quoted = [](QString text) {
        text.replace(QLatin1Char{'"'}, QStringLiteral("\"\""));
        return QStringLiteral("\"%1\"").arg(text);
    };

    for (const auto &row : said()) {
        const auto one = row.toMap();
        if (one.value(QStringLiteral("provisional")).toBool()) {
            continue;
        }
        out += QStringLiteral("said,%1,,%2\n")
                   .arg(one.value(QStringLiteral("at")).toString(),
                        quoted(one.value(QStringLiteral("text")).toString()));
    }
    for (const auto &row : answers_) {
        const auto one = row.toMap();
        out += QStringLiteral("answered,%1,%2,%3\n")
                   .arg(one.value(QStringLiteral("at")).toString(),
                        QString::number(one.value(QStringLiteral("covers")).toInt()),
                        quoted(one.value(QStringLiteral("text")).toString()));
    }
    return out;
}

void PrompterViewModel::save(const QString &where)
{
    if (where.isEmpty() || !dispatch_ || !save_) {
        return;
    }

    const auto text =
        where.endsWith(QStringLiteral(".csv"), Qt::CaseInsensitive) ? asCsv() : asText();
    dispatch_([this, where, text] { save_(where, text); });
}

void PrompterViewModel::setRehearsalActions(Rehearse choose, Plain leave, Switch play, Scrub seek,
                                            Plain list)
{
    rehearseWith_ = std::move(choose);
    leaveRehearsal_ = std::move(leave);
    playRehearsal_ = std::move(play);
    seekRehearsal_ = std::move(seek);
    listRehearsals_ = std::move(list);
}

void PrompterViewModel::listRehearsals()
{
    if (listRehearsals_) {
        listRehearsals_();
    }
}

QVariantList PrompterViewModel::rehearsalWaveNow(const QString &side) const
{
    const auto &lane = side == QStringLiteral("far") ? farWave_ : wave_;
    QVariantList out;
    out.reserve(static_cast<qsizetype>(lane.size()));
    for (const auto one : lane) {
        out.push_back(one);
    }
    return out;
}

void PrompterViewModel::rehearse(const QString &recordingId)
{
    if (!rehearseWith_ || recordingId.isEmpty() || running_ || waiting_) {
        return;
    }
    rehearseWith_(recordingId, QString{});
}

void PrompterViewModel::rehearseFile(const QString &fileUrl)
{
    if (!rehearseWith_ || fileUrl.isEmpty() || running_ || waiting_) {
        return;
    }
    rehearseWith_(QString{}, fileUrl);
}

void PrompterViewModel::stopRehearsing()
{

    if (running_ || waiting_ || asked_) {
        stop();
    }

    rehearsing_ = false;
    rehearsalName_.clear();
    rehearsalNote_.clear();
    rehearsalPlaying_ = false;
    rehearsalAt_ = 0;
    rehearsalLength_ = 0;
    wave_.clear();
    farWave_.clear();
    rehearsalStereo_ = false;
    ++waveRevision_;
    Q_EMIT rehearsalChanged();
    Q_EMIT rehearsalAtChanged();

    Q_EMIT readyChanged();

    if (leaveRehearsal_) {
        leaveRehearsal_();
    }
}

void PrompterViewModel::playRehearsal(bool wanted)
{
    if (!rehearsing_ || !playRehearsal_) {
        return;
    }

    rehearsalPlaying_ = wanted;
    Q_EMIT rehearsalAtChanged();
    playRehearsal_(wanted);
}

void PrompterViewModel::seekRehearsal(qint64 positionMs)
{
    if (!rehearsing_ || !seekRehearsal_) {
        return;
    }
    rehearsalAt_ = std::clamp<qint64>(positionMs, 0, rehearsalLength_);
    Q_EMIT rehearsalAtChanged();
    seekRehearsal_(rehearsalAt_);
}

void PrompterViewModel::rehearsalChosen(QString name, QString note, qint64 lengthMs,
                                        std::vector<float> nearPicture,
                                        std::vector<float> farPicture, bool stereo)
{
    rehearsing_ = true;
    rehearsalName_ = std::move(name);
    rehearsalNote_ = std::move(note);
    rehearsalLength_ = lengthMs;
    rehearsalAt_ = 0;
    rehearsalPlaying_ = false;
    wave_ = std::move(nearPicture);
    farWave_ = std::move(farPicture);
    rehearsalStereo_ = stereo;
    ++waveRevision_;
    Q_EMIT rehearsalChanged();
    Q_EMIT rehearsalAtChanged();
    Q_EMIT readyChanged();
}

QString PrompterViewModel::rehearsalNote() const
{

    if (rehearsalNote_ == QStringLiteral("one-room")) {

        return tr("One room, so the prompter hears all of it as the other party.");
    }
    return {};
}

void PrompterViewModel::rehearsalAt(qint64 positionMs, bool playing)
{
    if (!rehearsing_) {
        return;
    }
    if (rehearsalAt_ == positionMs && rehearsalPlaying_ == playing) {
        return;
    }
    rehearsalAt_ = positionMs;
    rehearsalPlaying_ = playing;
    Q_EMIT rehearsalAtChanged();
}

void PrompterViewModel::rehearsalRefused(QString why)
{

    rehearsing_ = false;
    rehearsalName_.clear();
    rehearsalNote_.clear();
    wave_.clear();
    ++waveRevision_;
    Q_EMIT rehearsalChanged();
    Q_EMIT readyChanged();
    heardTrouble(std::move(why));
}

void PrompterViewModel::setRehearsalChoices(QVariantList rows)
{
    if (rehearsalChoices_ == rows) {
        return;
    }
    rehearsalChoices_ = std::move(rows);
    Q_EMIT rehearsalChoicesChanged();
}

}
