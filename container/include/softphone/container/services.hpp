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

#include <softphone/container/version.hpp>

#include <mutex>
#include <string>
#include <string_view>
#include <typeindex>
#include <vector>

namespace softphone::container {

struct ServiceOffer {
    std::string name;
    Version version;

    std::string owner;
};

class ServiceRegistry {
public:
    ServiceRegistry() = default;
    ServiceRegistry(const ServiceRegistry &) = delete;
    ServiceRegistry &operator=(const ServiceRegistry &) = delete;
    ServiceRegistry(ServiceRegistry &&) = delete;
    ServiceRegistry &operator=(ServiceRegistry &&) = delete;
    ~ServiceRegistry() = default;

    template<typename T>
    [[nodiscard]] bool offer(std::string name, Version version, std::string owner, T *service)
    {
        if (service == nullptr) {
            return false;
        }
        return record({std::move(name), version, std::move(owner)}, std::type_index(typeid(T)),
                      service);
    }

    template<typename T>
    [[nodiscard]] T *find(std::string_view name, const VersionRange &wanted = {}) const
    {
        return static_cast<T *>(lookUp(name, std::type_index(typeid(T)), wanted));
    }

    void withdraw(std::string_view owner);

    [[nodiscard]] std::vector<ServiceOffer> offered() const;

private:
    struct Entry {
        ServiceOffer offer;
        std::type_index type;
        void *service{nullptr};
    };

    [[nodiscard]] bool record(ServiceOffer offer, std::type_index type, void *service);
    [[nodiscard]] void *lookUp(std::string_view name, std::type_index type,
                               const VersionRange &wanted) const;

    mutable std::mutex mutex_;
    std::vector<Entry> entries_;
};

}
