// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/contacts.hpp>

#include <softphone/core/call.hpp>

#include <algorithm>
#include <cctype>

namespace softphone::core {
namespace {

[[nodiscard]] char lowerAscii(char character) noexcept
{
    return static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
}

[[nodiscard]] bool isDigit(char character) noexcept
{
    return character >= '0' && character <= '9';
}

[[nodiscard]] std::string digitsOf(std::string_view key)
{
    std::string digits;
    digits.reserve(key.size());
    for (const char character : key) {
        if (isDigit(character)) {
            digits.push_back(character);
        } else if (character != '+') {

            return {};
        }
    }
    return digits;
}

}

ContactProblem validate(const Contact &contact) noexcept
{
    if (contact.name.empty()) {
        return ContactProblem::MissingName;
    }
    if (contact.number.empty()) {
        return ContactProblem::MissingNumber;
    }
    return ContactProblem::None;
}

std::string_view toString(ContactProblem problem) noexcept
{
    switch (problem) {
    case ContactProblem::None:
        return "none";
    case ContactProblem::MissingName:
        return "missing-name";
    case ContactProblem::MissingNumber:
        return "missing-number";
    }
    return "unknown";
}

std::string numberKey(std::string_view number)
{

    const auto user = sipUserPart(number);

    std::string key;
    key.reserve(user.size());

    bool numeric = true;
    for (const char character : user) {
        if (isDigit(character)) {
            key.push_back(character);
        } else if (character == '+' && key.empty()) {

            key.push_back(character);
        } else if (character == ' ' || character == '-' || character == '(' || character == ')' ||
                   character == '.' || character == '\t') {

        } else {
            numeric = false;
        }
    }

    if (numeric) {
        return key;
    }

    std::string name;
    name.reserve(user.size());
    for (const char character : user) {
        name.push_back(lowerAscii(character));
    }
    return name;
}

bool numbersMatch(std::string_view left, std::string_view right)
{
    const auto leftKey = numberKey(left);
    const auto rightKey = numberKey(right);
    if (leftKey.empty() || rightKey.empty()) {
        return false;
    }
    if (leftKey == rightKey) {
        return true;
    }

    const auto leftDigits = digitsOf(leftKey);
    const auto rightDigits = digitsOf(rightKey);
    if (leftDigits.size() < kSignificantDigits || rightDigits.size() < kSignificantDigits) {
        return false;
    }

    return std::string_view{leftDigits}.substr(leftDigits.size() - kSignificantDigits) ==
           std::string_view{rightDigits}.substr(rightDigits.size() - kSignificantDigits);
}

CallInfo asKnown(CallInfo call, const ContactBook &book)
{

    if (call.direction == CallDirection::Outgoing && !call.dialedNumber.empty()) {
        if (auto dialled = book.nameFor(call.dialedNumber); !dialled.empty()) {
            call.remoteDisplayName = std::move(dialled);
            return call;
        }
    }

    const auto &asked = call.remoteUri.empty() ? call.remoteDisplayName : call.remoteUri;
    if (auto name = book.nameFor(asked); !name.empty()) {
        call.remoteDisplayName = std::move(name);
    }
    return call;
}

ContactBook::ContactBook(std::vector<Contact> contacts) : contacts_{std::move(contacts)}
{
    index();
}

void ContactBook::reset(std::vector<Contact> contacts)
{
    contacts_ = std::move(contacts);
    index();
}

void ContactBook::index()
{
    byKey_.clear();
    byTail_.clear();
    byKey_.reserve(contacts_.size());
    byTail_.reserve(contacts_.size());

    for (std::size_t at = 0; at < contacts_.size(); ++at) {
        auto key = numberKey(contacts_[at].number);
        if (key.empty()) {
            continue;
        }

        const auto digits = digitsOf(key);
        byKey_.try_emplace(std::move(key), at);

        if (digits.size() >= kSignificantDigits) {
            byTail_.try_emplace(digits.substr(digits.size() - kSignificantDigits), at);
        }
    }
}

const Contact *ContactBook::find(std::string_view number) const
{
    const auto wanted = numberKey(number);
    if (wanted.empty()) {
        return nullptr;
    }

    if (const auto found = byKey_.find(wanted); found != byKey_.end()) {
        return &contacts_[found->second];
    }

    const auto digits = digitsOf(wanted);
    if (digits.size() < kSignificantDigits) {
        return nullptr;
    }

    const auto tail = digits.substr(digits.size() - kSignificantDigits);
    if (const auto found = byTail_.find(tail); found != byTail_.end()) {
        return &contacts_[found->second];
    }
    return nullptr;
}

std::string ContactBook::nameFor(std::string_view number) const
{
    const auto *contact = find(number);
    return contact == nullptr ? std::string{} : contact->name;
}

}
