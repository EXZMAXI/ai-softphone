// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "signal_drawing.hpp"

#include "signal_geometry.hpp"

#include <QDateTime>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>

#include <algorithm>
#include <cmath>

namespace softphone::ui {

namespace {

QColor faded(const QColor &colour, double alpha)
{
    QColor copy = colour;
    copy.setAlphaF(static_cast<float>(std::clamp(alpha, 0.0, 1.0)));
    return copy;
}

constexpr qint64 kRippleMs = 1100;

}

SignalDrawing::SignalDrawing(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    setAntialiasing(true);

    setAcceptedMouseButtons(Qt::NoButton);
}

void SignalDrawing::setFeed(SoundViewModel *feed)
{
    if (feed_ == feed) {
        return;
    }

    if (feed_ != nullptr) {
        disconnect(feed_, nullptr, this, nullptr);
    }

    feed_ = feed;
    if (feed_ != nullptr) {
        connect(feed_, &SoundViewModel::advanced, this, [this]() { update(); });
    }

    emit feedChanged();
    update();
}

void SignalDrawing::setShape(Shape shape)
{
    if (shape_ == shape) {
        return;
    }
    shape_ = shape;
    ripples_.clear();
    emit shapeChanged();
    update();
}

void SignalDrawing::setSide(Side side)
{
    if (side_ == side) {
        return;
    }
    side_ = side;
    emit sideChanged();
    update();
}

void SignalDrawing::setLocalColour(const QColor &colour)
{
    if (localColour_ == colour) {
        return;
    }
    localColour_ = colour;
    emit coloursChanged();
    update();
}

void SignalDrawing::setRemoteColour(const QColor &colour)
{
    if (remoteColour_ == colour) {
        return;
    }
    remoteColour_ = colour;
    emit coloursChanged();
    update();
}

void SignalDrawing::setFaint(const QColor &colour)
{
    if (faint_ == colour) {
        return;
    }
    faint_ = colour;
    emit coloursChanged();
    update();
}

void SignalDrawing::setInitials(const QString &initials)
{
    if (initials_ == initials) {
        return;
    }
    initials_ = initials;
    emit initialsChanged();
    update();
}

QColor SignalDrawing::colourFor(bool local) const
{
    return local ? localColour_ : remoteColour_;
}

void SignalDrawing::paint(QPainter *painter)
{
    if (feed_ == nullptr || width() <= 0 || height() <= 0) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, true);

    switch (shape_) {
    case Shape::Bar:
        paintBar(painter);
        return;
    case Shape::Upright:
        paintUpright(painter);
        return;
    case Shape::Histogram:
        paintHistogram(painter);
        return;
    case Shape::Wave:
        paintWave(painter);
        return;
    case Shape::Spectrum:
        paintSpectrum(painter);
        return;
    case Shape::Aurora:
        paintAurora(painter);
        return;
    case Shape::Halo:
        paintHalo(painter);
        return;
    }
}

void SignalDrawing::paintBar(QPainter *painter)
{
    const bool local = side_ == Side::Local;
    const QColor colour = colourFor(local);
    const double radius = height() / 2.0;

    painter->setPen(Qt::NoPen);
    painter->setBrush(faint_);
    painter->drawRoundedRect(QRectF(0, 0, width(), height()), radius, radius);

    const double filled = std::max(height(), width() * shaped(feed_->level(local)));
    QLinearGradient shade(0, 0, filled, 0);
    shade.setColorAt(0.0, colour.darker(125));
    shade.setColorAt(1.0, colour.lighter(115));
    painter->setBrush(shade);
    painter->drawRoundedRect(QRectF(0, 0, filled, height()), radius, radius);

    const double peak = shaped(feed_->peak(local));
    if (peak > 0.02) {
        const double x = std::clamp(width() * peak - 2.0, 0.0, width() - 2.0);
        painter->setBrush(faded(colour.lighter(150), 0.85));
        painter->drawRoundedRect(QRectF(x, 0, 2.0, height()), 1.0, 1.0);
    }
}

void SignalDrawing::paintUpright(QPainter *painter)
{
    const bool local = side_ == Side::Local;
    const QColor colour = colourFor(local);
    const double radius = width() / 2.0;

    painter->setPen(Qt::NoPen);
    painter->setBrush(faint_);
    painter->drawRoundedRect(QRectF(0, 0, width(), height()), radius, radius);

    const double filled = std::max(width(), height() * shaped(feed_->level(local)));
    QLinearGradient shade(0, height(), 0, height() - filled);
    shade.setColorAt(0.0, colour.darker(120));
    shade.setColorAt(1.0, colour.lighter(120));
    painter->setBrush(shade);
    painter->drawRoundedRect(QRectF(0, height() - filled, width(), filled), radius, radius);

    const double peak = shaped(feed_->peak(local));
    if (peak > 0.02) {
        const double y = std::clamp(height() * (1.0 - peak), 0.0, height() - 2.0);
        painter->setBrush(faded(colour.lighter(150), 0.85));
        painter->drawRoundedRect(QRectF(0, y, width(), 2.0), 1.0, 1.0);
    }
}

void SignalDrawing::paintHistogram(QPainter *painter)
{
    const auto local = feed_->trail(true);
    const auto remote = feed_->trail(false);
    if (local.empty() && remote.empty()) {
        return;
    }

    const double middle = height() / 2.0;
    const double room = middle - 3.0;
    const std::size_t columns = std::max<std::size_t>(local.size(), remote.size());
    const Strip strip = stripFor(width(), columns, feed_->slide());
    if (strip.step <= 0.0) {
        return;
    }

    painter->setPen(Qt::NoPen);

    auto draw = [&](std::span<const double> trail, const QColor &colour, bool up) {
        for (std::size_t index = 0; index < trail.size(); ++index) {
            const double x = columnX(strip, width(), index, trail.size());
            if (x < -strip.step) {
                continue;
            }
            const double length = std::max(1.5, shaped(trail[index]) * room);
            painter->setBrush(faded(colour, columnAlpha(x, width())));
            const QRectF bar(x - strip.barWidth, up ? middle - length : middle, strip.barWidth,
                             length);
            const double round = std::min(strip.barWidth / 2.0, length / 2.0);
            painter->drawRoundedRect(bar, round, round);
        }
    };

    draw(local, localColour_, true);
    draw(remote, remoteColour_, false);

    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(faint_, 1));
    painter->drawLine(QPointF(0, middle), QPointF(width(), middle));
}

void SignalDrawing::paintWave(QPainter *painter)
{
    const auto local = feed_->trail(true);
    const auto remote = feed_->trail(false);
    if (local.size() < 3 && remote.size() < 3) {
        return;
    }

    const double middle = height() / 2.0;
    const double room = middle - 2.0;
    const std::size_t columns = std::max<std::size_t>(local.size(), remote.size());
    const Strip strip = stripFor(width(), columns, feed_->slide());
    if (strip.step <= 0.0) {
        return;
    }

    auto envelope = [&](std::span<const double> trail, const QColor &colour, bool up) {
        if (trail.size() < 3) {
            return;
        }

        QPainterPath path;
        path.moveTo(columnX(strip, width(), 0, trail.size()), middle);

        for (std::size_t index = 0; index + 1 < trail.size(); ++index) {
            const double x = columnX(strip, width(), index, trail.size());
            const double y = middle + (up ? -1.0 : 1.0) * (1.5 + shaped(trail[index]) * room);
            const double nextX = x + strip.step;
            const double nextY =
                middle + (up ? -1.0 : 1.0) * (1.5 + shaped(trail[index + 1]) * room);
            path.quadTo(QPointF(x, y), QPointF((x + nextX) / 2.0, (y + nextY) / 2.0));
        }

        path.lineTo(width(), middle);
        path.closeSubpath();

        QLinearGradient shade(0, up ? 0 : middle, 0, up ? middle : height());
        shade.setColorAt(up ? 0.0 : 1.0, faded(colour, 0.85));
        shade.setColorAt(up ? 1.0 : 0.0, faded(colour, 0.25));
        painter->setPen(QPen(faded(colour.lighter(130), 0.9), 1.4));
        painter->setBrush(shade);
        painter->drawPath(path);
    };

    envelope(local, localColour_, true);
    envelope(remote, remoteColour_, false);

    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(faint_, 1));
    painter->drawLine(QPointF(0, middle), QPointF(width(), middle));
}

void SignalDrawing::paintSpectrum(QPainter *painter)
{
    const auto local = feed_->bands(true);
    const auto remote = feed_->bands(false);
    if (local.empty() && remote.empty()) {
        return;
    }

    const std::size_t count = std::max<std::size_t>(local.size(), remote.size());
    const double step = width() / static_cast<double>(count);
    const double barWidth = step * 0.68;
    const double middle = height() / 2.0;
    const double room = middle - 3.0;

    painter->setPen(Qt::NoPen);

    auto draw = [&](std::span<const float> bands, const QColor &colour, bool up) {
        for (std::size_t index = 0; index < bands.size(); ++index) {
            const double length =
                std::max(1.5, std::pow(static_cast<double>(bands[index]), 0.75) * room);
            const double x = static_cast<double>(index) * step + (step - barWidth) / 2.0;

            QLinearGradient shade(0, middle, 0, up ? 0 : height());
            shade.setColorAt(0.0, faded(colour, 0.45));
            shade.setColorAt(1.0, faded(colour.lighter(135), 0.95));
            painter->setBrush(shade);

            const QRectF bar(x, up ? middle - length : middle, barWidth, length);
            const double round = std::min(barWidth / 2.0, length / 2.0);
            painter->drawRoundedRect(bar, round, round);
        }
    };

    draw(local, localColour_, true);
    draw(remote, remoteColour_, false);
}

void SignalDrawing::paintAurora(QPainter *painter)
{

    const double localLevel = feed_->level(true);
    const double remoteLevel = feed_->level(false);
    localGlow_ += (localLevel - localGlow_) * (localLevel > localGlow_ ? 0.3 : 0.05);
    remoteGlow_ += (remoteLevel - remoteGlow_) * (remoteLevel > remoteGlow_ ? 0.3 : 0.05);
    drift_ += 0.005;

    auto glow = [&](double centreX, double centreY, double radius, const QColor &colour,
                    double strength) {
        if (radius <= 0.0) {
            return;
        }
        QRadialGradient shade(QPointF(centreX, centreY), radius);
        shade.setColorAt(0.0, faded(colour.lighter(120), 0.5 * strength));
        shade.setColorAt(0.45, faded(colour, 0.24 * strength));
        shade.setColorAt(1.0, faded(colour, 0.0));
        painter->setPen(Qt::NoPen);
        painter->setBrush(shade);
        painter->drawRect(QRectF(0, 0, width(), height()));
    };

    const double line = height() * 0.52;
    glow(width() * (0.26 + 0.05 * std::sin(drift_)), line + height() * 0.1 * std::cos(drift_ * 0.8),
         width() * (0.32 + 0.45 * localGlow_), localColour_, 0.35 + localGlow_);
    glow(width() * (0.74 + 0.05 * std::cos(drift_ * 1.1)),
         line - height() * 0.1 * std::sin(drift_ * 0.7), width() * (0.32 + 0.45 * remoteGlow_),
         remoteColour_, 0.35 + remoteGlow_);
}

void SignalDrawing::paintHalo(QPainter *painter)
{
    const double localLevel = feed_->level(true);
    const double remoteLevel = feed_->level(false);
    localGlow_ += (localLevel - localGlow_) * (localLevel > localGlow_ ? 0.4 : 0.07);
    remoteGlow_ += (remoteLevel - remoteGlow_) * (remoteLevel > remoteGlow_ ? 0.4 : 0.07);

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const double loudest = std::max(localGlow_, remoteGlow_);
    if (loudest > 0.45 && !armed_) {
        armed_ = true;
        ripples_.push_back(Ripple{now, localGlow_ >= remoteGlow_});
    } else if (loudest < 0.3) {
        armed_ = false;
    }
    std::erase_if(ripples_, [now](const Ripple &one) { return now - one.bornMs > kRippleMs; });

    const QPointF centre(width() / 2.0, height() / 2.0);
    const double base = std::min(width(), height()) * 0.3;

    painter->setBrush(Qt::NoBrush);
    for (const Ripple &one : ripples_) {
        const double age = static_cast<double>(now - one.bornMs) / kRippleMs;
        const QColor colour = colourFor(one.local);
        painter->setPen(QPen(faded(colour, 0.35 * (1.0 - age)), 2.0 * (1.0 - age)));
        const double radius = base * (1.35 + age * 1.6);
        painter->drawEllipse(centre, radius, radius);
    }

    auto ring = [&](double radius, double thickness, const QColor &colour, double value) {
        painter->setPen(QPen(faint_, thickness, Qt::SolidLine, Qt::RoundCap));
        painter->drawEllipse(centre, radius, radius);

        const double sweep = 180.0 * (0.06 + 0.94 * std::pow(std::clamp(value, 0.0, 1.0), 0.8));
        painter->setPen(
            QPen(faded(colour.lighter(115), 0.95), thickness, Qt::SolidLine, Qt::RoundCap));
        const QRectF box(centre.x() - radius, centre.y() - radius, radius * 2.0, radius * 2.0);

        painter->drawArc(box, static_cast<int>((90.0 - sweep) * 16),
                         static_cast<int>(sweep * 2.0 * 16));
    };

    ring(base * 1.24, 7.0, localColour_, localGlow_);
    ring(base * 0.88, 7.0, remoteColour_, remoteGlow_);

    painter->setPen(Qt::NoPen);
    painter->setBrush(faded(faint_, 0.5));
    painter->drawEllipse(centre, base * 0.62, base * 0.62);

    if (!initials_.isEmpty()) {
        QFont letters = painter->font();
        letters.setPixelSize(static_cast<int>(base * 0.62));
        letters.setWeight(QFont::Medium);
        painter->setFont(letters);
        painter->setPen(QPen(faded(faint_, 1.0)));
        painter->drawText(QRectF(centre.x() - base, centre.y() - base, base * 2.0, base * 2.0),
                          Qt::AlignCenter, initials_);
    }
}

}
