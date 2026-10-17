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

#include <softphone/core/asr.hpp>
#include <softphone/core/llm.hpp>
#include <softphone/core/provider_knobs.hpp>
#include <softphone/core/providers.hpp>

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace softphone::ui {

class ProvidersViewModel : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `providers`.")

    Q_PROPERTY(QString kind READ kindText CONSTANT)

    Q_PROPERTY(bool priced READ priced CONSTANT)

    Q_PROPERTY(bool listens READ listens CONSTANT)

    Q_PROPERTY(QVariantList rows READ rows NOTIFY rowsChanged)
    Q_PROPERTY(int count READ count NOTIFY rowsChanged)

    Q_PROPERTY(QVariantList types READ types NOTIFY typesChanged)

    Q_PROPERTY(QVariantList usable READ usable NOTIFY rowsChanged)

    Q_PROPERTY(QString editingId READ editingId NOTIFY editingChanged)
    Q_PROPERTY(bool editing READ editing NOTIFY editingChanged)

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY draftChanged)
    Q_PROPERTY(QString type READ type WRITE setType NOTIFY draftChanged)
    Q_PROPERTY(QString endpoint READ endpoint WRITE setEndpoint NOTIFY draftChanged)

    Q_PROPERTY(QString realtimeEndpoint READ realtimeEndpoint WRITE setRealtimeEndpoint NOTIFY
                   draftChanged)

    Q_PROPERTY(bool canStream READ canStream NOTIFY draftChanged)

    Q_PROPERTY(bool canTranscribe READ canTranscribe NOTIFY draftChanged)

    Q_PROPERTY(bool modelAnswersWithoutTimes READ modelAnswersWithoutTimes NOTIFY draftChanged)

    Q_PROPERTY(QString realtimeModel READ realtimeModel WRITE setRealtimeModel NOTIFY draftChanged)

    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY draftChanged)
    Q_PROPERTY(QString model READ model WRITE setModel NOTIFY draftChanged)
    Q_PROPERTY(QString region READ region WRITE setRegion NOTIFY draftChanged)

    Q_PROPERTY(QVariantList knobs READ knobs NOTIFY knobsChanged)

    Q_PROPERTY(QString params READ params WRITE setParams NOTIFY draftChanged)

    Q_PROPERTY(QString token READ token WRITE setToken NOTIFY draftChanged)

    Q_PROPERTY(bool hasToken READ hasToken NOTIFY draftChanged)

    Q_PROPERTY(QString inPrice READ inPrice WRITE setInPrice NOTIFY draftChanged)

    Q_PROPERTY(QString realtimePrice READ realtimePrice WRITE setRealtimePrice NOTIFY draftChanged)
    Q_PROPERTY(QString outPrice READ outPrice WRITE setOutPrice NOTIFY draftChanged)

    Q_PROPERTY(QString waitMinutes READ waitMinutes WRITE setWaitMinutes NOTIFY draftChanged)

    Q_PROPERTY(bool isDefault READ isDefault WRITE setIsDefault NOTIFY draftChanged)

    Q_PROPERTY(bool isRealtimeDefault READ isRealtimeDefault WRITE setIsRealtimeDefault NOTIFY
                   draftChanged)
    Q_PROPERTY(bool switchedOn READ switchedOn WRITE setSwitchedOn NOTIFY draftChanged)

    Q_PROPERTY(bool complete READ complete NOTIFY draftChanged)

    Q_PROPERTY(bool needsModel READ needsModel NOTIFY draftChanged)

    Q_PROPERTY(QString testState READ testState NOTIFY testChanged)

    Q_PROPERTY(QString testDetail READ testDetail NOTIFY testChanged)

    Q_PROPERTY(QStringList testModels READ testModels NOTIFY testChanged)

    Q_PROPERTY(QStringList offeredModels READ offeredModels NOTIFY testChanged)
    Q_PROPERTY(QStringList offeredRealtimeModels READ offeredRealtimeModels NOTIFY testChanged)

public:
    using Plain = std::function<void()>;

    using Save = std::function<void(core::ProviderConfig row, const QString &secret)>;
    using Remove = std::function<void(const QString &id)>;

    using Test = std::function<void(core::ProviderConfig row, const QString &secret)>;

    explicit ProvidersViewModel(QObject *parent = nullptr);

    void setKind(core::ProviderKind kind);

    void setCountry(const QString &country);

    [[nodiscard]] core::ProviderKind kind() const noexcept
    {
        return kind_;
    }

    [[nodiscard]] QString kindText() const;

    [[nodiscard]] bool priced() const noexcept
    {
        return kind_ == core::ProviderKind::Llm;
    }

    [[nodiscard]] bool listens() const noexcept
    {
        return kind_ == core::ProviderKind::Asr;
    }

    [[nodiscard]] QVariantList rows() const;

    [[nodiscard]] Q_INVOKABLE QVariantList rowsNow() const;

    [[nodiscard]] int count() const noexcept
    {
        return static_cast<int>(rows_.size());
    }

    [[nodiscard]] QVariantList types() const;

    [[nodiscard]] QVariantList usable() const;

    [[nodiscard]] QString labelForType(const QString &id) const;

    [[nodiscard]] QString editingId() const
    {
        return editingId_;
    }

    [[nodiscard]] bool editing() const noexcept
    {
        return editing_;
    }

    [[nodiscard]] QString name() const
    {
        return draft_.name;
    }

    [[nodiscard]] QString type() const
    {
        return draft_.type;
    }

    [[nodiscard]] QString endpoint() const
    {
        return draft_.endpoint;
    }

    [[nodiscard]] QString realtimeEndpoint() const
    {
        return draft_.realtimeEndpoint;
    }

    [[nodiscard]] QString language() const
    {
        return draft_.language;
    }

    [[nodiscard]] QString realtimeModel() const
    {
        return draft_.realtimeModel;
    }

    [[nodiscard]] bool canStream() const;
    [[nodiscard]] bool canTranscribe() const;
    [[nodiscard]] bool modelAnswersWithoutTimes() const;

    [[nodiscard]] QString liveAddress(const QString &type) const;

    [[nodiscard]] QString model() const
    {
        return draft_.model;
    }

    [[nodiscard]] QString region() const
    {
        return draft_.region;
    }

    [[nodiscard]] QString params() const
    {
        return draft_.params;
    }

    [[nodiscard]] QVariantList knobs() const;

    [[nodiscard]] Q_INVOKABLE QString knobValue(const QString &name) const;

    Q_INVOKABLE void setKnob(const QString &name, const QString &value);

    [[nodiscard]] QString token() const
    {
        return draft_.token;
    }

    [[nodiscard]] bool hasToken() const noexcept
    {
        return draft_.hasSecret;
    }

    [[nodiscard]] QString waitMinutes() const
    {
        return draft_.waitMinutes;
    }

    [[nodiscard]] QString realtimePrice() const
    {
        return draft_.realtimePrice;
    }

    [[nodiscard]] QString inPrice() const
    {
        return draft_.inPrice;
    }

    [[nodiscard]] QString outPrice() const
    {
        return draft_.outPrice;
    }

    [[nodiscard]] bool isDefault() const noexcept
    {
        return draft_.isDefault;
    }

    [[nodiscard]] bool isRealtimeDefault() const noexcept
    {
        return draft_.isRealtimeDefault;
    }

    [[nodiscard]] bool switchedOn() const noexcept
    {
        return draft_.enabled;
    }

    [[nodiscard]] bool complete() const;
    [[nodiscard]] bool needsModel() const;

    [[nodiscard]] bool modelRequired() const;

    [[nodiscard]] Q_INVOKABLE QString liveModelExample() const;

    [[nodiscard]] Q_INVOKABLE QString addressExample() const;

    [[nodiscard]] Q_INVOKABLE QString liveAddressExample() const;

    Q_PROPERTY(bool runsHere READ runsHere NOTIFY draftChanged)
    [[nodiscard]] bool runsHere() const;

    [[nodiscard]] QString testState() const
    {
        return testState_;
    }

    [[nodiscard]] QString testDetail() const
    {
        return testDetail_;
    }

    [[nodiscard]] QStringList offeredModels() const;
    [[nodiscard]] QStringList offeredRealtimeModels() const;

    [[nodiscard]] QStringList testModels() const
    {
        return testModels_;
    }

public Q_SLOTS:
    void setName(const QString &value);
    void setType(const QString &value);
    void setEndpoint(const QString &value);
    void setRealtimeEndpoint(const QString &value);
    void setRealtimeModel(const QString &value);
    void setLanguage(const QString &value);
    void setModel(const QString &value);
    void setRegion(const QString &value);
    void setParams(const QString &value);
    void setToken(const QString &value);
    void setWaitMinutes(const QString &value);
    void setInPrice(const QString &value);
    void setRealtimePrice(const QString &value);
    void setOutPrice(const QString &value);
    void setIsDefault(bool value);
    void setIsRealtimeDefault(bool value);
    void setSwitchedOn(bool value);

public:

    Q_INVOKABLE void add();

    Q_INVOKABLE void edit(const QString &id);

    Q_INVOKABLE void cancel();

    Q_INVOKABLE void save();

    Q_INVOKABLE void remove(const QString &id);

    Q_INVOKABLE void makeDefault(const QString &id, bool live, bool wanted);

    Q_INVOKABLE void test();

    void setProviders(std::vector<core::ProviderConfig> rows, bool available);
    void setTestResult(const core::AsrProbe &probe);
    void setTestResult(const core::LlmProbe &probe);
    void setActions(Save save, Remove remove, Test test, Plain refresh);

Q_SIGNALS:
    void rowsChanged();
    void editingChanged();
    void draftChanged();
    void typesChanged();
    void testChanged();
    void knobsChanged();

private:
    struct Draft {
        QString name;
        QString type{"openai"};
        QString endpoint;

        QString realtimeEndpoint;

        QString realtimeModel;
        QString language;

        QString model;
        QString region;
        QString params;

        std::vector<std::pair<QString, QString>> knobs;

        std::vector<std::pair<QString, QString>> learnt;

        QStringList modelsSeen;

        QString token;
        QString waitMinutes;
        QString inPrice;
        QString realtimePrice;
        QString outPrice;
        bool hasSecret{false};
        bool isDefault{false};
        bool isRealtimeDefault{false};
        bool enabled{true};
    };

    [[nodiscard]] core::ProviderConfig asRow() const;

    void seedKnobs(const std::vector<std::pair<std::string, std::string>> &stored);
    void openOn(const core::ProviderConfig *row);
    void clearTest();

    QString country_;
    core::ProviderKind kind_{core::ProviderKind::Asr};
    std::vector<core::ProviderConfig> rows_;
    bool available_{false};

    QString editingId_;
    bool editing_{false};
    Draft draft_;

    QString testState_{"idle"};
    QString testDetail_;
    QStringList testModels_;

    Save save_;
    Remove remove_;
    Test test_;
    Plain refresh_;
};

}
