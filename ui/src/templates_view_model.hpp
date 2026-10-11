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

class TemplatesViewModel : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `templates`.")

    Q_PROPERTY(QVariantList rows READ rows NOTIFY rowsChanged)

    Q_PROPERTY(QVariantList realtimeRows READ realtimeRows NOTIFY rowsChanged)
    Q_PROPERTY(int count READ count NOTIFY rowsChanged)

    Q_PROPERTY(QVariantList kinds READ kinds CONSTANT)

    Q_PROPERTY(QVariantList shapes READ shapes NOTIFY draftChanged)
    Q_PROPERTY(QVariantList policies READ policies CONSTANT)

    Q_PROPERTY(QStringList placeholders READ placeholders CONSTANT)

    Q_PROPERTY(QString editingId READ editingId NOTIFY editingChanged)
    Q_PROPERTY(bool editing READ editing NOTIFY editingChanged)

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY draftChanged)
    Q_PROPERTY(QString kind READ kind WRITE setKind NOTIFY draftChanged)
    Q_PROPERTY(QString shape READ shape WRITE setShape NOTIFY draftChanged)
    Q_PROPERTY(QString system READ system WRITE setSystem NOTIFY draftChanged)
    Q_PROPERTY(QString body READ body WRITE setBody NOTIFY draftChanged)
    Q_PROPERTY(QString policy READ policy WRITE setPolicy NOTIFY draftChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY draftChanged)
    Q_PROPERTY(QString params READ params WRITE setParams NOTIFY draftChanged)

    Q_PROPERTY(QString providerId READ providerId WRITE setProviderId NOTIFY draftChanged)

    Q_PROPERTY(QVariantList models READ models NOTIFY modelsChanged)

    Q_PROPERTY(QVariantList instructions READ instructions NOTIFY draftChanged)
    Q_PROPERTY(bool switchedOn READ switchedOn WRITE setSwitchedOn NOTIFY draftChanged)

    Q_PROPERTY(bool realtime READ realtime NOTIFY draftChanged)

    Q_PROPERTY(bool realtimeNear READ realtimeNear WRITE setRealtimeNear NOTIFY draftChanged)

    Q_PROPERTY(QVariantList recognisers READ recognisers NOTIFY recognisersChanged)
    Q_PROPERTY(QString asrProviderId READ asrProviderId WRITE setAsrProviderId NOTIFY draftChanged)

    Q_PROPERTY(QVariantList turnPolicies READ turnPolicies CONSTANT)
    Q_PROPERTY(QString turnPolicy READ turnPolicy WRITE setTurnPolicy NOTIFY draftChanged)

    Q_PROPERTY(bool asksAModel READ asksAModel NOTIFY draftChanged)

    Q_PROPERTY(bool complete READ complete NOTIFY draftChanged)

    Q_PROPERTY(QString refusal READ refusal NOTIFY draftChanged)

    Q_PROPERTY(bool builtIn READ builtIn NOTIFY draftChanged)

    Q_PROPERTY(bool restorable READ restorable NOTIFY draftChanged)

    Q_PROPERTY(QString preview READ preview NOTIFY previewChanged)

    Q_PROPERTY(QString previewSource READ previewSource NOTIFY previewChanged)

    Q_PROPERTY(QString previewProblem READ previewProblem NOTIFY previewChanged)

public:
    using Save = std::function<void(core::PromptTemplate row)>;
    using Remove = std::function<void(const QString &id)>;

    using Preview = std::function<void(core::PromptTemplate row)>;
    using Plain = std::function<void()>;

    using Order = std::function<void(std::vector<std::pair<std::string, int>> places)>;

    explicit TemplatesViewModel(QObject *parent = nullptr);

    [[nodiscard]] QVariantList rows() const;
    [[nodiscard]] QVariantList realtimeRows() const;

    [[nodiscard]] Q_INVOKABLE QVariantList rowsNow() const;

    [[nodiscard]] int count() const noexcept
    {
        return static_cast<int>(rows_.size());
    }

    [[nodiscard]] QVariantList kinds() const;
    [[nodiscard]] QVariantList shapes() const;
    [[nodiscard]] QVariantList policies() const;
    [[nodiscard]] QStringList placeholders() const;

    [[nodiscard]] static QString nameFor(const QString &code, const QString &stored);

    Q_INVOKABLE void retranslated();

    [[nodiscard]] static QString labelForKind(const QString &id);
    [[nodiscard]] static QString labelForShape(const QString &id);
    [[nodiscard]] static QString labelForPolicy(const QString &id);

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

    [[nodiscard]] QString kind() const
    {
        return draft_.kind;
    }

    [[nodiscard]] QString shape() const
    {
        return draft_.shape;
    }

    [[nodiscard]] QString system() const
    {
        return draft_.system;
    }

    [[nodiscard]] QString body() const
    {
        return draft_.body;
    }

    [[nodiscard]] QString policy() const
    {
        return draft_.policy;
    }

    [[nodiscard]] QString language() const
    {
        return draft_.language;
    }

    [[nodiscard]] QString params() const
    {
        return draft_.params;
    }

    [[nodiscard]] QString providerId() const
    {
        return draft_.providerId;
    }

    [[nodiscard]] QVariantList models() const;
    [[nodiscard]] QVariantList instructions() const;

    void setInstructionEntries(std::vector<core::TaxonomyEntry> entries);

    [[nodiscard]] Q_INVOKABLE QString shapeInstructionTitle() const;

    [[nodiscard]] Q_INVOKABLE QString shapeTitle(const QString &shape) const;

    [[nodiscard]] bool switchedOn() const noexcept
    {
        return draft_.enabled;
    }

    [[nodiscard]] bool realtime() const noexcept
    {
        return draft_.kind == QStringLiteral("realtime");
    }

    [[nodiscard]] bool realtimeNear() const noexcept
    {
        return draft_.realtimeNear;
    }

    [[nodiscard]] QVariantList recognisers() const;

    [[nodiscard]] QString asrProviderId() const noexcept
    {
        return draft_.asrProviderId;
    }

    [[nodiscard]] QVariantList turnPolicies() const;

    [[nodiscard]] QString turnPolicy() const noexcept
    {
        return draft_.turnPolicy;
    }

    [[nodiscard]] bool asksAModel() const;

    [[nodiscard]] std::string shippedIn() const;

    [[nodiscard]] bool complete() const;
    [[nodiscard]] QString refusal() const;

    [[nodiscard]] bool builtIn() const noexcept
    {
        return draft_.builtIn;
    }

    [[nodiscard]] bool restorable() const;

    [[nodiscard]] QString preview() const
    {
        return preview_;
    }

    [[nodiscard]] QString previewProblem() const
    {
        return previewProblem_;
    }

    [[nodiscard]] QString previewSource() const
    {
        return previewSource_;
    }

public Q_SLOTS:
    void setName(const QString &value);
    void setKind(const QString &value);
    void setShape(const QString &value);
    void setSystem(const QString &value);
    void setBody(const QString &value);
    void setPolicy(const QString &value);
    void setLanguage(const QString &value);
    void setParams(const QString &value);
    void setProviderId(const QString &value);

    void toggleInstruction(const QString &code, bool wanted);
    void setSwitchedOn(bool value);

    void setRealtimeNear(bool value);
    void setAsrProviderId(const QString &value);
    void setTurnPolicy(const QString &value);

public:
    Q_INVOKABLE void add();

    Q_INVOKABLE void addRealtime();
    Q_INVOKABLE void edit(const QString &id);
    Q_INVOKABLE void cancel();

    Q_INVOKABLE void save();

    Q_INVOKABLE void remove(const QString &id);

    Q_INVOKABLE void move(const QString &id, int delta);

    Q_INVOKABLE void duplicate();

    Q_INVOKABLE void restore();

    Q_INVOKABLE void askForPreview();

    Q_INVOKABLE void restoreEverything();

    [[nodiscard]] Q_INVOKABLE QVariantMap rowFor(const QString &id) const;

    void setTemplates(std::vector<core::PromptTemplate> rows);

    void setContentLanguage(QString language);

    void setModels(std::vector<core::ProviderConfig> models);

    void setRecognisers(std::vector<core::ProviderConfig> recognisers);
    void setPreview(const QString &text, const QString &problem, const QString &source = {});
    void setActions(Save save, Remove remove, Preview preview, Plain refresh, Plain restoreAll,
                    Order order);

Q_SIGNALS:
    void rowsChanged();
    void editingChanged();
    void draftChanged();
    void previewChanged();
    void modelsChanged();
    void recognisersChanged();

private:
    struct Draft {
        QString name;
        QString kind{"custom"};
        QString shape{"plain"};
        QString system;
        QString body;
        QString policy{"interface"};
        QString language;
        QString params;
        QString providerId;

        bool realtimeNear{false};
        QString asrProviderId;
        QString turnPolicy{"provider"};

        bool enabled{true};
        bool builtIn{false};
        QString code;
        QStringList instructions;
    };

    [[nodiscard]] QVariantList rowsOfKind(bool realtime) const;

    QString contentLanguage_{QStringLiteral("en")};

    [[nodiscard]] const core::PromptTemplate *find(const QString &id) const;
    [[nodiscard]] core::PromptTemplate asRow() const;
    void openOn(const core::PromptTemplate *row);

    std::vector<core::PromptTemplate> rows_;
    std::vector<core::ProviderConfig> models_;
    std::vector<core::ProviderConfig> recognisers_;
    std::vector<core::TaxonomyEntry> instructionEntries_;

    QString editingId_;
    bool editing_{false};
    Draft draft_;

    QString preview_;
    QString previewProblem_;
    QString previewSource_;

    Save save_;
    Remove remove_;
    Preview askPreview_;
    Plain refresh_;
    Plain restoreAll_;
    Order order_;
};

}
