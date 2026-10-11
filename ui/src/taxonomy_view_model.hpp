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

#include <softphone/core/taxonomy.hpp>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <functional>
#include <vector>

namespace softphone::ui {

class TaxonomyViewModel : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `taxonomy`.")

    Q_PROPERTY(QVariantList categories READ categories NOTIFY rowsChanged)
    Q_PROPERTY(QVariantList tags READ tags NOTIFY rowsChanged)
    Q_PROPERTY(QVariantList redFlags READ redFlags NOTIFY rowsChanged)
    Q_PROPERTY(QVariantList instructions READ instructions NOTIFY rowsChanged)

    Q_PROPERTY(bool showRetired READ showRetired WRITE setShowRetired NOTIFY rowsChanged)

    Q_PROPERTY(int count READ count NOTIFY rowsChanged)

    Q_PROPERTY(QString editingId READ editingId NOTIFY editingChanged)
    Q_PROPERTY(bool editing READ editing NOTIFY editingChanged)

    Q_PROPERTY(QString editingKind READ editingKind NOTIFY editingChanged)

    Q_PROPERTY(QString code READ code WRITE setCode NOTIFY draftChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY draftChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY draftChanged)

    Q_PROPERTY(QString severity READ severity WRITE setSeverity NOTIFY draftChanged)

    Q_PROPERTY(bool codeLocked READ codeLocked NOTIFY draftChanged)

    Q_PROPERTY(bool builtIn READ builtIn NOTIFY draftChanged)

    Q_PROPERTY(bool retired READ retired WRITE setRetired NOTIFY draftChanged)

    Q_PROPERTY(bool complete READ complete NOTIFY draftChanged)

    Q_PROPERTY(QString refusal READ refusal NOTIFY draftChanged)

public:
    using Save = std::function<void(core::TaxonomyEntry row)>;
    using Remove = std::function<void(const QString &id)>;
    using Plain = std::function<void()>;

    explicit TaxonomyViewModel(QObject *parent = nullptr);

    [[nodiscard]] QVariantList categories() const;
    [[nodiscard]] QVariantList tags() const;
    [[nodiscard]] QVariantList redFlags() const;
    [[nodiscard]] QVariantList instructions() const;

    [[nodiscard]] Q_INVOKABLE QVariantList rowsNow(const QString &kind) const;

    [[nodiscard]] bool showRetired() const noexcept
    {
        return showRetired_;
    }

    [[nodiscard]] int count() const noexcept
    {
        return static_cast<int>(rows_.size());
    }

    [[nodiscard]] QString editingId() const
    {
        return editingId_;
    }

    [[nodiscard]] bool editing() const noexcept
    {
        return editing_;
    }

    [[nodiscard]] QString editingKind() const
    {
        return editingKind_;
    }

    [[nodiscard]] QString code() const
    {
        return draft_.code;
    }

    [[nodiscard]] QString title() const
    {
        return draft_.title;
    }

    [[nodiscard]] QString description() const
    {
        return draft_.description;
    }

    [[nodiscard]] QString severity() const
    {
        return draft_.severity;
    }

    [[nodiscard]] bool codeLocked() const;

    [[nodiscard]] bool builtIn() const noexcept
    {
        return draft_.builtIn;
    }

    [[nodiscard]] bool retired() const noexcept
    {
        return draft_.retired;
    }

    [[nodiscard]] bool complete() const;
    [[nodiscard]] QString refusal() const;

    [[nodiscard]] Q_INVOKABLE QString categoryTitle(const QString &code) const;

    [[nodiscard]] Q_INVOKABLE QString titleOf(const QString &kind, const QString &code) const;

    [[nodiscard]] Q_INVOKABLE QString severityOf(const QString &code) const;

    [[nodiscard]] static QString titleFor(const QString &code, const QString &stored, bool builtIn);
    [[nodiscard]] static QString descriptionFor(const QString &code, const QString &stored,
                                                bool builtIn);

public Q_SLOTS:
    void setShowRetired(bool value);
    void setRetired(bool value);
    void setCode(const QString &value);
    void setTitle(const QString &value);
    void setDescription(const QString &value);
    void setSeverity(const QString &value);

public:

    Q_INVOKABLE void add(const QString &kind);

    Q_INVOKABLE void edit(const QString &id);
    Q_INVOKABLE void cancel();

    Q_INVOKABLE void save();

    Q_INVOKABLE void remove(const QString &id);

    [[nodiscard]] Q_INVOKABLE QVariantMap rowFor(const QString &id) const;

    Q_INVOKABLE void retranslated();

    void setEntries(std::vector<core::TaxonomyEntry> rows, QStringList usedCategories,
                    QStringList usedTags, QStringList usedFlags);

    Q_INVOKABLE void restoreEverything();

    void setActions(Save save, Remove remove, Plain refresh, Plain restoreAll);

Q_SIGNALS:
    void rowsChanged();
    void editingChanged();
    void draftChanged();

private:
    struct Draft {
        core::TaxonomyKind kind{core::TaxonomyKind::Category};
        QString code;
        QString title;
        QString description;
        QString severity{"medium"};
        bool builtIn{false};
        bool retired{false};

        QString openedTitle;
        QString openedDescription;
    };

    [[nodiscard]] QVariantList rowsOf(core::TaxonomyKind kind) const;
    [[nodiscard]] const core::TaxonomyEntry *find(const QString &id) const;
    [[nodiscard]] bool inUse(core::TaxonomyKind kind, const QString &code) const;
    [[nodiscard]] core::TaxonomyEntry asRow() const;
    void openOn(const core::TaxonomyEntry *row, core::TaxonomyKind kind);

    std::vector<core::TaxonomyEntry> rows_;
    QStringList usedCategories_;
    QStringList usedTags_;
    QStringList usedFlags_;
    bool showRetired_{false};

    QString editingId_;
    QString editingKind_;
    bool editing_{false};
    Draft draft_;

    Save save_;
    Remove remove_;
    Plain refresh_;
    Plain restoreAll_;
};

}
