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

#include <softphone/container/host.hpp>
#include <softphone/container/registry.hpp>
#include <softphone/container/settings.hpp>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

#include <functional>
#include <string>
#include <vector>

namespace softphone::ui {

class PluginsViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList rows READ rows NOTIFY rowsChanged)

    Q_PROPERTY(QString problem READ problem NOTIFY rowsChanged)

public:

    struct Hooks {

        std::function<std::string(const QString &)> open;
        std::function<std::string(const QString &)> contribute;
        std::function<void(const QString &)> withdraw;

        std::function<void(const QString &)> settled;
    };

    explicit PluginsViewModel(QObject *parent = nullptr);

    void observe(container::Host *host, container::Settings *settings);

    void wire(Hooks hooks);

    [[nodiscard]] QVariantList rows() const;

    [[nodiscard]] QString problem() const
    {
        return problem_;
    }

    [[nodiscard]] Q_INVOKABLE QStringList dependentsOf(const QString &id) const;

    [[nodiscard]] Q_INVOKABLE QStringList suggestersOf(const QString &id) const;

    Q_INVOKABLE void setEnabled(const QString &id, bool on);

    Q_INVOKABLE void retranslated();

Q_SIGNALS:
    void rowsChanged();

private:
    void publish();

    [[nodiscard]] QString named(const container::Manifest &manifest) const;
    [[nodiscard]] QString described(const container::Manifest &manifest) const;

    [[nodiscard]] bool turnOn(const std::string &id);

    container::Host *host_{nullptr};
    container::Settings *settings_{nullptr};

    std::vector<std::string> off_;

    Hooks hooks_;
    QString problem_;
};

}
