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

#include <softphone/core/call.hpp>
#include <softphone/core/store.hpp>

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace softphone::core {

struct Contact {

    std::string id;

    std::string name;

    std::string number;

    std::string accountId;

    std::string accountAddress;

    std::chrono::system_clock::time_point createdAt{};

    std::chrono::system_clock::time_point updatedAt{};

    friend bool operator==(const Contact &, const Contact &) = default;
};

enum class ContactProblem { None, MissingName, MissingNumber };

[[nodiscard]] ContactProblem validate(const Contact &contact) noexcept;

[[nodiscard]] std::string_view toString(ContactProblem problem) noexcept;

[[nodiscard]] std::string numberKey(std::string_view number);

inline constexpr std::size_t kSignificantDigits = 7;

[[nodiscard]] bool numbersMatch(std::string_view left, std::string_view right);

class ContactBook {
public:
    ContactBook() = default;
    explicit ContactBook(std::vector<Contact> contacts);

    void reset(std::vector<Contact> contacts);

    [[nodiscard]] const std::vector<Contact> &all() const noexcept
    {
        return contacts_;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return contacts_.empty();
    }

    [[nodiscard]] const Contact *find(std::string_view number) const;

    [[nodiscard]] std::string nameFor(std::string_view number) const;

private:

    void index();

    std::vector<Contact> contacts_;
    std::unordered_map<std::string, std::size_t> byKey_;
    std::unordered_map<std::string, std::size_t> byTail_;
};

[[nodiscard]] CallInfo asKnown(CallInfo call, const ContactBook &book);

class IContactStore {
public:
    IContactStore() = default;
    IContactStore(const IContactStore &) = delete;
    IContactStore &operator=(const IContactStore &) = delete;
    IContactStore(IContactStore &&) = delete;
    IContactStore &operator=(IContactStore &&) = delete;
    virtual ~IContactStore() = default;

    [[nodiscard]] virtual StoreStatus contacts(std::vector<Contact> &out) = 0;

    [[nodiscard]] virtual StoreStatus save(Contact &contact) = 0;

    [[nodiscard]] virtual StoreStatus removeContact(const std::string &id) = 0;
};

}
