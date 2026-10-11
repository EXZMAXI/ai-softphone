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

#include <softphone/core/audio_signal.hpp>
#include <softphone/core/settings.hpp>

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQmlIntegration>

#include <array>
#include <functional>
#include <span>

namespace softphone::ui {

class SoundViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Built by the shell")

    Q_PROPERTY(Drawing drawing READ drawing NOTIFY drawingChanged)

    Q_PROPERTY(bool showing READ showing NOTIFY showingChanged)

    Q_PROPERTY(int height READ height NOTIFY drawingChanged)

    Q_PROPERTY(bool flanking READ flanking NOTIFY drawingChanged)

    Q_PROPERTY(bool behind READ behind NOTIFY drawingChanged)

public:

    enum class Drawing {
        Nothing,
        Levels,
        Histogram,
        Wave,
        Spectrum,
        Aurora,
        Halo,
        Uprights,
    };
    Q_ENUM(Drawing)

    using Dispatcher = std::function<void(std::function<void()>)>;

    SoundViewModel(core::ISignalTaps *taps, Dispatcher dispatch, QObject *parent = nullptr);

    [[nodiscard]] Drawing drawing() const noexcept
    {
        return drawing_;
    }

    [[nodiscard]] bool showing() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] bool flanking() const noexcept;
    [[nodiscard]] bool behind() const noexcept;

    void follow(const QString &callId);

    void chooseDrawing(core::Visualisation drawing);

    Q_INVOKABLE void watchWindow(bool onScreen);

    [[nodiscard]] double level(bool local) const noexcept;
    [[nodiscard]] double peak(bool local) const noexcept;

    [[nodiscard]] std::span<const double> trail(bool local) const noexcept;

    [[nodiscard]] std::span<const float> bands(bool local) const noexcept;

    static constexpr std::chrono::milliseconds kFrame{16};

    [[nodiscard]] double slide() const noexcept;

signals:

    void advanced();

    void drawingChanged();
    void showingChanged();

private:

    void retarget();

    void reconsider();

    core::ISignalTaps *taps_;
    Dispatcher dispatch_;

    Drawing drawing_{Drawing::Uprights};
    QString callId_;
    bool showing_{false};
    bool onScreen_{true};

    QTimer frames_;

    QElapsedTimer since_;

    QElapsedTimer sinceReading_;

    void sayWhatItReads();

    QElapsedTimer sinceSaid_;

    void layOutAboveTheFloor();

    double localFloor_{0.0};
    double remoteFloor_{0.0};
    std::array<double, core::SignalTrail::kLength> localTrail_{};
    std::array<double, core::SignalTrail::kLength> remoteTrail_{};
};

[[nodiscard]] SoundViewModel::Drawing toDrawing(core::Visualisation drawing) noexcept;
[[nodiscard]] core::Visualisation toVisualisation(SoundViewModel::Drawing drawing) noexcept;

}
