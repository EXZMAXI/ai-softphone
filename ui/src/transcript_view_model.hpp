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

#include <softphone/core/transcript_export.hpp>
#include <softphone/core/transcripts.hpp>

#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace softphone::ui {

class TranscriptViewModel : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `transcript`.")

    Q_PROPERTY(QVariantList lines READ lines NOTIFY linesChanged)

    Q_PROPERTY(int count READ count NOTIFY linesChanged)

    Q_PROPERTY(bool available READ available NOTIFY linesChanged)

    Q_PROPERTY(QVariantList choices READ choices NOTIFY choicesChanged)

    Q_PROPERTY(QString openId READ openId NOTIFY choicesChanged)
    Q_PROPERTY(bool primary READ primary NOTIFY choicesChanged)

    Q_PROPERTY(QString provenance READ provenance NOTIFY choicesChanged)

    Q_PROPERTY(QString language READ language NOTIFY choicesChanged)

    Q_PROPERTY(QString failure READ failure NOTIFY choicesChanged)

    Q_PROPERTY(bool working READ working NOTIFY workingChanged)

    Q_PROPERTY(int currentLine READ currentLine NOTIFY playheadChanged)

    Q_PROPERTY(int currentWord READ currentWord NOTIFY playheadChanged)

    Q_PROPERTY(bool karaoke READ karaoke NOTIFY linesChanged)

    Q_PROPERTY(bool done READ done NOTIFY choicesChanged)

    Q_PROPERTY(QString notice READ notice NOTIFY noticeChanged)

    Q_PROPERTY(bool rawKept READ rawKept NOTIFY choicesChanged)

public:

    enum class Refusal {
        None,

        NoProvider,

        NoAudio,

        Failed,
    };
    Q_ENUM(Refusal)

    using Plain = std::function<void()>;
    using Choose = std::function<void(const QString &transcriptId)>;

    using Ask = std::function<void(const QString &providerId)>;

    using CopyRaw = std::function<void(const QString &rawPath, const QString &target)>;

    using Save = std::function<void(const QString &target, const QString &text)>;

    explicit TranscriptViewModel(QObject *parent = nullptr);

    [[nodiscard]] QVariantList lines() const;

    [[nodiscard]] Q_INVOKABLE QVariantList linesNow() const;

    [[nodiscard]] int count() const noexcept
    {
        return static_cast<int>(shown_.segments.size());
    }

    [[nodiscard]] bool available() const noexcept
    {
        return !shown_.id.isEmpty();
    }

    [[nodiscard]] QVariantList choices() const;

    [[nodiscard]] QString openId() const
    {
        return shown_.id;
    }

    [[nodiscard]] bool primary() const noexcept
    {
        return shown_.primary;
    }

    [[nodiscard]] QString provenance() const;

    [[nodiscard]] QString language() const
    {
        return shown_.language;
    }

    [[nodiscard]] QString failure() const
    {
        return shown_.error;
    }

    [[nodiscard]] bool working() const noexcept
    {
        return working_;
    }

    [[nodiscard]] int currentLine() const noexcept
    {
        return currentLine_;
    }

    [[nodiscard]] int currentWord() const noexcept
    {
        return currentWord_;
    }

    [[nodiscard]] bool karaoke() const noexcept
    {
        return karaoke_;
    }

    [[nodiscard]] bool done() const noexcept
    {
        return !shown_.id.isEmpty() && shown_.status == core::TranscriptStatus::Done;
    }

    [[nodiscard]] QString notice() const
    {
        return notice_;
    }

    [[nodiscard]] bool rawKept() const noexcept
    {
        return !shown_.rawPath.isEmpty();
    }

    void refuse(Refusal why);

    [[nodiscard]] Q_INVOKABLE qint64 startOf(int line) const;

    Q_INVOKABLE void choose(const QString &transcriptId);

    Q_INVOKABLE void makePrimary();

    Q_INVOKABLE void transcribe();

    Q_INVOKABLE void transcribeWith(const QString &providerId);

    Q_INVOKABLE void forget();

    Q_INVOKABLE QString exportName(int format) const;

    [[nodiscard]] Q_INVOKABLE QString openAsText() const;

    Q_INVOKABLE void exportTo(const QUrl &target, int format);

    void setTranscripts(std::vector<core::Transcript> all, const QString &show = {});

    void clear();

    void setProviderNames(std::vector<std::pair<QString, QString>> names);

    void setSubject(const QString &exportStem, const QString &party,
                    std::vector<std::pair<QString, QString>> stemNames);

    Q_INVOKABLE void setPosition(qint64 positionMs);

    void setWorking(bool working);

    void setActions(Ask transcribe, Choose choose, Plain makePrimary, Plain forget, Save save,
                    CopyRaw copyRaw);

    static constexpr int kRawFormat = 3;

Q_SIGNALS:
    void linesChanged();
    void choicesChanged();
    void playheadChanged();
    void workingChanged();
    void noticeChanged();

private:
    struct Line {
        int ordinal{0};
        qint64 startMs{0};
        qint64 endMs{0};
        QString voice;
        QString who;
        QString speaker;
        QString text;
        std::vector<std::pair<qint64, qint64>> wordTimes;
        QStringList words;

        QList<int> wordAt;
        QList<int> wordTo;
    };

    struct Shown {
        QString id;
        bool primary{false};
        QString provider;
        QString model;
        QString language;
        QString error;
        QString rawPath;
        core::TranscriptStatus status{core::TranscriptStatus::Done};
        std::vector<Line> segments;
    };

    struct Choice {
        QString id;
        QString label;
        bool primary{false};
        bool failed{false};
    };

    [[nodiscard]] QString labelFor(const QString &voice) const;
    [[nodiscard]] core::TranscriptNames names() const;
    void rebuild(const QString &show);
    void locate();

    std::vector<core::Transcript> all_;
    std::vector<Choice> choices_;
    Shown shown_;

    QString notice_;
    std::vector<std::pair<QString, QString>> providerNames_;
    QString exportStem_;
    QString party_;
    std::vector<std::pair<QString, QString>> stemNames_;

    qint64 positionMs_{0};
    int currentLine_{-1};
    int currentWord_{-1};
    bool karaoke_{false};
    bool working_{false};

    Ask transcribe_;
    Choose choose_;
    Plain makePrimary_;
    Plain forget_;
    Save save_;
    CopyRaw copyRaw_;
};

}
