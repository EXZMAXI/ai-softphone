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

#include <softphone/core/answers.hpp>
#include <softphone/core/artifacts.hpp>
#include <softphone/core/templates.hpp>

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace softphone::ui {

class ArtifactsViewModel : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `artifacts`.")

    Q_PROPERTY(QVariantList tabs READ tabs NOTIFY rowsChanged)

    Q_PROPERTY(QString openId READ openId NOTIFY openChanged)

    Q_PROPERTY(QString openKind READ openKind NOTIFY openChanged)

    Q_PROPERTY(QString question READ question NOTIFY openChanged)

    Q_PROPERTY(QString content READ content NOTIFY openChanged)

    Q_PROPERTY(QString shape READ shape NOTIFY openChanged)

    Q_PROPERTY(QString structured READ structured NOTIFY openChanged)

    Q_PROPERTY(QString provenance READ provenance NOTIFY openChanged)
    Q_PROPERTY(QString madeBy READ madeBy NOTIFY openChanged)

    Q_PROPERTY(bool working READ working NOTIFY openChanged)

    Q_PROPERTY(QString failure READ failure NOTIFY openChanged)

    Q_PROPERTY(QVariantList choices READ choices NOTIFY choicesChanged)

    Q_PROPERTY(QString refusal READ refusal NOTIFY choicesChanged)

    Q_PROPERTY(bool exchangeKept READ exchangeKept NOTIFY openChanged)

public:

    using Names = std::function<QString(const QString &kind, const QString &code)>;

    using Run = std::function<void(const QString &templateId, const QString &question)>;
    using Forget = std::function<void(const QString &artifactId)>;
    using Save = std::function<void(const QString &target, const QString &text)>;
    using Plain = std::function<void()>;

    explicit ArtifactsViewModel(QObject *parent = nullptr);

    [[nodiscard]] QVariantList tabs() const;

    [[nodiscard]] Q_INVOKABLE QVariantList tabsNow() const;

    [[nodiscard]] QString openId() const
    {
        return openId_;
    }

    [[nodiscard]] QString openKind() const;
    [[nodiscard]] QString question() const;

    [[nodiscard]] QString content() const;
    [[nodiscard]] QString shape() const;
    [[nodiscard]] QString structured() const;
    [[nodiscard]] QString provenance() const;

    [[nodiscard]] QString madeBy() const;
    [[nodiscard]] bool working() const;
    [[nodiscard]] QString failure() const;
    [[nodiscard]] QVariantList choices() const;
    [[nodiscard]] QString refusal() const;
    [[nodiscard]] bool exchangeKept() const;

    [[nodiscard]] Q_INVOKABLE QString exchangeAsText() const;

    [[nodiscard]] static QString nameOf(const core::Artifact &row);

    [[nodiscard]] QString labelOf(const core::Artifact &row) const;

    [[nodiscard]] static QString labelForKind(const QString &kind);

    [[nodiscard]] static QString readingKindOf(const core::Artifact &row);

    Q_INVOKABLE void open(const QString &artifactId);

    Q_INVOKABLE void run(const QString &templateId, const QString &question = {});

    Q_INVOKABLE void forget();

    [[nodiscard]] Q_INVOKABLE QVariantMap openRow() const;

    [[nodiscard]] Q_INVOKABLE QString openName() const;

    [[nodiscard]] Q_INVOKABLE QString openAsText() const;

    [[nodiscard]] Q_INVOKABLE QString exportName() const;

    Q_INVOKABLE void saveTo(const QUrl &target);

    void setStem(const QString &stem);

    void setArtifacts(std::vector<core::Artifact> rows);
    void setTemplates(std::vector<core::PromptTemplate> rows, bool haveModel);

    void setProviderNames(std::vector<std::pair<QString, QString>> names);
    void setActions(Run run, Forget forget, Save save, Plain refresh);

    void setVocabulary(Names names);

    [[nodiscard]] Q_INVOKABLE QString entryAsText(int index) const;

    [[nodiscard]] Q_INVOKABLE QString severityText(const QString &severity) const;

    [[nodiscard]] Q_INVOKABLE QString verdictText(const QString &verdict) const;

Q_SIGNALS:
    void rowsChanged();
    void openChanged();
    void choicesChanged();

private:
    [[nodiscard]] const core::Artifact *current() const;

    [[nodiscard]] std::optional<core::Answer> openAnswer() const;
    [[nodiscard]] core::AnswerWords words() const;

    [[nodiscard]] static QString severityWord(core::Severity severity);
    [[nodiscard]] static QString verdictWord(const QString &verdict);

    Names names_;

    std::vector<core::Artifact> rows_;
    std::vector<core::PromptTemplate> templates_;
    bool haveModel_{false};

    QString openId_;

    QString wanting_;
    std::vector<std::string> known_;

    std::vector<std::pair<QString, QString>> providerNames_;

    QString following_;

    QString stem_;

    Run run_;
    Forget forget_;
    Save save_;
    Plain refresh_;
};

}
