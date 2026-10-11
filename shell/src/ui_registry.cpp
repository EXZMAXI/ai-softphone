// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/shell/ui_registry.hpp>

#include <algorithm>
#include <limits>
#include <utility>

namespace softphone::shell {

UiRegistry::UiRegistry(QObject *parent) : QObject{parent}
{

    quiet_.setSingleShot(true);
    quiet_.setInterval(kQuietMs);
    QObject::connect(&quiet_, &QTimer::timeout, this, [this] {
        if (settled_) {
            return;
        }
        settled_ = true;
        announce(&UiRegistry::settledChanged);
    });
    quiet_.start();
}

void UiRegistry::stirred()
{

    quiet_.start();
    if (!settled_) {
        return;
    }
    settled_ = false;
    announce(&UiRegistry::settledChanged);
}

void UiRegistry::addSettingsGroup(QString id, int rank)
{
    if (id.isEmpty()) {
        return;
    }

    auto entry = Group{std::move(id), rank};
    const auto existing = std::ranges::find_if(
        groups_, [&entry](const Group &group) { return group.id == entry.id; });
    if (existing != groups_.end()) {
        *existing = std::move(entry);
    } else {
        groups_.push_back(std::move(entry));
    }

    sortSections();
    announce(&UiRegistry::settingsSectionsChanged);
}

int UiRegistry::rankOfGroup(const QString &group) const
{
    const auto found =
        std::ranges::find_if(groups_, [&group](const Group &known) { return known.id == group; });

    return found == groups_.end() ? std::numeric_limits<int>::max() : found->rank;
}

void UiRegistry::sortSections()
{
    std::ranges::stable_sort(sections_, [this](const Section &a, const Section &b) {
        const auto bandA = rankOfGroup(a.group);
        const auto bandB = rankOfGroup(b.group);
        return bandA != bandB ? bandA < bandB : a.rank < b.rank;
    });
}

void UiRegistry::addSettingsSection(QString id, QString group, int rank, QString owner, Title title,
                                    QUrl component)
{
    if (id.isEmpty() || !title) {
        return;
    }

    auto entry =
        Section{std::move(id),    rank, std::move(owner), std::move(title), std::move(component),
                std::move(group), {}};
    const auto existing = std::ranges::find_if(
        sections_, [&entry](const Section &section) { return section.id == entry.id; });
    if (existing != sections_.end()) {
        *existing = std::move(entry);
    } else {
        sections_.push_back(std::move(entry));
    }

    sortSections();
    announce(&UiRegistry::settingsSectionsChanged);
}

void UiRegistry::put(std::vector<Switch> &into, Switch entry)
{
    const auto existing =
        std::ranges::find_if(into, [&entry](const Switch &known) { return known.id == entry.id; });
    if (existing != into.end()) {
        *existing = std::move(entry);
    } else {
        into.push_back(std::move(entry));
    }

    std::ranges::stable_sort(into,
                             [](const Switch &a, const Switch &b) { return a.rank < b.rank; });
}

QStringList UiRegistry::namesOf(const std::vector<Switch> &switches)
{
    QStringList ids;
    ids.reserve(static_cast<qsizetype>(switches.size()));
    for (const auto &one : switches) {
        ids.push_back(one.id);
    }
    return ids;
}

QString UiRegistry::titleIn(const std::vector<Switch> &switches, const QString &id)
{
    const auto found =
        std::ranges::find_if(switches, [&id](const Switch &one) { return one.id == id; });
    return found == switches.end() ? QString{} : found->title();
}

void UiRegistry::addWindow(QString id, int rank, QString owner, Title title, QUrl component,
                           QString glyph)
{
    if (id.isEmpty() || !title) {
        return;
    }

    auto entry = Window{std::move(id),        rank, std::move(owner), std::move(title),
                        std::move(component), {},   std::move(glyph)};

    const auto existing = std::ranges::find_if(windows_, [&entry](const Window &window) {
        return window.id == entry.id && window.owner == entry.owner;
    });
    if (existing != windows_.end()) {
        *existing = std::move(entry);
    } else {
        windows_.push_back(std::move(entry));
    }

    std::ranges::stable_sort(windows_,
                             [](const Window &a, const Window &b) { return a.rank < b.rank; });

    announce(&UiRegistry::windowsChanged);
}

QStringList UiRegistry::windows() const
{
    QStringList ids;
    ids.reserve(static_cast<qsizetype>(windows_.size()));
    for (const auto &window : windows_) {

        if (ids.contains(window.id)) {
            continue;
        }

        if (withheld_.contains(window.id)) {
            continue;
        }
        ids.push_back(window.id);
    }
    return ids;
}

void UiRegistry::offerWindow(const QString &id, bool offered)
{
    if (id.isEmpty()) {
        return;
    }
    const bool was = !withheld_.contains(id);
    if (was == offered) {
        return;
    }
    if (offered) {
        withheld_.remove(id);
    } else {
        withheld_.insert(id);
    }
    announce(&UiRegistry::windowsChanged);
}

bool UiRegistry::windowOffered(const QString &id) const
{
    return !withheld_.contains(id);
}

void UiRegistry::mark(const QString &id, bool marked)
{
    if (id.isEmpty() || marked_.contains(id) == marked) {
        return;
    }
    if (marked) {
        marked_.insert(id);
    } else {
        marked_.remove(id);
    }
    ++markRevision_;
    announce(&UiRegistry::marksChanged);
}

bool UiRegistry::marked(const QString &id) const
{
    return marked_.contains(id);
}

bool UiRegistry::anythingMarked() const
{
    return !marked_.isEmpty();
}

QUrl UiRegistry::componentOfWindow(const QString &id) const
{

    for (const auto &window : windows_) {
        if (window.id == id && !window.component.isEmpty()) {
            return window.component;
        }
    }
    return {};
}

QString UiRegistry::windowTitle(const QString &id) const
{
    const auto found =
        std::ranges::find_if(windows_, [&id](const Window &window) { return window.id == id; });
    return found == windows_.end() ? QString{} : found->title();
}

QString UiRegistry::windowGlyph(const QString &id) const
{

    for (const auto &window : windows_) {
        if (window.id == id && !window.glyph.isEmpty()) {
            return window.glyph;
        }
    }
    return {};
}

void UiRegistry::addApiResource(QString id, int rank, QString owner, Title title)
{
    if (id.isEmpty() || !title) {
        return;
    }

    put(apiResources_, Switch{std::move(id), rank, std::move(owner), std::move(title)});
    announce(&UiRegistry::apiResourcesChanged);
}

QStringList UiRegistry::apiResources() const
{
    return namesOf(apiResources_);
}

QString UiRegistry::apiResourceTitle(const QString &id) const
{
    return titleIn(apiResources_, id);
}

void UiRegistry::addOutboundEvent(QString id, int rank, QString owner, Title title)
{
    if (id.isEmpty() || !title) {
        return;
    }

    put(outboundEvents_, Switch{std::move(id), rank, std::move(owner), std::move(title)});
    announce(&UiRegistry::outboundEventsChanged);
}

QStringList UiRegistry::outboundEvents() const
{
    return namesOf(outboundEvents_);
}

QString UiRegistry::outboundEventTitle(const QString &id) const
{
    return titleIn(outboundEvents_, id);
}

void UiRegistry::addSlotItem(QString slot, QString id, int rank, QString owner, QUrl component,
                             Title title, QString glyph)
{
    if (slot.isEmpty() || id.isEmpty()) {
        return;
    }

    auto entry =
        SlotItem{std::move(slot),  std::move(id),   rank, std::move(owner), std::move(component),
                 std::move(title), std::move(glyph)};
    const auto existing = std::ranges::find_if(
        slotItems_, [&entry](const SlotItem &item) { return item.id == entry.id; });
    if (existing != slotItems_.end()) {
        *existing = std::move(entry);
    } else {
        slotItems_.push_back(std::move(entry));
    }

    std::ranges::stable_sort(slotItems_,
                             [](const SlotItem &a, const SlotItem &b) { return a.rank < b.rank; });
    ++slotRevision_;
    announce(&UiRegistry::slotsChanged);
}

QStringList UiRegistry::itemsIn(const QString &slot) const
{
    QStringList ids;
    for (const auto &item : slotItems_) {
        if (item.slot == slot) {
            ids.push_back(item.id);
        }
    }
    return ids;
}

QUrl UiRegistry::componentOfItem(const QString &id) const
{
    const auto found =
        std::ranges::find_if(slotItems_, [&id](const SlotItem &item) { return item.id == id; });
    return found == slotItems_.end() ? QUrl{} : found->component;
}

QString UiRegistry::itemTitle(const QString &id) const
{
    const auto found =
        std::ranges::find_if(slotItems_, [&id](const SlotItem &item) { return item.id == id; });
    return found == slotItems_.end() || !found->title ? QString{} : found->title();
}

QString UiRegistry::itemGlyph(const QString &id) const
{
    const auto found =
        std::ranges::find_if(slotItems_, [&id](const SlotItem &item) { return item.id == id; });
    return found == slotItems_.end() ? QString{} : found->glyph;
}

void UiRegistry::addCatalogue(QString owner, QString directory, QString prefix)
{
    if (directory.isEmpty() || prefix.isEmpty()) {
        return;
    }

    const auto existing = std::ranges::find_if(
        catalogues_, [&owner](const Catalogue &c) { return c.owner == owner; });
    auto entry = Catalogue{std::move(owner), std::move(directory), std::move(prefix)};
    if (existing != catalogues_.end()) {
        *existing = std::move(entry);
    } else {
        catalogues_.push_back(std::move(entry));
    }
}

std::vector<UiRegistry::Catalogue> UiRegistry::catalogues() const
{
    return catalogues_;
}

void UiRegistry::stopAnnouncing() noexcept
{
    announcing_ = false;
}

void UiRegistry::announce(void (UiRegistry::*what)())
{

    if (what != &UiRegistry::settledChanged) {
        stirred();
    }

    if (announcing_) {
        (this->*what)();
    }
}

void UiRegistry::removeContributionsOf(const QString &owner)
{
    std::erase_if(catalogues_, [&owner](const Catalogue &c) { return c.owner == owner; });

    const auto sectionsBefore = sections_.size();
    std::erase_if(sections_, [&owner](const Section &section) { return section.owner == owner; });
    if (sections_.size() != sectionsBefore) {
        announce(&UiRegistry::settingsSectionsChanged);
    }

    const auto windowsBefore = windows_.size();
    std::erase_if(windows_, [&owner](const Window &window) { return window.owner == owner; });
    if (windows_.size() != windowsBefore) {
        announce(&UiRegistry::windowsChanged);
    }

    const auto resourcesBefore = apiResources_.size();
    std::erase_if(apiResources_, [&owner](const Switch &one) { return one.owner == owner; });
    if (apiResources_.size() != resourcesBefore) {
        announce(&UiRegistry::apiResourcesChanged);
    }

    const auto eventsBefore = outboundEvents_.size();
    std::erase_if(outboundEvents_, [&owner](const Switch &one) { return one.owner == owner; });
    if (outboundEvents_.size() != eventsBefore) {
        announce(&UiRegistry::outboundEventsChanged);
    }

    const auto slotsBefore = slotItems_.size();
    std::erase_if(slotItems_, [&owner](const SlotItem &item) { return item.owner == owner; });
    if (slotItems_.size() != slotsBefore) {
        ++slotRevision_;
        announce(&UiRegistry::slotsChanged);
    }
}

QStringList UiRegistry::settingsSections() const
{
    QStringList ids;
    ids.reserve(static_cast<qsizetype>(sections_.size()));
    for (const auto &section : sections_) {
        ids.push_back(section.id);
    }
    return ids;
}

QString UiRegistry::titleOf(const QString &id) const
{
    const auto found =
        std::ranges::find_if(sections_, [&id](const Section &section) { return section.id == id; });
    return found == sections_.end() ? QString{} : found->title();
}

QUrl UiRegistry::componentOf(const QString &id) const
{
    const auto found =
        std::ranges::find_if(sections_, [&id](const Section &section) { return section.id == id; });
    return found == sections_.end() ? QUrl{} : found->component;
}

QString UiRegistry::ownerOf(const QString &id) const
{
    const auto found =
        std::ranges::find_if(sections_, [&id](const Section &section) { return section.id == id; });
    return found == sections_.end() ? QString{} : found->owner;
}

QString UiRegistry::groupOf(const QString &id) const
{
    const auto found =
        std::ranges::find_if(sections_, [&id](const Section &section) { return section.id == id; });
    return found == sections_.end() ? QString{} : found->group;
}

void UiRegistry::retitle()
{
    ++revision_;
    announce(&UiRegistry::revisionChanged);
}

}
