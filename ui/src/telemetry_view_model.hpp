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

#include <softphone/core/telemetry.hpp>
#include <softphone/core/telemetry_service.hpp>

#include <QObject>
#include <QString>
#include <QStringList>

#include <filesystem>
#include <functional>

namespace softphone::ui {

class TelemetryViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool noticeWanted READ noticeWanted NOTIFY changed)

    Q_PROPERTY(QString mode READ mode NOTIFY changed)

    Q_PROPERTY(bool offAvailable READ offAvailable CONSTANT)

    Q_PROPERTY(QStringList always READ always NOTIFY languageChanged)

    Q_PROPERTY(QStringList extended READ extended NOTIFY languageChanged)

    Q_PROPERTY(QStringList never READ never NOTIFY languageChanged)

    Q_PROPERTY(QString identity READ identity NOTIFY languageChanged)

    Q_PROPERTY(QString lawful READ lawful NOTIFY languageChanged)

    Q_PROPERTY(bool sending READ sending NOTIFY feedbackChanged)

    Q_PROPERTY(QString outcome READ outcome NOTIFY feedbackChanged)

    Q_PROPERTY(bool delivered READ delivered NOTIFY feedbackChanged)

    Q_PROPERTY(bool fromSource READ fromSource CONSTANT)

    Q_PROPERTY(bool reportsOnItsOwn READ reportsOnItsOwn CONSTANT)

    Q_PROPERTY(bool reporting READ reporting NOTIFY reportChanged)

    Q_PROPERTY(QString reportOutcome READ reportOutcome NOTIFY reportChanged)

    Q_PROPERTY(QString attachments READ attachments NOTIFY feedbackChanged)

public:

    static constexpr auto kNoticeRevision = "0.6.0";

    using ModeChanged = std::function<void(core::TelemetryMode)>;

    using Persist = std::function<void(core::TelemetryMode)>;

    explicit TelemetryViewModel(QObject *parent = nullptr);

    [[nodiscard]] bool noticeWanted() const noexcept
    {
        return noticeWanted_;
    }

    [[nodiscard]] QString mode() const;

    [[nodiscard]] static bool offAvailable() noexcept
    {
        return false;
    }

    [[nodiscard]] QStringList always() const;
    [[nodiscard]] QStringList extended() const;
    [[nodiscard]] QStringList never() const;
    [[nodiscard]] QString identity() const;
    [[nodiscard]] QString lawful() const;

    Q_INVOKABLE void chooseMode(const QString &wanted);

    Q_INVOKABLE void acceptNotice();

    [[nodiscard]] bool reportsOnItsOwn() const noexcept
    {
        return core::thisBuildReportsOnItsOwn();
    }

    [[nodiscard]] bool sending() const noexcept
    {
        return sending_;
    }

    [[nodiscard]] QString outcome() const
    {
        return outcome_;
    }

    [[nodiscard]] bool delivered() const noexcept
    {
        return delivered_;
    }

    [[nodiscard]] QString attachments() const;

    [[nodiscard]] static bool fromSource() noexcept;

    [[nodiscard]] bool reporting() const noexcept
    {
        return reporting_;
    }

    [[nodiscard]] QString reportOutcome() const
    {
        return reportOutcome_;
    }

    Q_INVOKABLE void sendNow();

    using SendNow = std::function<void()>;
    void setReportSender(SendNow send);

    using OverviewDone = std::function<void(int)>;
    void setOverviewListener(OverviewDone listener);
    Q_INVOKABLE void setOverviewDone(int done);

    void reportFinished(bool accepted, const QString &host);

    Q_INVOKABLE void sendFeedback(const QString &subject, const QString &message,
                                  const QString &name, const QString &email, bool attach);

    using SendFeedback = std::function<void(core::FeedbackReport, bool)>;

    void feedbackFinished(bool arrived);

    void bind(core::TelemetryStore *store, ModeChanged told, Persist persist);

    void setFeedbackSender(SendFeedback send);

    void setPaths(std::filesystem::path logFile, std::filesystem::path stateDirectory);

    void applyMode(core::TelemetryMode mode);

    void refresh();

    void retranslate();

Q_SIGNALS:
    void changed();
    void feedbackChanged();
    void reportChanged();
    void languageChanged();
    void noticeAccepted();

private:
    core::TelemetryStore *store_{nullptr};
    ModeChanged told_;
    Persist persist_;
    core::TelemetryMode mode_{core::TelemetryMode::Extended};
    bool noticeWanted_{false};

    SendFeedback send_;
    SendNow report_;
    OverviewDone overviewDone_;

    int overviewSaid_{-1};
    bool reporting_{false};
    QString reportOutcome_;
    std::filesystem::path logFile_;
    std::filesystem::path stateDirectory_;
    bool sending_{false};
    bool delivered_{false};
    QString outcome_;
};

}
