// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/container/services.hpp>

#include <algorithm>

namespace softphone::container {

bool ServiceRegistry::record(ServiceOffer offer, std::type_index type, void *service)
{
    const std::lock_guard lock{mutex_};

    const auto sameName = [&offer](const Entry &entry) { return entry.offer.name == offer.name; };
    if (std::ranges::any_of(entries_, sameName)) {
        return false;
    }

    entries_.push_back(Entry{std::move(offer), type, service});
    return true;
}

void *ServiceRegistry::lookUp(std::string_view name, std::type_index type,
                              const VersionRange &wanted) const
{
    const std::lock_guard lock{mutex_};

    for (const auto &entry : entries_) {
        if (entry.offer.name != name) {
            continue;
        }
        if (entry.type != type) {
            return nullptr;
        }
        if (!wanted.allows(entry.offer.version)) {
            return nullptr;
        }
        return entry.service;
    }
    return nullptr;
}

void ServiceRegistry::withdraw(std::string_view owner)
{
    const std::lock_guard lock{mutex_};
    std::erase_if(entries_, [owner](const Entry &entry) { return entry.offer.owner == owner; });
}

std::vector<ServiceOffer> ServiceRegistry::offered() const
{
    const std::lock_guard lock{mutex_};

    std::vector<ServiceOffer> offers;
    offers.reserve(entries_.size());
    for (const auto &entry : entries_) {
        offers.push_back(entry.offer);
    }
    return offers;
}

}
