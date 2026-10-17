// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#pragma once

#include <softphone/core/contacts.hpp>
#include <softphone/core/recordings.hpp>

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace softphone::ui {

class RecordingsViewModel;

class RecordingRows : public QAbstractListModel {
    Q_OBJECT

public:
    explicit RecordingRows(RecordingsViewModel &owner);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void refilled();

    void arrivedAtTheTop(int howMany);

    void addedAtTheEnd(int howMany);

    void wentFromTheMiddle(int at, int howMany);

    void everyRowSaysSomethingNew();

private:
    RecordingsViewModel &owner_;

    int known_{0};
};

class RecordingsViewModel : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `recordings`.")

    Q_PROPERTY(QAbstractItemModel *rows READ rowsModel CONSTANT)

    Q_PROPERTY(int count READ count NOTIFY rowsChanged)

    Q_PROPERTY(bool available READ available NOTIFY availableChanged)

    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY filterChanged)

    Q_PROPERTY(Filter filter READ filter WRITE setFilter NOTIFY filterChanged)

    Q_PROPERTY(QString category READ category NOTIFY filterChanged)

    Q_PROPERTY(QString tag READ tag NOTIFY filterChanged)

    Q_PROPERTY(QString redFlag READ redFlag NOTIFY filterChanged)

    Q_PROPERTY(QStringList categories READ categories NOTIFY facetsChanged)
    Q_PROPERTY(QStringList tags READ tags NOTIFY facetsChanged)
    Q_PROPERTY(QStringList redFlags READ redFlags NOTIFY facetsChanged)

    Q_PROPERTY(bool more READ more NOTIFY rowsChanged)

    Q_PROPERTY(bool fetching READ fetching NOTIFY rowsChanged)

    Q_PROPERTY(Period period READ period NOTIFY filterChanged)

    Q_PROPERTY(qint64 fromMs READ fromMs NOTIFY filterChanged)
    Q_PROPERTY(qint64 toMs READ toMs NOTIFY filterChanged)

    Q_PROPERTY(bool filtered READ filtered NOTIFY filterChanged)

    Q_PROPERTY(QString callFilter READ callFilter NOTIFY filterChanged)

    Q_PROPERTY(int pageSize READ pageSize CONSTANT)
    Q_PROPERTY(bool pageFull READ pageFull NOTIFY rowsChanged)

    Q_PROPERTY(QString openId READ openId NOTIFY openChanged)

    Q_PROPERTY(QString openSeance READ openSeance NOTIFY openChanged)

    Q_PROPERTY(bool playerShown READ playerShown NOTIFY panelsChanged)
    Q_PROPERTY(bool readingShown READ readingShown NOTIFY panelsChanged)
    Q_PROPERTY(QString openLabel READ openLabel NOTIFY openChanged)
    Q_PROPERTY(QString openTitle READ openTitle NOTIFY openChanged)
    Q_PROPERTY(QString openDetail READ openDetail NOTIFY openChanged)

    Q_PROPERTY(qint64 openStartedAtMs READ openStartedAtMs NOTIFY openChanged)

    Q_PROPERTY(QString exportName READ exportName NOTIFY openChanged)

    Q_PROPERTY(qint64 openBytes READ openBytes NOTIFY openChanged)

    Q_PROPERTY(bool openHasAudio READ openHasAudio NOTIFY openChanged)

    Q_PROPERTY(bool openMixed READ openMixed NOTIFY openChanged)

    Q_PROPERTY(bool openUnreadable READ openUnreadable NOTIFY playbackChanged)

    Q_PROPERTY(bool openUnfinished READ openUnfinished NOTIFY openChanged)

    Q_PROPERTY(bool playing READ playing NOTIFY playbackChanged)

    Q_PROPERTY(bool starting READ starting NOTIFY playbackChanged)
    Q_PROPERTY(qint64 positionMs READ positionMs NOTIFY playbackChanged)
    Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY playbackChanged)

    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY playbackChanged)
    Q_PROPERTY(bool nearSide READ nearSide NOTIFY playbackChanged)
    Q_PROPERTY(bool farSide READ farSide NOTIFY playbackChanged)

public:

    enum class Filter { All, Incoming, Outgoing, Captured, Imported, Pinned, Conference };
    Q_ENUM(Filter)

    enum class Period { Any, Today, Yesterday, Week, Range };
    Q_ENUM(Period)

    using Reader = std::function<void()>;
    using Action = std::function<void(const QString &id)>;
    using Renamer = std::function<void(const QString &id, const QString &name)>;

    using Importer = std::function<void(const QList<QUrl> &files)>;

    using Opener = std::function<void(const QString &id, bool play)>;
    using Export = std::function<void(const QString &id, const QString &target)>;
    using Seek = std::function<void(qint64 positionMs)>;
    using Speed = std::function<void(double speed)>;
    using Channels = std::function<void(bool nearSide, bool farSide)>;
    using Plain = std::function<void()>;

    explicit RecordingsViewModel(QObject *parent = nullptr);

    [[nodiscard]] QAbstractItemModel *rowsModel();

    [[nodiscard]] QVariantMap rowFor(std::size_t at) const;

    [[nodiscard]] Q_INVOKABLE int indexOf(const QString &id) const;

    [[nodiscard]] int count() const noexcept
    {
        return static_cast<int>(shown_.size());
    }

    [[nodiscard]] bool available() const noexcept
    {
        return available_;
    }

    [[nodiscard]] QString search() const
    {
        return search_;
    }

    void setSearch(const QString &text);

    [[nodiscard]] Filter filter() const noexcept
    {
        return filter_;
    }

    void setFilter(Filter filter);

    [[nodiscard]] bool filtered() const noexcept
    {
        return !search_.isEmpty() || filter_ != Filter::All || !callFilter_.isEmpty() ||
               period_ != Period::Any || !category_.isEmpty() || !tag_.isEmpty() ||
               !redFlag_.isEmpty();
    }

    [[nodiscard]] Period period() const noexcept
    {
        return period_;
    }

    [[nodiscard]] qint64 fromMs() const noexcept
    {
        return fromMs_;
    }

    [[nodiscard]] qint64 toMs() const noexcept
    {
        return toMs_;
    }

    [[nodiscard]] core::RecordingQuery query() const;

    [[nodiscard]] QString callFilter() const
    {
        return callFilter_;
    }

    [[nodiscard]] QString category() const
    {
        return category_;
    }

    [[nodiscard]] QString tag() const
    {
        return tag_;
    }

    [[nodiscard]] QString redFlag() const
    {
        return redFlag_;
    }

    [[nodiscard]] QStringList categories() const
    {
        return categories_;
    }

    [[nodiscard]] QStringList tags() const
    {
        return tags_;
    }

    [[nodiscard]] QStringList redFlags() const
    {
        return redFlags_;
    }

    [[nodiscard]] bool more() const noexcept
    {
        return more_;
    }

    [[nodiscard]] bool fetching() const noexcept
    {
        return fetching_;
    }

    Q_INVOKABLE void loadMore();

public Q_SLOTS:

    void showCategory(const QString &code);
    void showTag(const QString &code);
    void showRedFlag(const QString &code);

public:

    [[nodiscard]] static constexpr int pageSize() noexcept
    {
        return 50;
    }

    [[nodiscard]] bool pageFull() const noexcept
    {
        return all_.size() >= static_cast<std::size_t>(pageSize());
    }

    [[nodiscard]] QString openId() const
    {
        return openId_;
    }

    [[nodiscard]] QString openSeance() const
    {
        const auto *recording = find(openId_);
        return recording != nullptr ? QString::fromStdString(recording->seanceId) : QString{};
    }

    [[nodiscard]] bool playerShown() const noexcept
    {
        return playerShown_;
    }

    [[nodiscard]] bool readingShown() const noexcept
    {
        return readingShown_;
    }

    [[nodiscard]] QString openLabel() const;

    [[nodiscard]] QString openTitle() const;
    [[nodiscard]] QString openDetail() const;
    [[nodiscard]] qint64 openStartedAtMs() const;
    [[nodiscard]] QString exportName() const;

    [[nodiscard]] Q_INVOKABLE QString listName() const;

    [[nodiscard]] Q_INVOKABLE bool saveListTo(const QUrl &where);

    Q_INVOKABLE void importFiles(const QList<QUrl> &files);

    void setImporter(Importer bring);

    void sayWhatWasImported(int brought, int refused);
    [[nodiscard]] qint64 openBytes() const;

    [[nodiscard]] bool openUnreadable() const noexcept
    {
        return unreadable_;
    }

    [[nodiscard]] bool openUnfinished() const
    {
        const auto *row = find(openId_);
        return row != nullptr && row->unfinished;
    }

    void reportUnreadable(const QString &id);

    [[nodiscard]] bool openHasAudio() const
    {
        const auto *recording = find(openId_);
        return recording != nullptr && recording->hasAudio();
    }

    [[nodiscard]] bool openMixed() const
    {
        const auto *recording = find(openId_);
        return recording != nullptr &&
               (recording->channelLayout != "near-far" || recording->channels < 2);
    }

    [[nodiscard]] bool playing() const noexcept
    {
        return playing_;
    }

    [[nodiscard]] bool starting() const noexcept
    {
        return starting_;
    }

    [[nodiscard]] qint64 positionMs() const noexcept
    {
        return positionMs_;
    }

    [[nodiscard]] qint64 durationMs() const noexcept
    {
        return durationMs_;
    }

    [[nodiscard]] double speed() const noexcept
    {
        return speed_;
    }

    void setSpeed(double speed);

    [[nodiscard]] bool nearSide() const noexcept
    {
        return nearSide_;
    }

    [[nodiscard]] bool farSide() const noexcept
    {
        return farSide_;
    }

    [[nodiscard]] Q_INVOKABLE QVariantList nearWaveNow() const;
    [[nodiscard]] Q_INVOKABLE QVariantList farWaveNow() const;

    [[nodiscard]] Q_INVOKABLE QVariantList bothWavesNow() const;

    void setRecordings(std::vector<core::Recording> found, bool available);

    void setContacts(std::vector<core::Contact> contacts);

    void appendRecordings(std::vector<core::Recording> found, bool available);

    void updateRecording(core::Recording row);

    void setFacets(QStringList categories, QStringList tags, QStringList redFlags);
    void setWaveform(const QString &id, std::vector<double> nearSide, std::vector<double> farSide);

    void setPlayback(bool playing, qint64 positionMs, qint64 durationMs, bool ended, bool ours);

    void setActions(Reader refresh, Opener open, Action stop, Plain pause, Plain resume, Seek seek,
                    Speed speed, Channels channels, Action remove, Action pin, Export save,
                    Renamer rename);

public Q_SLOTS:

    void refresh();

    void showCallsOf(const QString &callId, bool playFirst = false);

    void showPeriod(Period period);

    void showDates(qint64 fromMs, qint64 toMs);

    void showEverything();

    void open(const QString &id);

    void play(const QString &id);

    void togglePlay();

    void startPlaying();

    void stop();

    void closePlayer();
    void closeReading();

    void showReading();
    void seek(qint64 positionMs);

    void listenTo(bool near, bool far);

    void toggleNear();
    void toggleFar();

    void remove(const QString &id);

    void togglePin(const QString &id);

    Q_INVOKABLE void rename(const QString &id, const QString &name);

    void saveAs(const QUrl &target);

Q_SIGNALS:

    void moreWanted(int offset);
    void facetsChanged();

    void rowsChanged();
    void availableChanged();
    void filterChanged();
    void openChanged();
    void playbackChanged();

    void sought();

    void panelsChanged();
    void waveChanged();

    void imported(int brought, int refused);

private:
    void reshow();

    void openRow(const QString &id, bool play);

    [[nodiscard]] const core::Recording *find(const QString &id) const;

    void openAgainIfItHasFinished();

    [[nodiscard]] std::string spokenName(const std::string &stored,
                                         const std::string &number) const;

    [[nodiscard]] QString whoWasIn(const core::Recording &recording) const;

    std::vector<core::Recording> all_;

    core::ContactBook contacts_;

    QString category_;
    QString tag_;
    QString redFlag_;
    QStringList categories_;
    QStringList tags_;
    QStringList redFlags_;
    bool more_{false};
    bool fetching_{false};
    std::vector<const core::Recording *> shown_;

    std::vector<std::string> wereShown_;

    RecordingRows rows_;
    bool available_{true};

    QString search_;
    Filter filter_{Filter::All};
    QString callFilter_;

    Period period_{Period::Any};

    qint64 fromMs_{0};
    qint64 toMs_{0};

    bool playWhenReady_{false};

    QString openId_;
    bool playerShown_{false};
    bool readingShown_{false};
    bool playing_{false};

    bool starting_{false};

    bool ours_{false};

    qint64 positionMs_{0};
    qint64 durationMs_{0};
    double speed_{1.0};
    bool nearSide_{true};
    bool farSide_{true};

    QString waveId_;

    bool unreadable_{false};

    bool openedWhileBeingWritten_{false};

    std::vector<double> nearWave_;
    std::vector<double> farWave_;

    std::vector<double> bothWaves_;

    Reader refresh_;
    Opener open_;
    Action stop_;
    Plain pause_;
    Plain resume_;
    Seek seek_;
    Speed speed__;
    Channels channels_;
    Action remove_;
    Action pin_;

    Renamer rename_;
    Importer bring_;
    Export save_;
};

}
