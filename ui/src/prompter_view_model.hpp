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

#include <softphone/core/streaming_asr.hpp>
#include <softphone/core/templates.hpp>

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <functional>
#include <vector>

namespace softphone::ui {

class PrompterViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool available READ available NOTIFY readyChanged)

    Q_PROPERTY(bool configured READ configured NOTIFY readyChanged)

    Q_PROPERTY(QString unavailable READ unavailable NOTIFY readyChanged)

    Q_PROPERTY(bool offered READ offered NOTIFY readyChanged)

    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

    Q_PROPERTY(bool waiting READ waiting NOTIFY runningChanged)

    Q_PROPERTY(bool begun READ begun NOTIFY runningChanged)

    Q_PROPERTY(QVariantList templates READ templates NOTIFY templatesChanged)

    Q_PROPERTY(QString chosen READ chosen WRITE choose NOTIFY chosenChanged)

    Q_PROPERTY(QString heading READ heading NOTIFY runningChanged)

    Q_PROPERTY(QString waitingLine READ waitingLine NOTIFY runningChanged)
    Q_PROPERTY(QString costLine READ costLine NOTIFY spentChanged)

    Q_PROPERTY(QString trouble READ trouble NOTIFY troubleChanged)

    Q_PROPERTY(QString troubleSaid READ troubleSaid NOTIFY troubleChanged)

    Q_PROPERTY(QVariantList said READ said NOTIFY saidChanged)

    Q_PROPERTY(QVariantList answers READ answers NOTIFY answersChanged)

    Q_PROPERTY(bool anything READ anything NOTIFY saidChanged)

    Q_PROPERTY(bool asksByHand READ asksByHand NOTIFY chosenChanged)

    Q_PROPERTY(bool suggesting READ suggesting NOTIFY chosenChanged)

    Q_PROPERTY(bool rehearsing READ rehearsing NOTIFY rehearsalChanged)

    Q_PROPERTY(QString rehearsalName READ rehearsalName NOTIFY rehearsalChanged)

    Q_PROPERTY(QString rehearsalNote READ rehearsalNote NOTIFY rehearsalChanged)

    Q_PROPERTY(bool rehearsalPlaying READ rehearsalPlaying NOTIFY rehearsalAtChanged)
    Q_PROPERTY(qint64 rehearsalAtMs READ rehearsalAtMs NOTIFY rehearsalAtChanged)
    Q_PROPERTY(qint64 rehearsalLengthMs READ rehearsalLengthMs NOTIFY rehearsalChanged)

    Q_PROPERTY(int rehearsalWaveRevision READ rehearsalWaveRevision NOTIFY rehearsalChanged)

    Q_PROPERTY(bool rehearsalStereo READ rehearsalStereo NOTIFY rehearsalChanged)

    Q_PROPERTY(QVariantList rehearsalChoices READ rehearsalChoices NOTIFY rehearsalChoicesChanged)

public:
    explicit PrompterViewModel(QObject *parent = nullptr);

    [[nodiscard]] bool available() const noexcept;
    [[nodiscard]] bool configured() const noexcept;
    [[nodiscard]] QString unavailable() const;

    [[nodiscard]] bool offered() const noexcept
    {
        return offered_;
    }

    [[nodiscard]] bool running() const noexcept
    {
        return running_;
    }

    [[nodiscard]] bool waiting() const noexcept
    {
        return waiting_;
    }

    [[nodiscard]] bool begun() const noexcept
    {
        return running_ || waiting_ || asked_;
    }

    [[nodiscard]] QVariantList templates() const;
    [[nodiscard]] bool asksByHand() const;
    [[nodiscard]] bool suggesting() const;

    [[nodiscard]] bool rehearsing() const noexcept
    {
        return rehearsing_;
    }

    [[nodiscard]] QString rehearsalName() const
    {
        return rehearsalName_;
    }

    [[nodiscard]] QString rehearsalNote() const;

    [[nodiscard]] bool rehearsalPlaying() const noexcept
    {
        return rehearsalPlaying_;
    }

    [[nodiscard]] qint64 rehearsalAtMs() const noexcept
    {
        return rehearsalAt_;
    }

    [[nodiscard]] qint64 rehearsalLengthMs() const noexcept
    {
        return rehearsalLength_;
    }

    [[nodiscard]] int rehearsalWaveRevision() const noexcept
    {
        return waveRevision_;
    }

    [[nodiscard]] bool rehearsalStereo() const noexcept
    {
        return rehearsalStereo_;
    }

    [[nodiscard]] QVariantList rehearsalChoices() const
    {
        return rehearsalChoices_;
    }

    [[nodiscard]] Q_INVOKABLE QVariantList rehearsalWaveNow(const QString &side) const;

    void rehearsalChosen(QString name, QString note, qint64 lengthMs,
                         std::vector<float> nearPicture, std::vector<float> farPicture,
                         bool stereo);
    void rehearsalAt(qint64 positionMs, bool playing);
    void rehearsalRefused(QString why);
    void setRehearsalChoices(QVariantList rows);

    [[nodiscard]] QString chosen() const noexcept
    {
        return chosen_;
    }

    [[nodiscard]] QString heading() const;
    [[nodiscard]] QString waitingLine() const;
    [[nodiscard]] QString costLine() const;
    [[nodiscard]] QString trouble() const;

    [[nodiscard]] QString troubleSaid() const
    {
        return troubleSaid_;
    }

    [[nodiscard]] QVariantList said() const;
    [[nodiscard]] QVariantList answers() const;

    [[nodiscard]] bool anything() const noexcept
    {
        return !said_.isEmpty() || !answers_.isEmpty();
    }

    void setTemplates(std::vector<core::PromptTemplate> rows);

    void setReady(bool offered, bool conversation, bool recogniser);

    void setContentLanguage(QString language);

    void setRunning(bool running);

    void setWaiting(bool waiting);
    void setNames(QString templateName, QString recogniserName, QString modelName);

    void setSubject(QString who);

    [[nodiscard]] Q_INVOKABLE QString exportName(bool asCsv = false) const;

    [[nodiscard]] Q_INVOKABLE QString saidAsText() const;
    [[nodiscard]] Q_INVOKABLE QString answersAsText() const;

    void heardPartial(bool near, QString text);
    void heardTurn(bool near, QString text, std::chrono::milliseconds at);
    void heardAnswer(QString text, int covers, std::chrono::milliseconds at);
    void heardGap(std::chrono::milliseconds lost);
    void heardSpent(std::int64_t recogniserMicros, std::int64_t modelMicros, bool estimated);
    void heardTrouble(QString what, QString said = {});

    void clear();

    void freshConversation();

    using Plain = std::function<void()>;
    using Start = std::function<void(const QString &templateId)>;

    using Export = std::function<void(const QString &where, const QString &text)>;

    void setActions(Start start, Plain stop, Plain askNow, Plain forget, Export save);

    using Dispatch = std::function<void(std::function<void()>)>;
    void setDispatch(Dispatch dispatch);

    using Rehearse = std::function<void(const QString &recordingId, const QString &fileUrl)>;
    using Scrub = std::function<void(qint64 positionMs)>;
    using Switch = std::function<void(bool wanted)>;

    void setRehearsalActions(Rehearse choose, Plain leave, Switch play, Scrub seek, Plain list);

public Q_SLOTS:

    void choose(const QString &templateId);

    [[nodiscard]] QString sideWord(const QString &side) const;
    void start();
    void stop();
    void askNow();

    void clearEverything();

    void save(const QString &where);

    void rehearse(const QString &recordingId);
    void rehearseFile(const QString &fileUrl);

    void stopRehearsing();
    void playRehearsal(bool wanted);
    void seekRehearsal(qint64 positionMs);

    void listRehearsals();

Q_SIGNALS:
    void readyChanged();
    void runningChanged();
    void templatesChanged();
    void chosenChanged();
    void saidChanged();
    void answersChanged();
    void spentChanged();
    void troubleChanged();
    void rehearsalChanged();
    void rehearsalAtChanged();
    void rehearsalChoicesChanged();

    void started();

private:

    [[nodiscard]] QString asText() const;
    [[nodiscard]] QString asCsv() const;

    std::vector<core::PromptTemplate> templates_;
    QString chosen_;

    bool offered_{false};
    bool conversation_{false};
    bool recogniser_{false};
    bool running_{false};
    bool waiting_{false};
    QString subject_;

    bool asked_{false};

    QString language_;

    bool rehearsing_{false};
    QString rehearsalName_;
    QString rehearsalNote_;
    bool rehearsalPlaying_{false};
    qint64 rehearsalAt_{0};
    qint64 rehearsalLength_{0};
    std::vector<float> wave_;
    std::vector<float> farWave_;
    bool rehearsalStereo_{false};
    int waveRevision_{0};
    QVariantList rehearsalChoices_;

    Rehearse rehearseWith_;
    Plain leaveRehearsal_;
    Switch playRehearsal_;
    Scrub seekRehearsal_;
    Plain listRehearsals_;

    QString templateName_;
    QString recogniserName_;
    QString modelName_;

    QVariantList said_;
    QVariantMap provisionalFar_;
    QVariantMap provisionalNear_;

    QVariantList answers_;

    std::chrono::milliseconds pendingGap_{0};

    std::int64_t recogniserMicros_{0};
    std::int64_t modelMicros_{0};
    bool estimated_{true};
    QString trouble_;
    QString troubleSaid_;

    Start start_;
    Plain stop_;
    Plain askNow_;
    Plain forget_;
    Export save_;
    Dispatch dispatch_;
};

}
