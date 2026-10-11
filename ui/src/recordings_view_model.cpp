// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "recordings_view_model.hpp"

#include "export_names.hpp"

#include "list_edit.hpp"

#include <softphone/core/library_export.hpp>

#include <QDateTime>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QVariantMap>

#include <algorithm>
#include <chrono>
#include <span>
#include <utility>

namespace softphone::ui {
namespace {

[[nodiscard]] QString text(const std::string &value)
{
    return QString::fromStdString(value);
}

[[nodiscard]] qint64 millisecondsOf(std::chrono::system_clock::time_point when)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(when.time_since_epoch()).count();
}

[[nodiscard]] qint64 startOfDay(qint64 ms)
{
    auto when = QDateTime::fromMSecsSinceEpoch(ms);
    when.setTime(QTime{0, 0});
    return when.toMSecsSinceEpoch();
}

[[nodiscard]] qint64 startOfToday()
{
    return startOfDay(QDateTime::currentMSecsSinceEpoch());
}

[[nodiscard]] qint64 daysOn(qint64 ms, int days)
{
    return QDateTime::fromMSecsSinceEpoch(ms).addDays(days).toMSecsSinceEpoch();
}

[[nodiscard]] std::chrono::system_clock::time_point instantOf(qint64 ms)
{
    return std::chrono::system_clock::time_point{std::chrono::milliseconds{ms}};
}

}

RecordingRows::RecordingRows(RecordingsViewModel &owner) : owner_{owner} {}

int RecordingRows::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : known_;
}

QVariant RecordingRows::data(const QModelIndex &index, int role) const
{
    if (role != Qt::UserRole || index.row() < 0 || index.row() >= owner_.count()) {
        return {};
    }
    return owner_.rowFor(static_cast<std::size_t>(index.row()));
}

QHash<int, QByteArray> RecordingRows::roleNames() const
{
    return {{Qt::UserRole, QByteArrayLiteral("recording")}};
}

void RecordingRows::refilled()
{
    beginResetModel();
    known_ = owner_.count();
    endResetModel();
}

void RecordingRows::arrivedAtTheTop(int howMany)
{
    if (howMany <= 0) {
        return;
    }
    beginInsertRows({}, 0, howMany - 1);
    known_ += howMany;
    endInsertRows();
}

void RecordingRows::addedAtTheEnd(int howMany)
{
    if (howMany <= 0) {
        return;
    }
    beginInsertRows({}, known_, known_ + howMany - 1);
    known_ += howMany;
    endInsertRows();
}

void RecordingRows::wentFromTheMiddle(int at, int howMany)
{
    if (howMany <= 0 || at < 0 || at + howMany > known_) {
        return;
    }
    beginRemoveRows({}, at, at + howMany - 1);
    known_ -= howMany;
    endRemoveRows();
}

void RecordingRows::everyRowSaysSomethingNew()
{
    if (known_ == 0) {
        return;
    }

    Q_EMIT dataChanged(index(0, 0), index(known_ - 1, 0), {Qt::UserRole});
}

RecordingsViewModel::RecordingsViewModel(QObject *parent) : QObject{parent}, rows_{*this} {}

void RecordingsViewModel::setActions(Reader refresh, Opener open, Plain stop, Plain pause,
                                     Plain resume, Seek seek, Speed speed, Channels channels,
                                     Action remove, Action pin, Export save, Renamer rename)
{
    save_ = std::move(save);
    refresh_ = std::move(refresh);
    open_ = std::move(open);
    stop_ = std::move(stop);
    pause_ = std::move(pause);
    resume_ = std::move(resume);
    seek_ = std::move(seek);
    speed__ = std::move(speed);
    channels_ = std::move(channels);
    remove_ = std::move(remove);
    pin_ = std::move(pin);
    rename_ = std::move(rename);
}

void RecordingsViewModel::setRecordings(std::vector<core::Recording> found, bool available)
{
    more_ = found.size() >= static_cast<std::size_t>(pageSize());
    fetching_ = false;
    all_ = std::move(found);

    if (available != available_) {
        available_ = available;
        Q_EMIT availableChanged();
    }

    reshow();
    openAgainIfItHasFinished();

    if (playWhenReady_ && !shown_.empty()) {
        playWhenReady_ = false;
        play(text(shown_.front()->id));
    }
}

void RecordingsViewModel::appendRecordings(std::vector<core::Recording> found, bool available)
{
    more_ = found.size() >= static_cast<std::size_t>(pageSize());
    fetching_ = false;

    all_.insert(all_.end(), std::make_move_iterator(found.begin()),
                std::make_move_iterator(found.end()));

    if (available != available_) {
        available_ = available;
        Q_EMIT availableChanged();
    }

    reshow();
    openAgainIfItHasFinished();
}

void RecordingsViewModel::updateRecording(core::Recording row)
{
    const auto found = std::find_if(
        all_.begin(), all_.end(), [&row](const core::Recording &one) { return one.id == row.id; });
    if (found == all_.end()) {

        return;
    }

    *found = std::move(row);

    reshow();
}

void RecordingsViewModel::loadMore()
{
    if (!more_ || fetching_) {
        return;
    }
    fetching_ = true;
    Q_EMIT rowsChanged();

    Q_EMIT moreWanted(static_cast<int>(all_.size()));
}

void RecordingsViewModel::setFacets(QStringList categories, QStringList tags, QStringList redFlags)
{
    if (categories == categories_ && tags == tags_ && redFlags == redFlags_) {
        return;
    }
    categories_ = std::move(categories);
    tags_ = std::move(tags);
    redFlags_ = std::move(redFlags);

    const auto forget = [this](QString &chosen, const QStringList &offered) {
        if (chosen.isEmpty() || offered.contains(chosen)) {
            return;
        }
        chosen.clear();
        Q_EMIT filterChanged();
        refresh();
    };
    forget(category_, categories_);
    forget(tag_, tags_);
    forget(redFlag_, redFlags_);

    Q_EMIT facetsChanged();
}

void RecordingsViewModel::showCategory(const QString &code)
{
    if (code == category_) {
        return;
    }
    category_ = code;
    Q_EMIT filterChanged();
    refresh();
}

void RecordingsViewModel::showTag(const QString &code)
{
    if (code == tag_) {
        return;
    }
    tag_ = code;
    Q_EMIT filterChanged();
    refresh();
}

void RecordingsViewModel::showRedFlag(const QString &code)
{
    if (code == redFlag_) {
        return;
    }
    redFlag_ = code;
    Q_EMIT filterChanged();
    refresh();
}

void RecordingsViewModel::reshow()
{

    shown_.clear();
    shown_.reserve(all_.size());

    for (const core::Recording &recording : all_) {
        if (!recording.isDeleted()) {
            shown_.push_back(&recording);
        }
    }

    std::vector<std::string> nowShown;
    nowShown.reserve(shown_.size());
    for (const core::Recording *recording : shown_) {
        nowShown.push_back(recording->id);
    }

    const auto sameFrom = [](const std::vector<std::string> &longer,
                             const std::vector<std::string> &shorter, std::size_t offset) {
        for (std::size_t at = 0; at < shorter.size(); ++at) {
            if (longer[at + offset] != shorter[at]) {
                return false;
            }
        }
        return true;
    };

    const auto goneOrReset = [this, &nowShown]() {
        if (const auto gone = removalBetween(wereShown_, nowShown); gone.has_value()) {
            rows_.wentFromTheMiddle(gone->at, gone->howMany);
            rows_.addedAtTheEnd(gone->appended);
            return;
        }
        rows_.refilled();
    };

    if (wereShown_.empty()) {
        rows_.refilled();
    } else if (nowShown.size() < wereShown_.size()) {
        goneOrReset();
    } else if (nowShown.size() == wereShown_.size() && nowShown == wereShown_) {
        rows_.everyRowSaysSomethingNew();
    } else if (const auto extra = nowShown.size() - wereShown_.size();
               sameFrom(nowShown, wereShown_, extra)) {
        rows_.arrivedAtTheTop(static_cast<int>(extra));
    } else if (sameFrom(nowShown, wereShown_, 0)) {
        rows_.addedAtTheEnd(static_cast<int>(nowShown.size() - wereShown_.size()));
    } else {
        goneOrReset();
    }

    wereShown_ = std::move(nowShown);

    Q_EMIT rowsChanged();
}

QString RecordingsViewModel::whoWasIn(const core::Recording &recording) const
{
    if (recording.legs.size() < 2) {
        return {};
    }

    QStringList everybody;
    for (const core::RecordingLeg &leg : recording.legs) {
        const auto spoken = spokenName(leg.remoteName, leg.remoteNumber);
        const std::string &said = spoken.empty() ? leg.remoteNumber : spoken;
        if (!said.empty()) {
            everybody.append(QString::fromStdString(said));
        }
    }

    return everybody.join(QStringLiteral(", "));
}

void RecordingsViewModel::setContacts(std::vector<core::Contact> contacts)
{
    contacts_.reset(std::move(contacts));

    rows_.everyRowSaysSomethingNew();
    Q_EMIT rowsChanged();
    Q_EMIT openChanged();
}

std::string RecordingsViewModel::spokenName(const std::string &stored,
                                            const std::string &number) const
{
    if (!number.empty()) {
        if (auto known = contacts_.nameFor(number); !known.empty()) {
            return known;
        }
    }
    return stored;
}

QAbstractItemModel *RecordingsViewModel::rowsModel()
{
    return &rows_;
}

int RecordingsViewModel::indexOf(const QString &id) const
{
    const auto wanted = id.toStdString();
    for (std::size_t at = 0; at < shown_.size(); ++at) {
        if (shown_[at]->id == wanted) {
            return static_cast<int>(at);
        }
    }
    return -1;
}

QVariantMap RecordingsViewModel::rowFor(std::size_t at) const
{
    const core::Recording *recording = shown_.at(at);
    QVariantMap row;
    {
        row.insert(QStringLiteral("id"), text(recording->id));

        row.insert(QStringLiteral("seanceId"), text(recording->seanceId));
        row.insert(QStringLiteral("startedAtMs"), millisecondsOf(recording->startedAt));
        row.insert(QStringLiteral("incoming"),
                   recording->direction == core::CallDirection::Incoming);
        row.insert(QStringLiteral("durationMs"), static_cast<qint64>(recording->duration.count()));
        const QString everybody = whoWasIn(*recording);
        row.insert(QStringLiteral("number"),
                   everybody.isEmpty() ? text(recording->remoteNumber) : QString{});
        row.insert(QStringLiteral("name"),
                   everybody.isEmpty()
                       ? text(spokenName(recording->remoteName, recording->remoteNumber))
                       : everybody);
        row.insert(QStringLiteral("bytes"), static_cast<qint64>(recording->bytes));
        row.insert(QStringLiteral("pinned"), recording->pinned);

        row.insert(QStringLiteral("external"),
                   recording->source == core::RecordingSource::External);

        row.insert(QStringLiteral("imported"),
                   recording->source == core::RecordingSource::Imported);

        row.insert(QStringLiteral("title"), text(recording->title));
        row.insert(QStringLiteral("remoteName"), text(recording->remoteName));
        row.insert(QStringLiteral("hasGaps"), recording->hasGaps);

        row.insert(QStringLiteral("hasAudio"), recording->hasAudio());

        row.insert(QStringLiteral("unfinished"), recording->unfinished);
        row.insert(QStringLiteral("parties"), static_cast<int>(recording->partyCount));
        row.insert(QStringLiteral("consent"), text(std::string{toString(recording->consent)}));

        int stems = 0;
        for (const core::RecordingStem &stem : recording->stems) {
            stems += stem.droppedAt.time_since_epoch().count() == 0 ? 1 : 0;
        }
        row.insert(QStringLiteral("stems"), stems);

        row.insert(QStringLiteral("category"), text(recording->categoryCode));
        row.insert(QStringLiteral("summary"), text(recording->superSummary));
        QStringList flags;
        flags.reserve(static_cast<qsizetype>(recording->redFlags.size()));
        for (const auto &code : recording->redFlags) {
            flags.push_back(text(code));
        }
        row.insert(QStringLiteral("redFlags"), flags);
        row.insert(QStringLiteral("redFlagCount"), recording->redFlagCount);
        row.insert(QStringLiteral("score"), recording->score);

        QStringList tags;
        tags.reserve(static_cast<qsizetype>(recording->tags.size()));
        for (const auto &code : recording->tags) {
            tags.push_back(QString::fromStdString(code));
        }
        row.insert(QStringLiteral("tags"), tags);
    }

    return row;
}

const core::Recording *RecordingsViewModel::find(const QString &id) const
{
    const auto wanted = id.toStdString();
    for (const core::Recording &recording : all_) {
        if (recording.id == wanted) {
            return &recording;
        }
    }
    return nullptr;
}

void RecordingsViewModel::setSearch(const QString &value)
{

    if (value == search_) {
        return;
    }
    search_ = value;
    Q_EMIT filterChanged();

    refresh();
}

core::RecordingQuery RecordingsViewModel::query() const
{
    core::RecordingQuery wanted;
    wanted.callId = callFilter_.toStdString();
    wanted.text = search_.trimmed().toStdString();

    switch (filter_) {
    case Filter::All:
        break;
    case Filter::Incoming:
        wanted.direction = core::CallDirection::Incoming;
        break;
    case Filter::Outgoing:
        wanted.direction = core::CallDirection::Outgoing;
        break;
    case Filter::Captured:
        wanted.source = core::RecordingSource::External;
        break;
    case Filter::Imported:
        wanted.source = core::RecordingSource::Imported;
        break;
    case Filter::Pinned:
        wanted.pinned = true;
        break;
    case Filter::Conference:
        wanted.conference = true;
        break;
    }

    wanted.category = category_.toStdString();
    wanted.tag = tag_.toStdString();
    wanted.redFlag = redFlag_.toStdString();

    switch (period_) {
    case Period::Any:
        break;
    case Period::Today: {
        const auto today = startOfToday();
        wanted.startedFrom = instantOf(today);
        wanted.startedBefore = instantOf(daysOn(today, 1));
        break;
    }
    case Period::Yesterday: {
        const auto today = startOfToday();
        wanted.startedFrom = instantOf(daysOn(today, -1));
        wanted.startedBefore = instantOf(today);
        break;
    }
    case Period::Week: {

        const auto today = startOfToday();
        wanted.startedFrom = instantOf(daysOn(today, -6));
        wanted.startedBefore = instantOf(daysOn(today, 1));
        break;
    }
    case Period::Range:
        if (fromMs_ != 0) {
            wanted.startedFrom = instantOf(fromMs_);
        }
        if (toMs_ != 0) {

            wanted.startedBefore = instantOf(daysOn(toMs_, 1));
        }
        break;
    }

    return wanted;
}

void RecordingsViewModel::showPeriod(Period period)
{

    if (period == Period::Range || period == period_) {
        return;
    }

    period_ = period;
    fromMs_ = 0;
    toMs_ = 0;

    Q_EMIT filterChanged();

    refresh();
}

void RecordingsViewModel::showDates(qint64 fromMs, qint64 toMs)
{
    if (fromMs <= 0 || toMs <= 0) {
        return;
    }

    const auto first = startOfDay(std::min(fromMs, toMs));
    const auto last = startOfDay(std::max(fromMs, toMs));

    if (period_ == Period::Range && first == fromMs_ && last == toMs_) {
        return;
    }

    period_ = Period::Range;
    fromMs_ = first;
    toMs_ = last;

    Q_EMIT filterChanged();
    refresh();
}

void RecordingsViewModel::showEverything()
{
    if (callFilter_.isEmpty()) {
        return;
    }

    playWhenReady_ = false;
    callFilter_.clear();
    Q_EMIT filterChanged();
    refresh();
}

void RecordingsViewModel::setFilter(Filter filter)
{
    if (filter == filter_) {
        return;
    }
    filter_ = filter;
    Q_EMIT filterChanged();

    refresh();
}

QString RecordingsViewModel::openLabel() const
{
    const auto *recording = find(openId_);
    if (recording == nullptr) {
        return {};
    }
    if (const QString everybody = whoWasIn(*recording); !everybody.isEmpty()) {
        return everybody;
    }
    const auto said = spokenName(recording->remoteName, recording->remoteNumber);
    return said.empty() ? text(recording->remoteNumber) : text(said);
}

QString RecordingsViewModel::openTitle() const
{
    const auto *recording = find(openId_);
    return recording == nullptr ? QString{} : text(recording->title);
}

QString RecordingsViewModel::openDetail() const
{
    const auto *recording = find(openId_);
    if (recording == nullptr) {
        return {};
    }

    const auto said = spokenName(recording->remoteName, recording->remoteNumber);
    if (said.empty()) {
        return {};
    }

    if (!whoWasIn(*recording).isEmpty()) {
        return {};
    }

    if (said == recording->remoteNumber) {
        return {};
    }
    return text(recording->remoteNumber);
}

qint64 RecordingsViewModel::openBytes() const
{
    const auto *recording = find(openId_);
    return recording == nullptr ? 0 : static_cast<qint64>(recording->bytes);
}

qint64 RecordingsViewModel::openStartedAtMs() const
{
    const auto *recording = find(openId_);
    return recording == nullptr ? 0 : millisecondsOf(recording->startedAt);
}

QString RecordingsViewModel::listName() const
{

    const auto when = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH-mm"));

    return tr("Conversations %1.csv").arg(when);
}

bool RecordingsViewModel::saveListTo(const QUrl &where)
{

    const auto path = where.isLocalFile() ? where.toLocalFile() : where.toString();
    if (path.isEmpty()) {
        return false;
    }

    core::LibraryColumns columns;

    columns.when = tr("When").toStdString();
    columns.direction = tr("Direction").toStdString();
    columns.party = tr("Party").toStdString();
    columns.number = tr("Number").toStdString();
    columns.account = tr("Account").toStdString();
    columns.seconds = tr("Seconds").toStdString();
    columns.category = tr("Category").toStdString();
    columns.tags = tr("Tags").toStdString();
    columns.redFlags = tr("Flags").toStdString();
    columns.language = tr("Language").toStdString();
    columns.summary = tr("Summary").toStdString();

    columns.incoming = tr("in").toStdString();
    columns.outgoing = tr("out").toStdString();

    const auto csv = core::renderLibraryCsv(std::span{std::as_const(all_)}, columns);

    QFile file{path};
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    const auto bytes = QByteArray::fromStdString(csv);
    return file.write(bytes) == bytes.size();
}

QString RecordingsViewModel::exportName() const
{
    const auto *recording = find(openId_);
    if (recording == nullptr) {
        return {};
    }

    const QString everybody = whoWasIn(*recording);
    const auto who = recording->remoteNumber.empty()
                         ? spokenName(recording->remoteName, recording->remoteNumber)
                         : recording->remoteNumber;
    const QString said = everybody.isEmpty() ? text(who) : everybody;

    const auto when = QDateTime::fromMSecsSinceEpoch(millisecondsOf(recording->startedAt));
    return conversationStem(when, said) + QStringLiteral(".mp3");
}

void RecordingsViewModel::saveAs(const QUrl &target)
{
    if (openId_.isEmpty() || !save_ || !target.isValid()) {
        return;
    }
    save_(openId_, target.toLocalFile());
}

QVariantList RecordingsViewModel::nearWaveNow() const
{
    QVariantList out;
    out.reserve(static_cast<qsizetype>(nearWave_.size()));
    for (const double value : nearWave_) {
        out.append(value);
    }
    return out;
}

QVariantList RecordingsViewModel::farWaveNow() const
{
    QVariantList out;
    out.reserve(static_cast<qsizetype>(farWave_.size()));
    for (const double value : farWave_) {
        out.append(value);
    }
    return out;
}

QVariantList RecordingsViewModel::bothWavesNow() const
{
    QVariantList out;
    out.reserve(static_cast<qsizetype>(bothWaves_.size()));
    for (const double value : bothWaves_) {
        out.append(value);
    }
    return out;
}

void RecordingsViewModel::setWaveform(const QString &id, std::vector<double> nearSide,
                                      std::vector<double> farSide)
{

    if (id != openId_) {
        return;
    }

    waveId_ = id;
    nearWave_ = std::move(nearSide);
    farWave_ = std::move(farSide);

    bothWaves_.assign(std::max(nearWave_.size(), farWave_.size()), 0.0);
    for (std::size_t at = 0; at < bothWaves_.size(); ++at) {
        const auto near = at < nearWave_.size() ? nearWave_[at] : 0.0;
        const auto far = at < farWave_.size() ? farWave_[at] : 0.0;
        bothWaves_[at] = std::max(near, far);
    }
    Q_EMIT waveChanged();
}

void RecordingsViewModel::setPlayback(bool playing, qint64 positionMs, qint64 durationMs,
                                      bool ended)
{
    playing_ = playing;
    positionMs_ = ended ? 0 : positionMs;
    durationMs_ = durationMs;
    Q_EMIT playbackChanged();
}

void RecordingsViewModel::refresh()
{
    if (refresh_) {
        refresh_();
    }
}

void RecordingsViewModel::showCallsOf(const QString &callId, bool playFirst)
{
    playWhenReady_ = playFirst;

    if (callId == callFilter_) {

        if (playWhenReady_ && !shown_.empty()) {
            playWhenReady_ = false;
            play(text(shown_.front()->id));
        }
        return;
    }

    callFilter_ = callId;

    search_.clear();
    filter_ = Filter::All;
    period_ = Period::Any;
    fromMs_ = 0;
    toMs_ = 0;

    Q_EMIT filterChanged();
    refresh();
}

void RecordingsViewModel::open(const QString &id)
{

    if (!playerShown_ && !readingShown_) {
        playerShown_ = true;
        readingShown_ = true;
        Q_EMIT panelsChanged();
    }

    if (id != openId_) {
        openRow(id, false);
    }
}

void RecordingsViewModel::play(const QString &id)
{

    const auto wasShown = playerShown_;
    if (!playerShown_) {
        playerShown_ = true;
        Q_EMIT panelsChanged();
    }

    if (id == openId_ && wasShown) {

        togglePlay();
        return;
    }

    openRow(id, true);
}

void RecordingsViewModel::startPlaying()
{

    if (playing_ || openId_.isEmpty()) {
        return;
    }
    play(openId_);
}

void RecordingsViewModel::reportUnreadable(const QString &id)
{

    if (id != openId_ || unreadable_) {
        return;
    }
    unreadable_ = true;
    Q_EMIT playbackChanged();
}

void RecordingsViewModel::closePlayer()
{
    if (!playerShown_) {
        return;
    }
    playerShown_ = false;

    if (!readingShown_) {
        stop();
        return;
    }

    playing_ = false;
    unreadable_ = false;
    positionMs_ = 0;
    durationMs_ = 0;
    nearWave_.clear();
    farWave_.clear();
    bothWaves_.clear();
    waveId_.clear();

    Q_EMIT panelsChanged();
    Q_EMIT waveChanged();
    Q_EMIT playbackChanged();

    if (stop_) {
        stop_();
    }
}

void RecordingsViewModel::closeReading()
{
    if (!readingShown_) {
        return;
    }
    readingShown_ = false;

    if (!playerShown_) {
        stop();
        return;
    }
    Q_EMIT panelsChanged();
}

void RecordingsViewModel::showReading()
{

    if (readingShown_ || openId_.isEmpty()) {
        return;
    }
    readingShown_ = true;
    Q_EMIT panelsChanged();
}

void RecordingsViewModel::openAgainIfItHasFinished()
{
    if (!openedWhileBeingWritten_ || openId_.isEmpty()) {
        return;
    }

    const auto *row = find(openId_);
    if (row == nullptr || row->unfinished) {
        return;
    }

    openRow(openId_, false);
}

void RecordingsViewModel::openRow(const QString &id, bool play)
{
    openId_ = id;
    unreadable_ = false;
    positionMs_ = 0;
    durationMs_ = 0;
    nearWave_.clear();
    farWave_.clear();
    bothWaves_.clear();
    waveId_.clear();

    Q_EMIT openChanged();
    Q_EMIT waveChanged();
    Q_EMIT playbackChanged();

    const auto *row = find(id);
    openedWhileBeingWritten_ = row != nullptr && row->unfinished;
    if (row != nullptr && (!row->hasAudio() || row->unfinished)) {
        return;
    }

    if (open_ && playerShown_) {
        open_(id, play);
    }
}

void RecordingsViewModel::togglePlay()
{
    if (openId_.isEmpty()) {
        return;
    }

    if (playing_) {
        if (pause_) {
            pause_();
        }
        return;
    }
    if (resume_) {
        resume_();
    }
}

void RecordingsViewModel::stop()
{
    const auto wasShown = playerShown_ || readingShown_;
    playerShown_ = false;
    readingShown_ = false;

    openId_.clear();
    unreadable_ = false;
    playing_ = false;
    positionMs_ = 0;
    durationMs_ = 0;
    nearWave_.clear();
    farWave_.clear();
    bothWaves_.clear();

    Q_EMIT openChanged();
    Q_EMIT waveChanged();
    Q_EMIT playbackChanged();
    if (wasShown) {
        Q_EMIT panelsChanged();
    }

    if (stop_) {
        stop_();
    }
}

void RecordingsViewModel::seek(qint64 positionMs)
{
    if (openId_.isEmpty()) {
        return;
    }

    positionMs_ = std::clamp<qint64>(positionMs, 0, durationMs_);
    Q_EMIT playbackChanged();
    Q_EMIT sought();

    if (seek_) {
        seek_(positionMs_);
    }
}

void RecordingsViewModel::setSpeed(double speed)
{
    const auto wanted = std::clamp(speed, 0.25, 4.0);
    if (std::abs(wanted - speed_) < 0.001) {
        return;
    }
    speed_ = wanted;
    Q_EMIT playbackChanged();

    if (speed__) {
        speed__(wanted);
    }
}

void RecordingsViewModel::listenTo(bool near, bool far)
{

    if (!near && !far) {
        near = true;
        far = true;
    }
    if (nearSide_ == near && farSide_ == far) {
        return;
    }

    nearSide_ = near;
    farSide_ = far;
    Q_EMIT playbackChanged();

    if (channels_) {
        channels_(nearSide_, farSide_);
    }
}

void RecordingsViewModel::toggleNear()
{

    const bool near = !nearSide_;
    listenTo(near, (!near && !farSide_) ? true : farSide_);
}

void RecordingsViewModel::toggleFar()
{
    const bool far = !farSide_;
    listenTo((!far && !nearSide_) ? true : nearSide_, far);
}

void RecordingsViewModel::remove(const QString &id)
{
    if (id == openId_) {
        stop();
    }
    if (remove_) {
        remove_(id);
    }
}

void RecordingsViewModel::togglePin(const QString &id)
{
    if (pin_) {
        pin_(id);
    }
}

void RecordingsViewModel::setImporter(Importer bring)
{
    bring_ = std::move(bring);
}

void RecordingsViewModel::importFiles(const QList<QUrl> &files)
{
    if (!bring_ || files.isEmpty()) {
        return;
    }
    bring_(files);
}

void RecordingsViewModel::sayWhatWasImported(int brought, int refused)
{
    Q_EMIT imported(brought, refused);
}

void RecordingsViewModel::rename(const QString &id, const QString &name)
{

    if (rename_) {
        rename_(id, name.trimmed());
    }
}

}
