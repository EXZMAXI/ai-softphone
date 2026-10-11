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

#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>

#include <functional>
#include <vector>

namespace softphone::shell {

namespace settingsBand {

inline constexpr auto kOverview = "overview";

inline constexpr auto kTelephone = "telephone";

inline constexpr auto kProcessing = "processing";

inline constexpr auto kApplication = "application";

inline constexpr auto kOther = "other";

}

class UiRegistry : public QObject {
    Q_OBJECT

    Q_PROPERTY(QStringList settingsSections READ settingsSections NOTIFY settingsSectionsChanged)

    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

    Q_PROPERTY(int slotRevision READ slotRevision NOTIFY slotsChanged)

    Q_PROPERTY(int markRevision READ markRevision NOTIFY marksChanged)

    Q_PROPERTY(bool settled READ settled NOTIFY settledChanged)

    Q_PROPERTY(QStringList apiResources READ apiResources NOTIFY apiResourcesChanged)

    Q_PROPERTY(QStringList outboundEvents READ outboundEvents NOTIFY outboundEventsChanged)

    Q_PROPERTY(QStringList windows READ windows NOTIFY windowsChanged)

public:
    explicit UiRegistry(QObject *parent = nullptr);

    using Title = std::function<QString()>;

    void addSettingsGroup(QString id, int rank);

    void addSettingsSection(QString id, QString group, int rank, QString owner, Title title,
                            QUrl component = {});

    void addWindow(QString id, int rank, QString owner, Title title, QUrl component,
                   QString glyph = {});

    [[nodiscard]] QStringList windows() const;

    void offerWindow(const QString &id, bool offered);

    [[nodiscard]] Q_INVOKABLE bool windowOffered(const QString &id) const;

    Q_INVOKABLE void mark(const QString &id, bool marked);

    [[nodiscard]] Q_INVOKABLE bool marked(const QString &id) const;

    [[nodiscard]] Q_INVOKABLE bool anythingMarked() const;

    [[nodiscard]] Q_INVOKABLE QUrl componentOfWindow(const QString &id) const;

    [[nodiscard]] Q_INVOKABLE QString windowTitle(const QString &id) const;

    [[nodiscard]] Q_INVOKABLE QString windowGlyph(const QString &id) const;

    void addSlotItem(QString slot, QString id, int rank, QString owner, QUrl component,
                     Title title = {}, QString glyph = {});

    [[nodiscard]] Q_INVOKABLE QString itemTitle(const QString &id) const;
    [[nodiscard]] Q_INVOKABLE QString itemGlyph(const QString &id) const;

    void addApiResource(QString id, int rank, QString owner, Title title);

    [[nodiscard]] QStringList apiResources() const;

    [[nodiscard]] Q_INVOKABLE QString apiResourceTitle(const QString &id) const;

    void addOutboundEvent(QString id, int rank, QString owner, Title title);

    [[nodiscard]] QStringList outboundEvents() const;

    [[nodiscard]] Q_INVOKABLE QString outboundEventTitle(const QString &id) const;

    [[nodiscard]] Q_INVOKABLE QStringList itemsIn(const QString &slot) const;

    [[nodiscard]] Q_INVOKABLE QUrl componentOfItem(const QString &id) const;

    struct Catalogue {
        QString owner;

        QString directory;

        QString prefix;
    };

    void addCatalogue(QString owner, QString directory, QString prefix);

    [[nodiscard]] std::vector<Catalogue> catalogues() const;

    void removeContributionsOf(const QString &owner);

    [[nodiscard]] QStringList settingsSections() const;

    [[nodiscard]] Q_INVOKABLE QString titleOf(const QString &id) const;

    [[nodiscard]] Q_INVOKABLE QString ownerOf(const QString &id) const;

    [[nodiscard]] Q_INVOKABLE QString groupOf(const QString &id) const;

    [[nodiscard]] Q_INVOKABLE QUrl componentOf(const QString &id) const;

    [[nodiscard]] int revision() const noexcept
    {
        return revision_;
    }

    [[nodiscard]] int slotRevision() const noexcept
    {
        return slotRevision_;
    }

    [[nodiscard]] int markRevision() const noexcept
    {
        return markRevision_;
    }

    [[nodiscard]] bool settled() const noexcept
    {
        return settled_;
    }

    static constexpr int kQuietMs = 250;

    void retitle();

    void stopAnnouncing() noexcept;

signals:
    void settingsSectionsChanged();

    void slotsChanged();

    void apiResourcesChanged();

    void outboundEventsChanged();

    void windowsChanged();

    void revisionChanged();

    void marksChanged();

    void settledChanged();

private:
    struct Section {
        QString id;
        int rank{0};
        QString owner;
        Title title;
        QUrl component;

        QString group;

        QString glyph;
    };

    struct Group {
        QString id;
        int rank{0};
    };

    struct Switch {
        QString id;
        int rank{0};
        QString owner;
        Title title;
    };

    using Window = Section;

    struct SlotItem {
        QString slot;
        QString id;
        int rank{0};
        QString owner;
        QUrl component;

        Title title;
        QString glyph;
    };

    [[nodiscard]] int rankOfGroup(const QString &group) const;

    void sortSections();

    void announce(void (UiRegistry::*what)());

    void stirred();

    static void put(std::vector<Switch> &into, Switch entry);
    [[nodiscard]] static QStringList namesOf(const std::vector<Switch> &switches);
    [[nodiscard]] static QString titleIn(const std::vector<Switch> &switches, const QString &id);

    std::vector<Section> sections_;
    std::vector<Group> groups_;
    std::vector<Window> windows_;
    std::vector<SlotItem> slotItems_;
    std::vector<Switch> apiResources_;
    std::vector<Switch> outboundEvents_;
    std::vector<Catalogue> catalogues_;
    int revision_{0};

    QSet<QString> withheld_;
    int slotRevision_{0};
    int markRevision_{0};

    QSet<QString> marked_;

    bool announcing_{true};

    bool settled_{false};
    QTimer quiet_;
};

}
