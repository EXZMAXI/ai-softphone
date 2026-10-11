// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "update_view_model.hpp"

#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>

#include <QDateTime>
#include <QDesktopServices>
#include <QLocale>
#include <QMetaObject>
#include <QNetworkInformation>
#include <QUrl>

#include <chrono>

namespace softphone::ui {
namespace {

using namespace std::chrono_literals;

constexpr auto kBetweenChecks = 24h;

constexpr auto kFirstCheck = 5s;

constexpr auto kAfterACall = 5s;

constexpr auto kBetweenAnnouncements = 24h;

[[nodiscard]] UpdateViewModel::State translate(core::UpdateState state) noexcept
{
    switch (state) {
    case core::UpdateState::Unconfigured:
        return UpdateViewModel::State::Unconfigured;
    case core::UpdateState::Idle:
        return UpdateViewModel::State::Idle;
    case core::UpdateState::Checking:
        return UpdateViewModel::State::Checking;
    case core::UpdateState::UpToDate:
        return UpdateViewModel::State::UpToDate;
    case core::UpdateState::Available:
        return UpdateViewModel::State::Available;
    case core::UpdateState::Downloading:
        return UpdateViewModel::State::Downloading;
    case core::UpdateState::Ready:
        return UpdateViewModel::State::Ready;
    case core::UpdateState::Failed:
        return UpdateViewModel::State::Failed;
    }
    return UpdateViewModel::State::Idle;
}

void openInBrowser(const QString &address)
{
    const QUrl url{address};
    if (url.scheme() != QStringLiteral("https")) {
        return;
    }
    QDesktopServices::openUrl(url);
}

}

UpdateViewModel::UpdateViewModel(core::UpdateService *service, QObject *parent)
    : QObject{parent}, service_{service},
      afterACall_{std::chrono::duration_cast<std::chrono::milliseconds>(kAfterACall)}
{
    daily_.setInterval(std::chrono::duration_cast<std::chrono::milliseconds>(kBetweenChecks));
    daily_.setSingleShot(false);
    connect(&daily_, &QTimer::timeout, this, [this] { check(); });

    first_.setInterval(std::chrono::duration_cast<std::chrono::milliseconds>(kFirstCheck));
    first_.setSingleShot(true);
    connect(&first_, &QTimer::timeout, this, [this] { check(); });

    settle_.setSingleShot(true);
    connect(&settle_, &QTimer::timeout, this, [this] {
        announcing_ = true;
        toldAt_ = QDateTime::currentDateTime();
        Q_EMIT changed();
    });

    if (QNetworkInformation::loadDefaultBackend()) {
        if (auto *watching = QNetworkInformation::instance(); watching != nullptr) {
            connect(watching, &QNetworkInformation::reachabilityChanged, this,
                    [this]() { Q_EMIT networkReachableChanged(); });
        }
    }

    if (service_ == nullptr) {
        return;
    }

    adopt(service_->status());

    service_->observe([this](const core::UpdateStatus &status) {
        QMetaObject::invokeMethod(this, [this, status] { adopt(status); }, Qt::QueuedConnection);
    });
}

UpdateViewModel::~UpdateViewModel()
{

    if (service_ != nullptr) {
        service_->observe(nullptr);
    }
}

double UpdateViewModel::progress() const noexcept
{
    if (state_ != State::Downloading) {
        return 0.0;
    }
    if (total_ == 0) {
        return -1.0;
    }
    return static_cast<double>(received_) / static_cast<double>(total_);
}

void UpdateViewModel::adopt(const core::UpdateStatus &status)
{
    state_ = translate(status.state);
    offered_ = QString::fromStdString(status.offered);
    notes_ = QString::fromStdString(status.notes);
    downloadUrl_ = QString::fromStdString(status.downloadUrl);
    complaint_ = QString::fromStdString(status.complaint);
    selfReplacing_ = status.selfReplacing;
    received_ = status.received;
    total_ = status.total;

    lastChecked_ =
        status.checkedAt.time_since_epoch().count() == 0
            ? QString{}
            : QLocale{}.toString(QDateTime::fromMSecsSinceEpoch(
                                     std::chrono::duration_cast<std::chrono::milliseconds>(
                                         status.checkedAt.time_since_epoch())
                                         .count()),
                                 QLocale::ShortFormat);

    if (installWhenReady_ && state_ == State::Ready) {
        installWhenReady_ = false;
        QMetaObject::invokeMethod(this, &UpdateViewModel::installUnlessBusy, Qt::QueuedConnection);
    }

    considerAnnouncing(std::chrono::milliseconds{0});

    Q_EMIT changed();
}

void UpdateViewModel::considerAnnouncing(std::chrono::milliseconds after)
{
    if (announcing_ || settle_.isActive()) {
        return;
    }

    const bool worthSaying = (state_ == State::Available || state_ == State::Ready) &&
                             !offered_.isEmpty() && (selfReplacing_ || !downloadUrl_.isEmpty());
    if (!worthSaying) {
        return;
    }

    if (inACall_ || capturing_) {
        return;
    }

    if (toldAt_.isValid() &&
        toldAt_.msecsTo(QDateTime::currentDateTime()) <
            std::chrono::duration_cast<std::chrono::milliseconds>(kBetweenAnnouncements).count()) {
        return;
    }

    settle_.setInterval(after);
    settle_.start();
}

bool UpdateViewModel::networkReachable() const
{
    const auto *asked = QNetworkInformation::instance();
    if (asked == nullptr) {
        return true;
    }

    return asked->reachability() != QNetworkInformation::Reachability::Disconnected;
}

void UpdateViewModel::inACall(bool busy)
{
    if (busy == inACall_) {
        return;
    }
    inACall_ = busy;
    busyChanged(busy);
}

void UpdateViewModel::capturing(bool busy)
{
    if (busy == capturing_) {
        return;
    }
    capturing_ = busy;
    busyChanged(busy);
}

void UpdateViewModel::busyChanged(bool nowBusy)
{
    if (nowBusy) {

        settle_.stop();
        return;
    }

    considerAnnouncing(afterACall_);
}

void UpdateViewModel::downloadAndInstall()
{
    notNow();

    if (state_ == State::Ready) {
        installUnlessBusy();
        return;
    }

    installWhenReady_ = true;
    fetch();
}

void UpdateViewModel::installUnlessBusy()
{

    if (inACall_ || capturing_) {
        heldBack_ = true;
        core::Logger::instance().log(
            core::LogLevel::Info, "updates",
            "the new version is downloaded and the phone is busy; not restarting");
        Q_EMIT changed();
        return;
    }
    install();
}

void UpdateViewModel::notNow()
{
    settle_.stop();
    installWhenReady_ = false;
    if (!announcing_) {

        toldAt_ = QDateTime::currentDateTime();
        return;
    }
    announcing_ = false;
    toldAt_ = QDateTime::currentDateTime();
    Q_EMIT changed();
}

void UpdateViewModel::automatic(bool wanted)
{
    if (service_ == nullptr) {
        return;
    }

    if (!wanted) {
        daily_.stop();
        first_.stop();
        return;
    }

    if (!daily_.isActive()) {
        daily_.start();
    }
    if (!first_.isActive() && state_ == State::Idle) {
        first_.start();
    }
}

void UpdateViewModel::check()
{
    if (service_ != nullptr) {
        service_->check();
    }
}

void UpdateViewModel::fetch()
{
    if (service_ != nullptr) {
        service_->fetch();
    }
}

void UpdateViewModel::install()
{
    if (service_ == nullptr) {
        return;
    }

    heldBack_ = false;

    const auto image = QString::fromStdString(softphone::core::asUtf8(service_->channel().running));

    std::string complaint;
    if (!service_->apply(complaint)) {
        complaint_ = QString::fromStdString(complaint);
        state_ = State::Failed;
        core::Logger::instance().log(core::LogLevel::Error, "updates",
                                     "the update was not installed: " + complaint);
        Q_EMIT changed();
        return;
    }

    core::Logger::instance().log(core::LogLevel::Info, "updates",
                                 image.isEmpty()
                                     ? "the installer has been started; closing so it can work"
                                     : "installed; restarting into the new version");

    Q_EMIT restartWanted(image);
}

void UpdateViewModel::openDownload()
{
    openInBrowser(downloadUrl_);
}

void UpdateViewModel::openNotes()
{
    openInBrowser(notes_);
}

}
