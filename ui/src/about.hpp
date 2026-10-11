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

#include <softphone/core/settings.hpp>

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <functional>

namespace softphone::ui {

class About : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString version READ version CONSTANT)

    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)

    Q_PROPERTY(QString website READ website NOTIFY websiteChanged)

    Q_PROPERTY(QString documentation READ documentation NOTIFY documentationChanged)

    Q_PROPERTY(QString documentationPage READ documentationPage NOTIFY documentationChanged)

    Q_PROPERTY(QString licence READ licence CONSTANT)

    Q_PROPERTY(QVariantList countries READ countries NOTIFY countriesChanged)

    Q_PROPERTY(QString country READ country NOTIFY countryChanged)

    Q_PROPERTY(QVariantList components READ components CONSTANT)

public:
    explicit About(QObject *parent = nullptr);

    [[nodiscard]] QString version() const;
    [[nodiscard]] QString qtVersion() const;
    [[nodiscard]] QString website() const;
    [[nodiscard]] QString documentation() const;
    [[nodiscard]] QString documentationPage() const;
    [[nodiscard]] QString licence() const;
    [[nodiscard]] QVariantList components() const;
    [[nodiscard]] QVariantList countries() const;
    [[nodiscard]] QString country() const;

    Q_INVOKABLE void chooseCountry(const QString &code);

    Q_INVOKABLE void detectCountry();

    [[nodiscard]] static QString countryOfThisMachine();

    void applyRegion(const core::RegionSettings &settings);

    [[nodiscard]] core::RegionSettings region() const;

    void setRegionCallback(std::function<void(core::RegionSettings)> told);

    Q_INVOKABLE void speak(const QString &language);

Q_SIGNALS:
    void websiteChanged();
    void documentationChanged();
    void countriesChanged();
    void countryChanged();

private:

    void addressesMoved();

    core::RegionSettings region_;
    std::function<void(core::RegionSettings)> told_;

    QString language_;

    QString installed_;
};

}
