// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "plugins_view_model.hpp"

#include <QVariantMap>

#include <utility>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::ui {
namespace {

[[nodiscard]] QString stateOf(container::LoadState state)
{
    switch (state) {
    case container::LoadState::Ready:
        return QStringLiteral("running");
    case container::LoadState::Disabled:
        return QStringLiteral("off");
    case container::LoadState::Blocked:
        return QStringLiteral("blocked");
    case container::LoadState::Cycle:
        return QStringLiteral("cycle");
    case container::LoadState::Failed:
        return QStringLiteral("failed");
    case container::LoadState::Waiting:
        break;
    }
    return QStringLiteral("waiting");
}

[[nodiscard]] QString asText(std::string_view text)
{
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

[[nodiscard]] std::string_view shownName(const container::Manifest &manifest)
{
    return manifest.name.empty() ? std::string_view{manifest.id} : std::string_view{manifest.name};
}

struct Module {
    const char *id;
    const char *name;
    const char *summary;
};

constexpr std::array kModules{
    Module{
        "application", QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Application"),
        QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Main window, settings, icons etc.")},
    Module{"blf", QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Buttons"),
           QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel",
                             "The buttons beside the dialler, and the lamps on them")},
    Module{"capture", QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Capture"),
           QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel",
                             "Recording a conversation happening in another application")},
    Module{"diagnostics", QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Diagnostics"),
           QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel",
                             "The SIP trace, the media statistics and the counters")},
    Module{"directory", QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Directory"),
           QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "The address book")},
    Module{"integration", QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Integration"),
           QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "The local API, and webhooks")},
    Module{"mediastorage", QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Mediastorage"),
           QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel",
                             "The library of conversations and how long they are kept")},
    Module{"processing", QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Processing"),
           QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel",
                             "Prompts and the rules that fire them")},
    Module{"realtime", QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Prompter"),
           QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel",
                             "Listening to a conversation in progress, and suggesting")},
    Module{"recording", QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Recording"),
           QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel",
                             "Recording calls, and telling the other party about it")},
    Module{"softphone", QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Telephone"),
           QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel",
                             "SIP, accounts, calls, the dialler and the call history")},
    Module{"storage", QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Local database"),
           QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel",
                             "The database used as a local storage")},
    Module{
        "taxonomy", QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Dictionaries"),
        QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel",
                          "Categories, tags and red flags — the codes everything else points at")},
    Module{"transcription", QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Transcription"),
           QT_TRANSLATE_NOOP(
               "softphone::ui::PluginsViewModel",
               "Recognisers, the queue that drives them, and the transcripts they produce")},
    Module{
        "webui", QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Browser interface"),
        QT_TRANSLATE_NOOP("softphone::ui::PluginsViewModel", "Some features in a local browser")},
};

[[nodiscard]] const Module *moduleFor(std::string_view id)
{
    const auto found = std::ranges::find_if(
        kModules, [id](const Module &one) { return std::string_view{one.id} == id; });
    return found == kModules.end() ? nullptr : &*found;
}

}

PluginsViewModel::PluginsViewModel(QObject *parent) : QObject{parent} {}

void PluginsViewModel::observe(container::Host *host, container::Settings *settings)
{
    host_ = host;
    settings_ = settings;
    off_ = settings_ != nullptr ? settings_->disabledPlugins() : std::vector<std::string>{};
    publish();
}

void PluginsViewModel::publish()
{
    Q_EMIT rowsChanged();
}

QString PluginsViewModel::named(const container::Manifest &manifest) const
{
    if (const auto *const known = moduleFor(manifest.id); known != nullptr) {
        return tr(known->name);
    }
    return asText(shownName(manifest));
}

QString PluginsViewModel::described(const container::Manifest &manifest) const
{
    if (const auto *const known = moduleFor(manifest.id); known != nullptr) {
        return tr(known->summary);
    }
    return asText(manifest.summary);
}

void PluginsViewModel::retranslated()
{

    Q_EMIT rowsChanged();
}

QVariantList PluginsViewModel::rows() const
{
    QVariantList out;
    if (host_ == nullptr) {
        return out;
    }

    std::vector<const container::Entry *> ordered;
    ordered.reserve(host_->plugins().entries().size());
    for (const auto &entry : host_->plugins().entries()) {
        ordered.push_back(&entry);
    }
    std::ranges::stable_sort(
        ordered, [this](const container::Entry *left, const container::Entry *right) {
            if (left->manifest.required != right->manifest.required) {
                return left->manifest.required;
            }

            return named(left->manifest).localeAwareCompare(named(right->manifest)) < 0;
        });

    for (const auto *const one : ordered) {
        const auto &entry = *one;
        QVariantMap row;
        row[QStringLiteral("id")] = asText(entry.manifest.id);
        row[QStringLiteral("name")] = named(entry.manifest);
        row[QStringLiteral("summary")] = described(entry.manifest);
        row[QStringLiteral("version")] = asText(toString(entry.manifest.version));
        row[QStringLiteral("state")] = stateOf(entry.state);
        row[QStringLiteral("problem")] = asText(entry.problem);
        row[QStringLiteral("required")] = entry.manifest.required;

        row[QStringLiteral("enabled")] = std::ranges::find(off_, entry.manifest.id) == off_.end();

        row[QStringLiteral("missing")] =
            entry.plugin == nullptr && row[QStringLiteral("enabled")].toBool();

        out.append(row);
    }
    return out;
}

QStringList PluginsViewModel::dependentsOf(const QString &id) const
{
    QStringList out;
    if (host_ == nullptr) {
        return out;
    }

    for (const auto &dependent : host_->plugins().dependentsOf(id.toStdString())) {
        const auto *const entry = host_->plugins().find(dependent);

        if (entry == nullptr || entry->state != container::LoadState::Ready) {
            continue;
        }

        out.append(named(entry->manifest));
    }
    return out;
}

QStringList PluginsViewModel::suggestersOf(const QString &id) const
{
    QStringList out;
    if (host_ == nullptr) {
        return out;
    }

    for (const auto &one : host_->plugins().suggestersOf(id.toStdString())) {
        const auto *const entry = host_->plugins().find(one);
        if (entry == nullptr) {
            continue;
        }
        out.append(named(entry->manifest));
    }
    return out;
}

void PluginsViewModel::wire(Hooks hooks)
{
    hooks_ = std::move(hooks);
}

void PluginsViewModel::setEnabled(const QString &id, bool on)
{
    if (host_ == nullptr) {
        return;
    }

    const auto wanted = id.toStdString();
    const auto *const entry = host_->plugins().find(wanted);
    if (entry == nullptr) {
        return;
    }

    problem_.clear();

    if (on) {

        auto order = host_->plugins().requirementsOf(wanted);
        order.push_back(wanted);

        for (const auto &one : order) {
            if (!turnOn(one)) {
                break;
            }
        }
    } else {

        if (entry->manifest.required) {
            return;
        }

        const auto change = host_->deactivate(wanted);
        if (!change.ok()) {
            problem_ = QString::fromStdString(change.problem);
        }

        for (const auto &gone : change.changed) {
            if (hooks_.withdraw) {
                hooks_.withdraw(QString::fromStdString(gone));
            }
            if (std::ranges::find(off_, gone) == off_.end()) {
                off_.push_back(gone);
            }
        }
    }

    if (settings_ != nullptr) {
        settings_->setDisabledPlugins(off_);
        static_cast<void>(settings_->save());
    }
    publish();
}

bool PluginsViewModel::turnOn(const std::string &id)
{
    const auto *const entry = host_->plugins().find(id);
    if (entry == nullptr || entry->state == container::LoadState::Ready) {
        std::erase(off_, id);
        return true;
    }

    const auto asText = QString::fromStdString(id);

    if (entry->plugin == nullptr && hooks_.open) {
        if (auto trouble = hooks_.open(asText); !trouble.empty()) {
            problem_ = QString::fromStdString(trouble);
            return false;
        }
    }

    const auto change = host_->activate(id);
    if (!change.ok()) {
        problem_ = QString::fromStdString(change.problem);
        return false;
    }

    if (hooks_.contribute) {
        if (auto trouble = hooks_.contribute(asText); !trouble.empty()) {

            problem_ = QString::fromStdString(trouble);
        }
    }

    if (hooks_.settled) {
        hooks_.settled(asText);
    }

    std::erase(off_, id);
    return true;
}

}
