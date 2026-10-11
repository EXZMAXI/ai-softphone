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

#include "tray_presentation.hpp"

#include <softphone/core/account_config.hpp>
#include <softphone/core/engine.hpp>
#include <softphone/core/registration.hpp>
#include <softphone/core/settings.hpp>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace softphone::ui {

class AccountsViewModel : public QObject, private core::EngineListener {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `accounts`.")

    Q_PROPERTY(QVariantList accounts READ accounts NOTIFY accountsChanged)

    Q_PROPERTY(int count READ count NOTIFY accountsChanged)

    Q_PROPERTY(int lineCount READ lineCount NOTIFY accountsChanged)
    Q_PROPERTY(bool severalLines READ severalLines NOTIFY accountsChanged)

    Q_PROPERTY(bool connected READ connected NOTIFY accountsChanged)

    Q_PROPERTY(QString spotlightLabel READ spotlightLabel NOTIFY accountsChanged)
    Q_PROPERTY(State spotlightState READ spotlightState NOTIFY accountsChanged)
    Q_PROPERTY(Problem spotlightProblem READ spotlightProblem NOTIFY accountsChanged)
    Q_PROPERTY(int spotlightSipStatus READ spotlightSipStatus NOTIFY accountsChanged)
    Q_PROPERTY(QString spotlightDetail READ spotlightDetail NOTIFY accountsChanged)
    Q_PROPERTY(bool spotlightRetrying READ spotlightRetrying NOTIFY accountsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY accountsChanged)

    Q_PROPERTY(int failedCount READ failedCount NOTIFY accountsChanged)

    Q_PROPERTY(QString dialAccount READ dialAccount NOTIFY accountsChanged)
    Q_PROPERTY(QString dialLabel READ dialLabel NOTIFY accountsChanged)

    Q_PROPERTY(Keyring keyring READ keyring NOTIFY keyringChanged)

    Q_PROPERTY(QString selectedId READ selectedId NOTIFY selectionChanged)
    Q_PROPERTY(bool editing READ editing NOTIFY selectionChanged)
    Q_PROPERTY(bool addingNew READ addingNew NOTIFY selectionChanged)

    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY editorChanged)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY editorChanged)
    Q_PROPERTY(QString domain READ domain WRITE setDomain NOTIFY editorChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY editorChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY editorChanged)
    Q_PROPERTY(Dtmf dtmfTransport READ dtmfTransport WRITE setDtmfTransport NOTIFY editorChanged)

    Q_PROPERTY(bool autoAnswer READ autoAnswer WRITE setAutoAnswer NOTIFY editorChanged)
    Q_PROPERTY(int autoAnswerAfterSeconds READ autoAnswerAfterSeconds WRITE
                   setAutoAnswerAfterSeconds NOTIFY editorChanged)

    Q_PROPERTY(int maxAutoAnswerAfterSeconds READ maxAutoAnswerAfterSeconds CONSTANT)

    Q_PROPERTY(QString authUser READ authUser WRITE setAuthUser NOTIFY editorChanged)
    Q_PROPERTY(QString registrar READ registrar WRITE setRegistrar NOTIFY editorChanged)
    Q_PROPERTY(QString outboundProxy READ outboundProxy WRITE setOutboundProxy NOTIFY editorChanged)
    Q_PROPERTY(Transport transport READ transport WRITE setTransport NOTIFY editorChanged)

    Q_PROPERTY(bool verifyCertificate READ verifyCertificate WRITE setVerifyCertificate NOTIFY
                   editorChanged)
    Q_PROPERTY(Encryption mediaEncryption READ mediaEncryption WRITE setMediaEncryption NOTIFY
                   editorChanged)
    Q_PROPERTY(int expirySeconds READ expirySeconds WRITE setExpirySeconds NOTIFY editorChanged)

    Q_PROPERTY(Fault fault READ fault NOTIFY editorChanged)
    Q_PROPERTY(bool canSave READ canSave NOTIFY editorChanged)

    Q_PROPERTY(State selectedState READ selectedState NOTIFY accountsChanged)

public:

    enum class State {
        Offline,
        Registering,
        Registered,
        Failed,
        Unregistering,
    };
    Q_ENUM(State)

    enum class Problem {
        None,
        InvalidConfiguration,
        PasswordUnavailable,
        AuthenticationRejected,
        Forbidden,
        AccountNotFound,
        RegistrarUnreachable,
        DomainNotResolved,
        ServerUnavailable,
        CertificateNotTrusted,
        Unknown,
    };
    Q_ENUM(Problem)

    enum class Dtmf {
        Rfc2833,
        SipInfo,
    };
    Q_ENUM(Dtmf)

    enum class Transport {
        Udp,
        Tcp,
        Tls,
    };
    Q_ENUM(Transport)

    enum class Encryption {
        Off,
        Offered,
        Required,
    };
    Q_ENUM(Encryption)

    enum class Fault {
        None,
        MissingUsername,
        MissingDomain,
        InvalidPort,
        InvalidExpiry,
    };
    Q_ENUM(Fault)

    enum class Keyring {

        Working,

        Sealed,

        Locked,

        Unreadable,

        Unavailable,
    };
    Q_ENUM(Keyring)

    using Dispatcher = std::function<void(std::function<void()>)>;

    AccountsViewModel(core::Engine &engine, Dispatcher dispatch, QObject *parent = nullptr);
    ~AccountsViewModel() override;

    [[nodiscard]] QVariantList accounts() const;

    Q_INVOKABLE [[nodiscard]] QVariantMap accountAt(int index) const;

    [[nodiscard]] int count() const noexcept;
    [[nodiscard]] int lineCount() const noexcept;

    [[nodiscard]] bool severalLines() const noexcept
    {
        return lineCount() > 1;
    }

    [[nodiscard]] bool connected() const noexcept;
    [[nodiscard]] bool busy() const noexcept;
    [[nodiscard]] int failedCount() const noexcept;

    [[nodiscard]] QString spotlightLabel() const;
    [[nodiscard]] State spotlightState() const noexcept;
    [[nodiscard]] Problem spotlightProblem() const noexcept;
    [[nodiscard]] int spotlightSipStatus() const noexcept;
    [[nodiscard]] QString spotlightDetail() const;
    [[nodiscard]] bool spotlightRetrying() const noexcept;

    [[nodiscard]] QString dialAccount() const;
    [[nodiscard]] QString dialLabel() const;

    [[nodiscard]] Keyring keyring() const noexcept
    {
        return keyring_;
    }

    void setKeyring(Keyring state);

    [[nodiscard]] QString selectedId() const;

    [[nodiscard]] bool editing() const noexcept
    {
        return editing_;
    }

    [[nodiscard]] bool addingNew() const noexcept
    {
        return editing_ && edited_.id.empty();
    }

    [[nodiscard]] QString displayName() const;
    [[nodiscard]] QString username() const;
    [[nodiscard]] QString domain() const;

    [[nodiscard]] QString password() const
    {
        return password_;
    }

    [[nodiscard]] int port() const noexcept
    {
        return port_;
    }

    [[nodiscard]] Dtmf dtmfTransport() const noexcept;
    [[nodiscard]] bool autoAnswer() const noexcept;
    [[nodiscard]] int autoAnswerAfterSeconds() const noexcept;

    [[nodiscard]] static int maxAutoAnswerAfterSeconds() noexcept
    {
        return static_cast<int>(core::kMaxAutoAnswerSetting.count());
    }

    [[nodiscard]] QString authUser() const;
    [[nodiscard]] QString registrar() const;
    [[nodiscard]] QString outboundProxy() const;
    [[nodiscard]] Transport transport() const noexcept;
    [[nodiscard]] bool verifyCertificate() const noexcept;
    [[nodiscard]] Encryption mediaEncryption() const noexcept;
    [[nodiscard]] int expirySeconds() const noexcept;
    [[nodiscard]] Fault fault() const;
    [[nodiscard]] bool canSave() const;
    [[nodiscard]] State selectedState() const noexcept;

    Q_INVOKABLE [[nodiscard]] State stateOf(const QString &accountId) const;

    Q_INVOKABLE [[nodiscard]] QString labelOf(const QString &accountId) const;

    Q_INVOKABLE [[nodiscard]] QVariantMap chipPlaces(const QVariantList &widths, double available,
                                                     double spacing, double rowHeight,
                                                     double rowSpacing) const;

    void setDisplayName(const QString &value);
    void setUsername(const QString &value);
    void setDomain(const QString &value);
    void setPassword(const QString &value);
    void setPort(int value);
    void setDtmfTransport(Dtmf value);
    void setAutoAnswer(bool value);
    void setAutoAnswerAfterSeconds(int value);
    void setAuthUser(const QString &value);
    void setRegistrar(const QString &value);
    void setOutboundProxy(const QString &value);

    void setTransport(Transport value);
    void setVerifyCertificate(bool value);
    void setMediaEncryption(Encryption value);
    void setExpirySeconds(int value);

    void applyAccounts(std::vector<core::AccountConfig> accounts, std::string dialAccount);

    void applyPending(const std::vector<core::SettingChange> &pending);

    [[nodiscard]] core::AccountConfig edited() const;

    [[nodiscard]] std::vector<TrayLine> lines() const;

    [[nodiscard]] core::AccountConfig accountRecord(const QString &accountId) const;

public Q_SLOTS:

    void select(const QString &accountId);

    void addAccount();

    void cancelEdit();

    void save();

    void removeAccount(const QString &accountId);

    void setAccountEnabled(const QString &accountId, bool enabled);

    void connectAccount(const QString &accountId);
    void disconnectAccount(const QString &accountId);

    int reconnectWhatIsDown();

    void chooseDialAccount(const QString &accountId);

    void moveAccount(const QString &accountId, int by);

Q_SIGNALS:
    void accountsChanged();
    void selectionChanged();
    void editorChanged();
    void keyringChanged();

    void accountSubmitted(const core::AccountConfig &account, const QString &password);

    void accountDropped(const QString &accountId);

    void accountChanged(const core::AccountConfig &account);
    void dialAccountChosen(const QString &accountId);

    void accountOrderChanged(const QStringList &accountIds);

public:

    void couldNotReadThePassword(const std::string &accountId, std::string_view keyringSaid);

private:

    [[nodiscard]] QVariantMap entryFor(const core::AccountConfig &account) const;

    struct Registration {
        State state{State::Offline};
        Problem problem{Problem::None};
        int sipStatus{0};
        QString detail;
        bool retrying{false};
    };

    void onRegistrationStateChanged(const std::string &accountId, core::RegistrationState state,
                                    const core::RegistrationError &error) override;

    [[nodiscard]] const core::AccountConfig *find(const std::string &accountId) const;
    [[nodiscard]] Registration registrationOf(const std::string &accountId) const;

    [[nodiscard]] std::string spotlight() const;

    core::Engine &engine_;
    Dispatcher dispatch_;

    std::vector<core::AccountConfig> accounts_;
    std::string dialAccount_;
    std::map<std::string, Registration> registrations_;
    std::vector<std::string> leaving_;

    Keyring keyring_{Keyring::Working};

    bool editing_{false};
    core::AccountConfig edited_;
    QString password_;

    int port_{5060};
    int expirySeconds_{300};
};

}

Q_DECLARE_METATYPE(softphone::core::AccountConfig)
