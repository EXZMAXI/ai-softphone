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

#include <softphone/core/automations.hpp>
#include <softphone/core/providers.hpp>
#include <softphone/core/taxonomy.hpp>
#include <softphone/core/templates.hpp>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <functional>
#include <utility>
#include <vector>

namespace softphone::ui {

class AutomationsViewModel : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `automations`.")

    Q_PROPERTY(QVariantList rows READ rows NOTIFY rowsChanged)
    Q_PROPERTY(int count READ count NOTIFY rowsChanged)

    Q_PROPERTY(QVariantList actions READ actions CONSTANT)
    Q_PROPERTY(QVariantList fields READ fields CONSTANT)
    Q_PROPERTY(QVariantList operators READ operators CONSTANT)

    Q_PROPERTY(QVariantList prompts READ prompts NOTIFY promptsChanged)

    Q_PROPERTY(QString editingId READ editingId NOTIFY editingChanged)
    Q_PROPERTY(bool editing READ editing NOTIFY editingChanged)

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY draftChanged)
    Q_PROPERTY(QString action READ action WRITE setAction NOTIFY draftChanged)
    Q_PROPERTY(QString templateCode READ templateCode WRITE setTemplateCode NOTIFY draftChanged)
    Q_PROPERTY(QString tagCode READ tagCode WRITE setTagCode NOTIFY draftChanged)

    Q_PROPERTY(bool switchedOn READ switchedOn WRITE setSwitchedOn NOTIFY draftChanged)

    Q_PROPERTY(QVariantList conditions READ conditions NOTIFY conditionsChanged)

    Q_PROPERTY(bool complete READ complete NOTIFY draftChanged)

    Q_PROPERTY(QString sentence READ sentence NOTIFY draftChanged)

public:
    using Save = std::function<void(core::Automation row)>;
    using Remove = std::function<void(const QString &id)>;
    using Plain = std::function<void()>;

    explicit AutomationsViewModel(QObject *parent = nullptr);

    [[nodiscard]] QVariantList rows() const;

    [[nodiscard]] Q_INVOKABLE QVariantList rowsNow() const;

    [[nodiscard]] int count() const noexcept
    {
        return static_cast<int>(rows_.size());
    }

    [[nodiscard]] QVariantList actions() const;
    [[nodiscard]] QVariantList fields() const;
    [[nodiscard]] QVariantList operators() const;
    [[nodiscard]] QVariantList prompts() const;

    [[nodiscard]] static QString nameFor(const QString &code, const QString &stored);

    Q_INVOKABLE void retranslated();

    [[nodiscard]] static QString labelForAction(const QString &id);
    [[nodiscard]] static QString labelForField(const QString &id);
    [[nodiscard]] static QString labelForOperator(const QString &id);

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

    [[nodiscard]] QString action() const
    {
        return draft_.action;
    }

    [[nodiscard]] QString templateCode() const
    {
        return draft_.templateCode;
    }

    [[nodiscard]] QString tagCode() const
    {
        return draft_.tagCode;
    }

    [[nodiscard]] bool switchedOn() const noexcept
    {
        return draft_.enabled;
    }

    [[nodiscard]] QVariantList conditions() const;

    [[nodiscard]] Q_INVOKABLE QVariantList operatorsFor(const QString &field) const;

    [[nodiscard]] Q_INVOKABLE QVariantList valuesFor(const QString &field) const;

    [[nodiscard]] Q_INVOKABLE QString valueKindFor(const QString &field, const QString &op) const;

    [[nodiscard]] bool complete() const;
    [[nodiscard]] QString sentence() const;

    [[nodiscard]] Q_INVOKABLE QString sentenceFor(const QString &id) const;

public Q_SLOTS:
    void setName(const QString &value);
    void setAction(const QString &value);
    void setTemplateCode(const QString &value);
    void setTagCode(const QString &value);
    void setSwitchedOn(bool value);

public:
    Q_INVOKABLE void add();
    Q_INVOKABLE void edit(const QString &id);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void save();
    Q_INVOKABLE void remove(const QString &id);

    Q_INVOKABLE void toggle(const QString &id, bool on);

    Q_INVOKABLE void moveBy(const QString &id, int by);

    Q_INVOKABLE void addCondition();
    Q_INVOKABLE void removeCondition(int at);
    Q_INVOKABLE void setConditionField(int at, const QString &field);
    Q_INVOKABLE void setConditionOperator(int at, const QString &op);
    Q_INVOKABLE void setConditionValue(int at, const QString &value);

    [[nodiscard]] Q_INVOKABLE QVariantMap rowFor(const QString &id) const;

    void setAutomations(std::vector<core::Automation> rows);
    void setPrompts(std::vector<core::PromptTemplate> rows);

    void setRecognisers(std::vector<core::ProviderConfig> recognisers);
    void setModels(std::vector<core::ProviderConfig> models);

    void setVocabulary(std::vector<core::TaxonomyEntry> categories,
                       std::vector<core::TaxonomyEntry> tags,
                       std::vector<core::TaxonomyEntry> redFlags);
    void setAccounts(std::vector<std::pair<QString, QString>> accounts);

    void setActions(Save save, Remove remove, Plain refresh);

Q_SIGNALS:
    void rowsChanged();
    void editingChanged();
    void draftChanged();
    void promptsChanged();

    void conditionsChanged();

private:

    void settle(core::Condition &clause) const;

    struct Draft {
        QString name;
        QString action{"generate"};
        QString templateCode;
        QString tagCode;
        bool enabled{true};
        std::vector<core::Condition> conditions;
        QString code;
        bool builtIn{false};
        int ordinal{0};
    };

    [[nodiscard]] const core::Automation *find(const QString &id) const;
    [[nodiscard]] core::Automation asRow() const;
    [[nodiscard]] QString sentenceOf(const core::Automation &row) const;
    void openOn(const core::Automation *row);

    std::vector<core::Automation> rows_;
    std::vector<core::PromptTemplate> prompts_;
    std::vector<core::TaxonomyEntry> categories_;
    std::vector<core::TaxonomyEntry> tags_;
    std::vector<core::TaxonomyEntry> redFlags_;
    std::vector<std::pair<QString, QString>> accounts_;
    std::vector<core::ProviderConfig> recognisers_;
    std::vector<core::ProviderConfig> models_;

    QString editingId_;
    bool editing_{false};
    Draft draft_;

    Save save_;
    Remove remove_;
    Plain refresh_;
};

}
