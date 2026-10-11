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

#include "sound_view_model.hpp"

#include <QColor>
#include <QQuickPaintedItem>
#include <QString>
#include <QtQmlIntegration>

#include <vector>

namespace softphone::ui {

class SignalDrawing : public QQuickPaintedItem {
    Q_OBJECT

    Q_PROPERTY(softphone::ui::SoundViewModel *feed READ feed WRITE setFeed NOTIFY feedChanged)
    Q_PROPERTY(Shape shape READ shape WRITE setShape NOTIFY shapeChanged)
    Q_PROPERTY(Side side READ side WRITE setSide NOTIFY sideChanged)
    Q_PROPERTY(QColor localColour READ localColour WRITE setLocalColour NOTIFY coloursChanged)
    Q_PROPERTY(QColor remoteColour READ remoteColour WRITE setRemoteColour NOTIFY coloursChanged)

    Q_PROPERTY(QColor faint READ faint WRITE setFaint NOTIFY coloursChanged)

    Q_PROPERTY(QString initials READ initials WRITE setInitials NOTIFY initialsChanged)

public:
    enum class Shape {

        Bar,

        Upright,

        Histogram,

        Wave,

        Spectrum,

        Aurora,

        Halo,
    };
    Q_ENUM(Shape)

    enum class Side {
        Local,
        Remote,
    };
    Q_ENUM(Side)

    explicit SignalDrawing(QQuickItem *parent = nullptr);

    [[nodiscard]] SoundViewModel *feed() const noexcept
    {
        return feed_;
    }

    void setFeed(SoundViewModel *feed);

    [[nodiscard]] Shape shape() const noexcept
    {
        return shape_;
    }

    void setShape(Shape shape);

    [[nodiscard]] Side side() const noexcept
    {
        return side_;
    }

    void setSide(Side side);

    [[nodiscard]] QColor localColour() const noexcept
    {
        return localColour_;
    }

    void setLocalColour(const QColor &colour);

    [[nodiscard]] QColor remoteColour() const noexcept
    {
        return remoteColour_;
    }

    void setRemoteColour(const QColor &colour);

    [[nodiscard]] QColor faint() const noexcept
    {
        return faint_;
    }

    void setFaint(const QColor &colour);

    [[nodiscard]] QString initials() const
    {
        return initials_;
    }

    void setInitials(const QString &initials);

    void paint(QPainter *painter) override;

signals:
    void feedChanged();
    void shapeChanged();
    void sideChanged();
    void coloursChanged();
    void initialsChanged();

private:
    void paintBar(QPainter *painter);
    void paintUpright(QPainter *painter);
    void paintHistogram(QPainter *painter);
    void paintWave(QPainter *painter);
    void paintSpectrum(QPainter *painter);
    void paintAurora(QPainter *painter);
    void paintHalo(QPainter *painter);

    [[nodiscard]] QColor colourFor(bool local) const;

    SoundViewModel *feed_{nullptr};
    Shape shape_{Shape::Bar};
    Side side_{Side::Local};
    QColor localColour_{"#26A69A"};
    QColor remoteColour_{"#5C6BC0"};
    QColor faint_{QColor(128, 128, 128, 60)};
    QString initials_;

    struct Ripple {
        qint64 bornMs{0};
        bool local{true};
    };

    std::vector<Ripple> ripples_;

    bool armed_{false};

    double localGlow_{0.0};
    double remoteGlow_{0.0};

    double drift_{0.0};
};

}
