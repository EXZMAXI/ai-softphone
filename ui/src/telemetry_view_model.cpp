// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "telemetry_view_model.hpp"

#include <softphone/core/logging.hpp>
#include <softphone/core/telemetry_service.hpp>
#include <softphone/core/version.hpp>

#include <QCoreApplication>

#include <algorithm>
#include <array>
#include <string>
#include <system_error>
#include <utility>

namespace softphone::ui {

TelemetryViewModel::TelemetryViewModel(QObject *parent) : QObject{parent} {}

void TelemetryViewModel::bind(core::TelemetryStore *store, ModeChanged told, Persist persist)
{
    store_ = store;
    told_ = std::move(told);
    persist_ = std::move(persist);
    refresh();
}

void TelemetryViewModel::refresh()
{
    const bool was = noticeWanted_;
    noticeWanted_ = store_ != nullptr && store_->state().noticeSeen != kNoticeRevision;
    if (was != noticeWanted_) {
        Q_EMIT changed();
    }
}

void TelemetryViewModel::retranslate()
{
    Q_EMIT languageChanged();
}

QString TelemetryViewModel::mode() const
{
    const auto text = core::toString(mode_);
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

void TelemetryViewModel::applyMode(core::TelemetryMode mode)
{
    if (mode_ == mode) {
        return;
    }
    mode_ = mode;
    if (told_) {

        told_(mode_);
    }
    Q_EMIT changed();
}

void TelemetryViewModel::chooseMode(const QString &wanted)
{
    const auto asked = core::parseTelemetryMode(wanted.toStdString());

    if (asked == core::TelemetryMode::Off && !offAvailable()) {
        return;
    }
    if (asked == mode_) {
        return;
    }
    mode_ = asked;
    if (told_) {
        told_(mode_);
    }
    if (persist_) {
        persist_(mode_);
    }
    Q_EMIT changed();
}

void TelemetryViewModel::acceptNotice()
{
    if (store_ != nullptr) {
        store_->noteNoticeSeen(kNoticeRevision);
    }
    noticeWanted_ = false;
    Q_EMIT changed();
    Q_EMIT noticeAccepted();
}

bool TelemetryViewModel::fromSource() noexcept
{

    return core::packageFormat() == "source";
}

void TelemetryViewModel::setReportSender(SendNow send)
{
    report_ = std::move(send);
}

void TelemetryViewModel::setOverviewListener(OverviewDone listener)
{
    overviewDone_ = std::move(listener);
}

void TelemetryViewModel::setOverviewDone(int done)
{

    if (done != overviewSaid_) {
        overviewSaid_ = done;
        const auto green = std::to_string(done);
        const std::array attributes{core::LogAttribute{"green", green}};
        core::Logger::instance().log(core::LogLevel::Info, "ui", "the summary counted its lamps",
                                     attributes);
    }

    if (overviewDone_) {
        overviewDone_(done);
    }
}

void TelemetryViewModel::sendNow()
{
    if (reporting_ || !report_ || !fromSource()) {
        return;
    }
    if (noticeWanted_) {

        reportOutcome_ = tr("Not yet — the notice has not been answered.");
        Q_EMIT reportChanged();
        return;
    }

    reporting_ = true;
    reportOutcome_.clear();
    Q_EMIT reportChanged();
    report_();
}

void TelemetryViewModel::reportFinished(bool accepted, const QString &host)
{
    reporting_ = false;
    reportOutcome_ = accepted

                         ? tr("Accepted by %1").arg(host)

                         : tr("No host took it.");
    Q_EMIT reportChanged();
}

void TelemetryViewModel::setFeedbackSender(SendFeedback send)
{
    send_ = std::move(send);
}

void TelemetryViewModel::setPaths(std::filesystem::path logFile,
                                  std::filesystem::path stateDirectory)
{
    logFile_ = std::move(logFile);
    stateDirectory_ = std::move(stateDirectory);
    Q_EMIT feedbackChanged();
}

QString TelemetryViewModel::attachments() const
{

    std::error_code failure;
    const auto log =
        std::filesystem::exists(logFile_, failure)
            ? std::min<std::uintmax_t>(std::filesystem::file_size(logFile_, failure), 512 * 1024)
            : 0;

    std::uintmax_t dump = 0;
    if (!stateDirectory_.empty()) {
        for (const auto &entry : std::filesystem::directory_iterator{stateDirectory_, failure}) {
            if (entry.is_regular_file(failure) && entry.path().extension() == ".dmp" &&
                entry.path().filename().string().starts_with("crash-")) {
                dump = std::max(dump, std::filesystem::file_size(entry.path(), failure));
            }
        }
    }

    if (log == 0 && dump == 0) {
        return {};
    }

    const auto kb = [](std::uintmax_t bytes) {
        return QString::number(static_cast<double>(bytes) / 1024.0, 'f', 0);
    };
    if (dump == 0) {

        return tr("The end of the log, about %1 kB").arg(kb(log));
    }

    return tr("The end of the log, about %1 kB, and the last crash report, %2 kB")
        .arg(kb(log))
        .arg(kb(dump));
}

void TelemetryViewModel::sendFeedback(const QString &subject, const QString &message,
                                      const QString &name, const QString &email, bool attach)
{
    if (sending_ || !send_) {
        return;
    }

    core::FeedbackReport report;

    report.subject = subject.trimmed().toStdString();
    report.message = message.trimmed().toStdString();
    report.name = name.trimmed().toStdString();
    report.email = email.trimmed().toStdString();

    if (report.message.empty()) {
        return;
    }

    sending_ = true;
    delivered_ = false;
    outcome_.clear();
    Q_EMIT feedbackChanged();

    send_(std::move(report), attach);
}

void TelemetryViewModel::feedbackFinished(bool arrived)
{
    sending_ = false;
    delivered_ = arrived;
    outcome_ = arrived

                   ? tr("Thank you — your message has been sent.")

                   : tr("The message could not be sent just now. Nothing was lost — try again in "
                        "a little while.");
    Q_EMIT feedbackChanged();
}

QStringList TelemetryViewModel::always() const
{
    return {

        tr("Application launch fact, version and interface language"),

        tr("Operating system version, locale, country and timezone"),
    };
}

QStringList TelemetryViewModel::extended() const
{
    return {

        tr("Calls and captured conversations counters"),

        tr("Connected softswitch vendor and version, never address"),

        tr("How many steps of the overview are done, and the chosen layout"),
    };
}

QStringList TelemetryViewModel::never() const
{
    return {

        tr("Numbers you dialled or were called from"),

        tr("Accounts, passwords, or anything from the keychain"),

        tr("Contacts, conversations, transcripts or recordings"),

        tr("Anything you typed, and any private data on this computer"),
    };
}

QString TelemetryViewModel::identity() const
{

    return tr("Each installation makes one random identifier for itself, so that reports from "
              "the same copy of the program can be recognised as one. It is not derived from "
              "anything about you or your computer, and it names nobody — but because it lasts, "
              "the reports it carries can be linked to each other. That makes them pseudonymous "
              "rather than anonymous, and we would rather say so than call them anonymous and "
              "be wrong.");
}

QString TelemetryViewModel::lawful() const
{

    return tr("The basic report has a legitimate interest as its basis: knowing which versions "
              "are in use is what lets a fix reach the people who need it. Everything the "
              "extended report adds is there because you chose it, and you can change that here "
              "at any time. Turning reporting off altogether is an Enterprise feature.");
}

}
