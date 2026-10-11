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

#include <softphone/container/services.hpp>
#include <softphone/core/taxonomy.hpp>

#include <string>
#include <utility>
#include <vector>

namespace softphone::ui {

class Vocabulary final : public core::ITaxonomyStore {
public:
    Vocabulary(container::ServiceRegistry &services, std::string name)
        : services_{services}, name_{std::move(name)}
    {}

    [[nodiscard]] bool present() const
    {
        return services_.find<core::ITaxonomyStore>(name_) != nullptr;
    }

    [[nodiscard]] core::StoreStatus save(core::TaxonomyEntry &entry) override
    {
        return now().save(entry);
    }

    [[nodiscard]] core::StoreStatus entries(core::TaxonomyKind kind, bool includeRetired,
                                            std::vector<core::TaxonomyEntry> &out) override
    {
        return now().entries(kind, includeRetired, out);
    }

    [[nodiscard]] core::StoreStatus entry(core::TaxonomyKind kind, const std::string &code,
                                          core::TaxonomyEntry &out) override
    {
        return now().entry(kind, code, out);
    }

    [[nodiscard]] core::StoreStatus retireEntry(const std::string &id, bool retired) override
    {
        return now().retireEntry(id, retired);
    }

    [[nodiscard]] core::StoreStatus restoreShippedTaxonomy() override
    {
        return now().restoreShippedTaxonomy();
    }

    [[nodiscard]] core::StoreStatus removeEntry(const std::string &id,
                                                bool &retiredInstead) override
    {
        return now().removeEntry(id, retiredInstead);
    }

    [[nodiscard]] core::StoreStatus usedCodes(core::TaxonomyKind kind,
                                              std::vector<std::string> &out) override
    {
        return now().usedCodes(kind, out);
    }

    [[nodiscard]] core::StoreStatus tags(const std::string &recordingId,
                                         std::vector<core::RecordingTag> &out) override
    {
        return now().tags(recordingId, out);
    }

    [[nodiscard]] core::StoreStatus setTag(const std::string &recordingId,
                                           const core::RecordingTag &tag) override
    {
        return now().setTag(recordingId, tag);
    }

    [[nodiscard]] core::StoreStatus clearTag(const std::string &recordingId,
                                             const std::string &code) override
    {
        return now().clearTag(recordingId, code);
    }

    [[nodiscard]] core::StoreStatus clearTagsFrom(const std::string &recordingId,
                                                  const std::string &templateCode) override
    {
        return now().clearTagsFrom(recordingId, templateCode);
    }

    [[nodiscard]] core::StoreStatus redFlags(const std::string &recordingId,
                                             std::vector<core::RedFlagHit> &out) override
    {
        return now().redFlags(recordingId, out);
    }

    [[nodiscard]] core::StoreStatus saveRedFlag(core::RedFlagHit &hit) override
    {
        return now().saveRedFlag(hit);
    }

    [[nodiscard]] core::StoreStatus dismissRedFlag(const std::string &id, bool dismissed) override
    {
        return now().dismissRedFlag(id, dismissed);
    }

    [[nodiscard]] core::StoreStatus clearRedFlagsFrom(const std::string &recordingId,
                                                      const std::string &templateCode) override
    {
        return now().clearRedFlagsFrom(recordingId, templateCode);
    }

private:

    [[nodiscard]] core::ITaxonomyStore &now()
    {
        auto *const found = services_.find<core::ITaxonomyStore>(name_);
        return found != nullptr ? *found : none_;
    }

    container::ServiceRegistry &services_;
    std::string name_;
    core::NoDictionaries none_;
};

}
