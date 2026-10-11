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

#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <random>
#include <string>

namespace softphone::ui {

class DemoCapture final : public QObject, public core::ISignalTaps {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Created by the application and exposed as `demoCapture`.")

public:
    explicit DemoCapture(QObject *parent = nullptr);

    Q_INVOKABLE void capturing(const QString &what, int secondsIn);

    Q_INVOKABLE void quiet();

    [[nodiscard]] Q_INVOKABLE bool bothVoicesInTheTrail(int seconds) const;

    [[nodiscard]] bool running() const noexcept
    {
        return running_;
    }

    [[nodiscard]] QString what() const
    {
        return what_;
    }

    [[nodiscard]] std::chrono::milliseconds elapsed() const noexcept;

    void watchSignals(const std::string &name) override;

    [[nodiscard]] core::CallSignals &feed() noexcept override;

Q_SIGNALS:

    void changed();

private:

    void speak();

    static constexpr std::chrono::milliseconds kTick{20};

    static constexpr unsigned kRate = 16000;

    static constexpr std::size_t kSamplesATick = kRate / 1000 * 20;

    QTimer clock_;
    core::CallSignals signals_{kRate};
    QString what_;
    bool running_{false};
    bool watching_{false};

    std::chrono::milliseconds began_{0};
    std::chrono::milliseconds spoken_{0};

    bool nearSide_{true};
    bool silent_{true};
    std::chrono::milliseconds turnEnds_{0};

    std::chrono::milliseconds nearSpokeAt_{-1};
    std::chrono::milliseconds farSpokeAt_{-1};

    double syllable_{0.0};
    double phase_{0.0};

    std::chrono::milliseconds word_{0};
    bool between_{false};
    double loud_{1.0};

    std::mt19937 noise_{20260909};

    std::array<std::int16_t, kSamplesATick> block_{};
};

}
