// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "accounts_view_model.hpp"

#include "chip_wrap.hpp"

#include <softphone/core/logging.hpp>

#include <QMetaObject>
#include <QVariantMap>

#include <algorithm>
#include <chrono>
#include <utility>

namespace softphone::ui {
namespace {

[[nodiscard]] core::RegistrationState fromMirror(AccountsViewModel::State state) noexcept
{
    switch (state) {
    case AccountsViewModel::State::Offline:
        return core::RegistrationState::Offline;
    case AccountsViewModel::State::Registering:
        return core::RegistrationState::Registering;
    case AccountsViewModel::State::Registered:
        return core::RegistrationState::Registered;
    case AccountsViewModel::State::Failed:
        return core::RegistrationState::Failed;
    case AccountsViewModel::State::Unregistering:
        return core::RegistrationState::Unregistering;
    }
    return core::RegistrationState::Offline;
}

[[nodiscard]] AccountsViewModel::State toViewState(core::RegistrationState state) noexcept
{
    switch (state) {
    case core::RegistrationState::Offline:
        return AccountsViewModel::State::Offline;
    case core::RegistrationState::Registering:
        return AccountsViewModel::State::Registering;
    case core::RegistrationState::Registered:
        return AccountsViewModel::State::Registered;
    case core::RegistrationState::Failed:
        return AccountsViewModel::State::Failed;
    case core::RegistrationState::Unregistering:
        return AccountsViewModel::State::Unregistering;
    }
    return AccountsViewModel::State::Offline;
}

[[nodiscard]] AccountsViewModel::Problem toViewProblem(core::RegistrationErrorCategory category)
{
    switch (category) {
    case core::RegistrationErrorCategory::None:
        return AccountsViewModel::Problem::None;
    case core::RegistrationErrorCategory::InvalidConfiguration:
        return AccountsViewModel::Problem::InvalidConfiguration;
    case core::RegistrationErrorCategory::PasswordUnavailable:
        return AccountsViewModel::Problem::PasswordUnavailable;
    case core::RegistrationErrorCategory::AuthenticationRejected:
        return AccountsViewModel::Problem::AuthenticationRejected;
    case core::RegistrationErrorCategory::Forbidden:
        return AccountsViewModel::Problem::Forbidden;
    case core::RegistrationErrorCategory::AccountNotFound:
        return AccountsViewModel::Problem::AccountNotFound;
    case core::RegistrationErrorCategory::RegistrarUnreachable:
        return AccountsViewModel::Problem::RegistrarUnreachable;
    case core::RegistrationErrorCategory::DomainNotResolved:
        return AccountsViewModel::Problem::DomainNotResolved;
    case core::RegistrationErrorCategory::ServerUnavailable:
        return AccountsViewModel::Problem::ServerUnavailable;
    case core::RegistrationErrorCategory::CertificateNotTrusted:
        return AccountsViewModel::Problem::CertificateNotTrusted;
    case core::RegistrationErrorCategory::Unknown:
        return AccountsViewModel::Problem::Unknown;
    }
    return AccountsViewModel::Problem::Unknown;
}

}

AccountsViewModel::AccountsViewModel(core::Engine &engine, Dispatcher dispatch, QObject *parent)
    : QObject{parent}, engine_{engine}, dispatch_{std::move(dispatch)}
{
    qRegisterMetaType<core::AccountConfig>();
    engine_.setListener(this);
}

AccountsViewModel::~AccountsViewModel()
{
    engine_.setListener(nullptr);
}

QVariantMap AccountsViewModel::entryFor(const core::AccountConfig &account) const
{
    const auto registration = registrationOf(account.id);

    QVariantMap entry;
    entry.insert(QStringLiteral("id"), QString::fromStdString(account.id));
    entry.insert(QStringLiteral("label"), QString::fromStdString(accountLabel(account)));

    entry.insert(QStringLiteral("shortLabel"), QString::fromStdString(shortAccountLabel(account)));

    entry.insert(QStringLiteral("uri"),
                 QString::fromStdString(account.username + '@' + account.domain));
    entry.insert(QStringLiteral("enabled"), account.enabled);
    entry.insert(QStringLiteral("state"), QVariant::fromValue(registration.state));
    entry.insert(QStringLiteral("problem"), QVariant::fromValue(registration.problem));
    entry.insert(QStringLiteral("sipStatus"), registration.sipStatus);
    entry.insert(QStringLiteral("detail"), registration.detail);
    entry.insert(QStringLiteral("retrying"), registration.retrying);
    entry.insert(QStringLiteral("dialsFrom"), account.id == dialAccount_);

    entry.insert(QStringLiteral("leaving"),
                 std::ranges::find(leaving_, account.id) != leaving_.end());
    return entry;
}

QVariantList AccountsViewModel::accounts() const
{
    QVariantList entries;
    entries.reserve(static_cast<qsizetype>(accounts_.size()));

    for (const auto &account : accounts_) {
        entries.append(entryFor(account));
    }

    return entries;
}

QVariantMap AccountsViewModel::accountAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(accounts_.size())) {
        return {};
    }
    return entryFor(accounts_[static_cast<std::size_t>(index)]);
}

int AccountsViewModel::count() const noexcept
{
    return static_cast<int>(accounts_.size());
}

int AccountsViewModel::lineCount() const noexcept
{
    return static_cast<int>(
        std::ranges::count_if(accounts_, [](const core::AccountConfig &a) { return a.enabled; }));
}

bool AccountsViewModel::connected() const noexcept
{
    return std::ranges::any_of(accounts_, [this](const core::AccountConfig &account) {
        return registrationOf(account.id).state == State::Registered;
    });
}

bool AccountsViewModel::busy() const noexcept
{
    return std::ranges::any_of(accounts_, [this](const core::AccountConfig &account) {
        const auto state = registrationOf(account.id).state;
        return state == State::Registering || state == State::Unregistering;
    });
}

int AccountsViewModel::failedCount() const noexcept
{
    return static_cast<int>(
        std::ranges::count_if(accounts_, [this](const core::AccountConfig &account) {
            return account.enabled && registrationOf(account.id).state == State::Failed;
        }));
}

std::string AccountsViewModel::spotlight() const
{

    for (const auto &account : accounts_) {
        if (account.enabled && registrationOf(account.id).state == State::Failed) {
            return account.id;
        }
    }

    if (!dialAccount_.empty() && find(dialAccount_) != nullptr) {
        return dialAccount_;
    }
    for (const auto &account : accounts_) {
        if (account.enabled) {
            return account.id;
        }
    }
    return {};
}

QString AccountsViewModel::spotlightLabel() const
{

    const auto *account = find(spotlight());
    return account == nullptr ? QString{} : QString::fromStdString(shortAccountLabel(*account));
}

AccountsViewModel::State AccountsViewModel::spotlightState() const noexcept
{
    return registrationOf(spotlight()).state;
}

AccountsViewModel::Problem AccountsViewModel::spotlightProblem() const noexcept
{
    return registrationOf(spotlight()).problem;
}

int AccountsViewModel::spotlightSipStatus() const noexcept
{
    return registrationOf(spotlight()).sipStatus;
}

QString AccountsViewModel::spotlightDetail() const
{
    return registrationOf(spotlight()).detail;
}

bool AccountsViewModel::spotlightRetrying() const noexcept
{
    return registrationOf(spotlight()).retrying;
}

QString AccountsViewModel::dialAccount() const
{
    return QString::fromStdString(dialAccount_);
}

QString AccountsViewModel::dialLabel() const
{
    const auto *account = find(dialAccount_);
    return account == nullptr ? QString{} : QString::fromStdString(shortAccountLabel(*account));
}

void AccountsViewModel::applyAccounts(std::vector<core::AccountConfig> accounts,
                                      std::string dialAccount)
{
    accounts_ = std::move(accounts);
    dialAccount_ = std::move(dialAccount);

    if (editing_ && !edited_.id.empty() && find(edited_.id) == nullptr) {
        editing_ = false;
        edited_ = {};
        password_.clear();
        Q_EMIT selectionChanged();
        Q_EMIT editorChanged();
    }

    Q_EMIT accountsChanged();
}

void AccountsViewModel::applyPending(const std::vector<core::SettingChange> &pending)
{
    std::vector<std::string> leaving;
    for (const auto &change : pending) {
        if (change.area == core::SettingArea::Account && !change.subject.empty()) {
            leaving.push_back(change.subject);
        }
    }

    if (leaving == leaving_) {
        return;
    }
    leaving_ = std::move(leaving);
    Q_EMIT accountsChanged();
}

void AccountsViewModel::setKeyring(Keyring state)
{
    if (keyring_ == state) {
        return;
    }
    keyring_ = state;
    Q_EMIT keyringChanged();
}

QString AccountsViewModel::selectedId() const
{
    return QString::fromStdString(edited_.id);
}

QString AccountsViewModel::displayName() const
{
    return QString::fromStdString(edited_.displayName);
}

QString AccountsViewModel::username() const
{
    return QString::fromStdString(edited_.username);
}

QString AccountsViewModel::domain() const
{
    return QString::fromStdString(edited_.domain);
}

AccountsViewModel::Dtmf AccountsViewModel::dtmfTransport() const noexcept
{
    return edited_.dtmfTransport == core::DtmfTransport::SipInfo ? Dtmf::SipInfo : Dtmf::Rfc2833;
}

QString AccountsViewModel::authUser() const
{
    return QString::fromStdString(edited_.authUser);
}

QString AccountsViewModel::registrar() const
{
    return QString::fromStdString(edited_.registrar);
}

QString AccountsViewModel::outboundProxy() const
{
    return QString::fromStdString(edited_.outboundProxy);
}

AccountsViewModel::Transport AccountsViewModel::transport() const noexcept
{
    switch (edited_.transport) {
    case core::SipTransport::Udp:
        break;
    case core::SipTransport::Tcp:
        return Transport::Tcp;
    case core::SipTransport::Tls:
        return Transport::Tls;
    }
    return Transport::Udp;
}

bool AccountsViewModel::verifyCertificate() const noexcept
{
    return edited_.verifyCertificate;
}

AccountsViewModel::Encryption AccountsViewModel::mediaEncryption() const noexcept
{
    switch (edited_.mediaEncryption) {
    case core::MediaEncryption::Off:
        return Encryption::Off;
    case core::MediaEncryption::Offered:
        break;
    case core::MediaEncryption::Required:
        return Encryption::Required;
    }
    return Encryption::Offered;
}

int AccountsViewModel::expirySeconds() const noexcept
{
    return expirySeconds_;
}

AccountsViewModel::Fault AccountsViewModel::fault() const
{

    if (port_ <= 0 || port_ > 65535) {
        return Fault::InvalidPort;
    }

    auto record = edited();

    if (record.id.empty()) {
        record.id = "unsaved";
    }

    switch (core::validate(record)) {
    case core::ConfigProblem::None:
        return Fault::None;
    case core::ConfigProblem::MissingUsername:
        return Fault::MissingUsername;
    case core::ConfigProblem::MissingDomain:
        return Fault::MissingDomain;
    case core::ConfigProblem::InvalidPort:
        return Fault::InvalidPort;
    case core::ConfigProblem::InvalidExpiry:
        return Fault::InvalidExpiry;
    case core::ConfigProblem::MissingId:
        break;
    }
    return Fault::None;
}

bool AccountsViewModel::canSave() const
{
    return editing_ && fault() == Fault::None;
}

AccountsViewModel::State AccountsViewModel::selectedState() const noexcept
{
    return registrationOf(edited_.id).state;
}

AccountsViewModel::State AccountsViewModel::stateOf(const QString &accountId) const
{
    return registrationOf(accountId.toStdString()).state;
}

QString AccountsViewModel::labelOf(const QString &accountId) const
{

    const auto *account = find(accountId.toStdString());
    return account == nullptr ? QString{} : QString::fromStdString(shortAccountLabel(*account));
}

QVariantMap AccountsViewModel::chipPlaces(const QVariantList &widths, double available,
                                          double spacing, double rowHeight, double rowSpacing) const
{

    return wrapAsVariant(widths, available, spacing, rowHeight, rowSpacing);
}

void AccountsViewModel::setDisplayName(const QString &value)
{
    if (displayName() == value) {
        return;
    }
    edited_.displayName = value.toStdString();
    Q_EMIT editorChanged();
}

void AccountsViewModel::setUsername(const QString &value)
{
    if (username() == value) {
        return;
    }
    edited_.username = value.toStdString();
    Q_EMIT editorChanged();
}

void AccountsViewModel::setDomain(const QString &value)
{
    if (domain() == value) {
        return;
    }
    edited_.domain = value.toStdString();
    Q_EMIT editorChanged();
}

void AccountsViewModel::setPassword(const QString &value)
{
    if (password_ == value) {
        return;
    }
    password_ = value;
    Q_EMIT editorChanged();
}

void AccountsViewModel::setPort(int value)
{
    if (port_ == value) {
        return;
    }
    port_ = value;
    Q_EMIT editorChanged();
}

void AccountsViewModel::setAuthUser(const QString &value)
{
    if (authUser() == value) {
        return;
    }
    edited_.authUser = value.toStdString();
    Q_EMIT editorChanged();
}

void AccountsViewModel::setRegistrar(const QString &value)
{
    if (registrar() == value) {
        return;
    }
    edited_.registrar = value.toStdString();
    Q_EMIT editorChanged();
}

void AccountsViewModel::setOutboundProxy(const QString &value)
{
    if (outboundProxy() == value) {
        return;
    }
    edited_.outboundProxy = value.toStdString();
    Q_EMIT editorChanged();
}

void AccountsViewModel::setTransport(Transport value)
{
    auto wanted = core::SipTransport::Udp;
    if (value == Transport::Tcp) {
        wanted = core::SipTransport::Tcp;
    } else if (value == Transport::Tls) {
        wanted = core::SipTransport::Tls;
    }

    if (edited_.transport == wanted) {
        return;
    }

    if (port_ == core::AccountConfig::defaultPortFor(edited_.transport)) {
        port_ = core::AccountConfig::defaultPortFor(wanted);
    }
    edited_.transport = wanted;
    Q_EMIT editorChanged();
}

void AccountsViewModel::setVerifyCertificate(bool value)
{
    if (edited_.verifyCertificate == value) {
        return;
    }
    edited_.verifyCertificate = value;
    Q_EMIT editorChanged();
}

void AccountsViewModel::setMediaEncryption(Encryption value)
{
    auto wanted = core::MediaEncryption::Offered;
    if (value == Encryption::Off) {
        wanted = core::MediaEncryption::Off;
    } else if (value == Encryption::Required) {
        wanted = core::MediaEncryption::Required;
    }

    if (edited_.mediaEncryption == wanted) {
        return;
    }
    edited_.mediaEncryption = wanted;
    Q_EMIT editorChanged();
}

void AccountsViewModel::setExpirySeconds(int value)
{
    if (expirySeconds_ == value) {
        return;
    }
    expirySeconds_ = value;
    Q_EMIT editorChanged();
}

void AccountsViewModel::setDtmfTransport(Dtmf value)
{
    const auto wanted =
        value == Dtmf::SipInfo ? core::DtmfTransport::SipInfo : core::DtmfTransport::Rfc2833;
    if (edited_.dtmfTransport == wanted) {
        return;
    }
    edited_.dtmfTransport = wanted;
    Q_EMIT editorChanged();
}

bool AccountsViewModel::autoAnswer() const noexcept
{
    return edited_.autoAnswer;
}

int AccountsViewModel::autoAnswerAfterSeconds() const noexcept
{
    return static_cast<int>(edited_.autoAnswerAfter.count());
}

void AccountsViewModel::setAutoAnswer(bool value)
{
    if (edited_.autoAnswer == value) {
        return;
    }
    edited_.autoAnswer = value;
    Q_EMIT editorChanged();
}

void AccountsViewModel::setAutoAnswerAfterSeconds(int value)
{

    const auto wanted = std::chrono::seconds{
        std::clamp(value, 0, static_cast<int>(core::kMaxAutoAnswerSetting.count()))};
    if (edited_.autoAnswerAfter == wanted) {
        return;
    }
    edited_.autoAnswerAfter = wanted;
    Q_EMIT editorChanged();
}

core::AccountConfig AccountsViewModel::edited() const
{

    auto config = edited_;
    config.username = QString::fromStdString(config.username).trimmed().toStdString();
    config.domain = QString::fromStdString(config.domain).trimmed().toStdString();
    config.displayName = QString::fromStdString(config.displayName).trimmed().toStdString();
    config.authUser = QString::fromStdString(config.authUser).trimmed().toStdString();
    config.registrar = QString::fromStdString(config.registrar).trimmed().toStdString();
    config.outboundProxy = QString::fromStdString(config.outboundProxy).trimmed().toStdString();
    config.port = static_cast<std::uint16_t>(port_);
    config.expiry = std::chrono::seconds{expirySeconds_};
    return config;
}

std::vector<TrayLine> AccountsViewModel::lines() const
{
    std::vector<TrayLine> lines;
    lines.reserve(accounts_.size());

    for (const auto &account : accounts_) {

        lines.push_back(TrayLine{core::shortAccountLabel(account), account.enabled,
                                 fromMirror(registrationOf(account.id).state)});
    }

    return lines;
}

core::AccountConfig AccountsViewModel::accountRecord(const QString &accountId) const
{
    const auto *account = find(accountId.toStdString());
    return account == nullptr ? core::AccountConfig{} : *account;
}

void AccountsViewModel::select(const QString &accountId)
{
    const auto *account = find(accountId.toStdString());
    if (account == nullptr) {
        cancelEdit();
        return;
    }

    if (editing_ && edited_.id == account->id) {
        cancelEdit();
        return;
    }

    editing_ = true;
    edited_ = *account;
    port_ = account->port;
    expirySeconds_ = static_cast<int>(account->expiry.count());

    password_.clear();

    Q_EMIT selectionChanged();
    Q_EMIT editorChanged();
}

void AccountsViewModel::addAccount()
{
    editing_ = true;
    edited_ = core::AccountConfig{};
    port_ = core::AccountConfig{}.port;
    expirySeconds_ = static_cast<int>(core::AccountConfig{}.expiry.count());
    password_.clear();

    Q_EMIT selectionChanged();
    Q_EMIT editorChanged();
}

void AccountsViewModel::cancelEdit()
{
    if (!editing_) {
        return;
    }
    editing_ = false;
    edited_ = {};
    port_ = core::AccountConfig{}.port;
    expirySeconds_ = static_cast<int>(core::AccountConfig{}.expiry.count());
    password_.clear();

    Q_EMIT selectionChanged();
    Q_EMIT editorChanged();
}

void AccountsViewModel::save()
{
    if (!canSave()) {
        return;
    }
    Q_EMIT accountSubmitted(edited(), password_);

    cancelEdit();
}

void AccountsViewModel::removeAccount(const QString &accountId)
{
    if (find(accountId.toStdString()) == nullptr) {
        return;
    }
    Q_EMIT accountDropped(accountId);
}

void AccountsViewModel::setAccountEnabled(const QString &accountId, bool enabled)
{
    const auto *account = find(accountId.toStdString());
    if (account == nullptr || account->enabled == enabled) {
        return;
    }

    auto changed = *account;
    changed.enabled = enabled;
    Q_EMIT accountChanged(changed);
}

void AccountsViewModel::connectAccount(const QString &accountId)
{
    const auto *account = find(accountId.toStdString());
    if (account == nullptr || !account->enabled) {
        return;
    }

    dispatch_([this, config = *account]() {
        static_cast<void>(engine_.addAccount(config));
        static_cast<void>(engine_.startRegistration(config.id));
    });
}

int AccountsViewModel::reconnectWhatIsDown()
{
    int asked = 0;
    for (const auto &account : accounts_) {
        if (!account.enabled) {
            continue;
        }

        const auto state = registrationOf(account.id).state;
        if (state == State::Registered || state == State::Registering) {
            continue;
        }

        dispatch_([this, config = account]() {
            static_cast<void>(engine_.addAccount(config));
            static_cast<void>(engine_.startRegistration(config.id));
        });
        ++asked;
    }

    if (asked > 0) {

        const auto howMany = std::to_string(asked);
        const core::LogAttribute fields[] = {{"lines", howMany}};
        core::Logger::instance().log(core::LogLevel::Info, "registration",
                                     "something changed outside; asking the lines that are down "
                                     "to register again",
                                     fields);
    }
    return asked;
}

void AccountsViewModel::disconnectAccount(const QString &accountId)
{
    dispatch_([this, id = accountId.toStdString()]() {
        static_cast<void>(engine_.stopRegistration(id));
    });
}

void AccountsViewModel::chooseDialAccount(const QString &accountId)
{
    const auto *account = find(accountId.toStdString());
    if (account == nullptr || !account->enabled || account->id == dialAccount_) {
        return;
    }

    dialAccount_ = account->id;
    Q_EMIT accountsChanged();
    Q_EMIT dialAccountChosen(accountId);
}

void AccountsViewModel::moveAccount(const QString &accountId, int by)
{
    const auto id = accountId.toStdString();
    const auto found = std::ranges::find(accounts_, id, &core::AccountConfig::id);
    if (found == accounts_.end() || by == 0) {
        return;
    }

    const auto from = static_cast<int>(std::distance(accounts_.begin(), found));
    const auto to = from + by;
    if (to < 0 || to >= static_cast<int>(accounts_.size())) {
        return;
    }

    std::swap(accounts_.at(static_cast<std::size_t>(from)),
              accounts_.at(static_cast<std::size_t>(to)));
    Q_EMIT accountsChanged();

    QStringList order;
    order.reserve(static_cast<qsizetype>(accounts_.size()));
    for (const auto &account : accounts_) {
        order.append(QString::fromStdString(account.id));
    }
    Q_EMIT accountOrderChanged(order);
}

void AccountsViewModel::onRegistrationStateChanged(const std::string &accountId,
                                                   core::RegistrationState state,
                                                   const core::RegistrationError &error)
{

    const bool retrying = engine_.retryPending(accountId);

    QMetaObject::invokeMethod(
        this,
        [this, accountId, state, error, retrying]() {
            auto &registration = registrations_[accountId];
            registration.state = toViewState(state);
            registration.problem = toViewProblem(error.category);
            registration.sipStatus = error.sipStatus;
            registration.detail = QString::fromStdString(error.detail);
            registration.retrying = retrying;

            Q_EMIT accountsChanged();
        },
        Qt::QueuedConnection);
}

void AccountsViewModel::couldNotReadThePassword(const std::string &accountId,
                                                std::string_view keyringSaid)
{
    core::RegistrationError error;
    error.category = core::RegistrationErrorCategory::PasswordUnavailable;
    error.detail = std::string{keyringSaid};

    onRegistrationStateChanged(accountId, core::RegistrationState::Failed, error);
}

const core::AccountConfig *AccountsViewModel::find(const std::string &accountId) const
{
    if (accountId.empty()) {
        return nullptr;
    }
    const auto found = std::ranges::find(accounts_, accountId, &core::AccountConfig::id);
    return found == accounts_.end() ? nullptr : &*found;
}

AccountsViewModel::Registration
AccountsViewModel::registrationOf(const std::string &accountId) const
{
    const auto found = registrations_.find(accountId);
    return found == registrations_.end() ? Registration{} : found->second;
}

}
