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

#include "window_shape.hpp"

#include <softphone/core/settings.hpp>

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace softphone::ui {

class PhoneShape : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `shape`.")

    Q_PROPERTY(int column READ column CONSTANT)

    Q_PROPERTY(bool keyLetters READ keyLetters CONSTANT)

    Q_PROPERTY(int air READ air CONSTANT)

    Q_PROPERTY(int edge READ edge CONSTANT)

    Q_PROPERTY(int rowGap READ rowGap CONSTANT)
    Q_PROPERTY(int field READ field CONSTANT)

    Q_PROPERTY(int keyStrip READ keyStrip CONSTANT)

    Q_PROPERTY(int keyColumns READ keyColumns CONSTANT)

    Q_PROPERTY(bool oneChipRow READ oneChipRow CONSTANT)
    Q_PROPERTY(bool chipsCentred READ chipsCentred CONSTANT)

    Q_PROPERTY(int dialKey READ dialKey CONSTANT)
    Q_PROPERTY(int dialDigit READ dialDigit CONSTANT)

    Q_PROPERTY(int cardName READ cardName CONSTANT)
    Q_PROPERTY(int cardPad READ cardPad CONSTANT)
    Q_PROPERTY(int keyGlyph READ keyGlyph CONSTANT)
    Q_PROPERTY(int cardButton READ cardButton CONSTANT)
    Q_PROPERTY(int cardButtonLarge READ cardButtonLarge CONSTANT)
    Q_PROPERTY(int cardGlyph READ cardGlyph CONSTANT)
    Q_PROPERTY(int cardGlyphLarge READ cardGlyphLarge CONSTANT)

    Q_PROPERTY(int leastHeight READ leastHeight CONSTANT)

    Q_PROPERTY(bool inAWorkspace READ inAWorkspace CONSTANT)

    Q_PROPERTY(bool toolbarIsAMenu READ toolbarIsAMenu CONSTANT)

    Q_PROPERTY(bool phoneInTheWorkspace READ phoneInTheWorkspace CONSTANT)

    Q_PROPERTY(bool hasAColumn READ hasAColumn CONSTANT)

    Q_PROPERTY(QString name READ name CONSTANT)

    Q_PROPERTY(QString peopleWindow READ peopleWindow CONSTANT)

public:
    explicit PhoneShape(core::Layout layout, QObject *parent = nullptr);

    [[nodiscard]] int column() const noexcept
    {
        return metrics_.column;
    }

    [[nodiscard]] int dialKey() const noexcept
    {
        return metrics_.dialKey;
    }

    [[nodiscard]] bool keyLetters() const noexcept
    {
        return metrics_.keyLetters;
    }

    [[nodiscard]] int air() const noexcept
    {
        return metrics_.air;
    }

    [[nodiscard]] int edge() const noexcept
    {
        return metrics_.edge;
    }

    [[nodiscard]] int rowGap() const noexcept
    {
        return metrics_.rowGap;
    }

    [[nodiscard]] int field() const noexcept
    {
        return metrics_.field;
    }

    [[nodiscard]] int keyStrip() const noexcept
    {
        return metrics_.keyStrip;
    }

    [[nodiscard]] int keyColumns() const noexcept
    {
        return metrics_.keyColumns;
    }

    [[nodiscard]] bool oneChipRow() const noexcept
    {
        return metrics_.oneChipRow;
    }

    [[nodiscard]] bool chipsCentred() const noexcept
    {
        return metrics_.chipsCentred;
    }

    [[nodiscard]] int dialDigit() const noexcept
    {
        return metrics_.dialDigit;
    }

    [[nodiscard]] int cardName() const noexcept
    {
        return metrics_.cardName;
    }

    [[nodiscard]] int cardPad() const noexcept
    {
        return metrics_.cardPad;
    }

    [[nodiscard]] int keyGlyph() const noexcept
    {
        return metrics_.keyGlyph;
    }

    [[nodiscard]] int cardButton() const noexcept
    {
        return metrics_.cardButton;
    }

    [[nodiscard]] int cardButtonLarge() const noexcept
    {
        return metrics_.cardButtonLarge;
    }

    [[nodiscard]] int cardGlyph() const noexcept
    {
        return metrics_.cardGlyph;
    }

    [[nodiscard]] int cardGlyphLarge() const noexcept
    {
        return metrics_.cardGlyphLarge;
    }

    [[nodiscard]] int leastHeight() const noexcept
    {
        return metrics_.leastHeight;
    }

    [[nodiscard]] bool toolbarIsAMenu() const noexcept
    {
        return toolbarIsAMenu_;
    }

    [[nodiscard]] bool hasAColumn() const noexcept
    {
        return hasAColumn_;
    }

    [[nodiscard]] bool phoneInTheWorkspace() const noexcept
    {
        return phoneInTheWorkspace_;
    }

    [[nodiscard]] bool inAWorkspace() const noexcept
    {
        return inAWorkspace_;
    }

    [[nodiscard]] Q_INVOKABLE QString zoneOfWindow(const QString &windowId) const;

    [[nodiscard]] Q_INVOKABLE QVariantMap placeInRows(const QVariantList &widths, double available,
                                                      double spacing, double rowHeight,
                                                      double rowSpacing) const;

    [[nodiscard]] QString name() const;

    [[nodiscard]] static QString peopleWindow();

private:
    core::Layout layout_;
    PhoneMetrics metrics_;
    bool toolbarIsAMenu_{false};
    bool hasAColumn_{true};
    bool inAWorkspace_{false};
    bool phoneInTheWorkspace_{false};
};

}
