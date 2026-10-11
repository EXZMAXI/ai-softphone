// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/automations.hpp>

#include "shipped_content.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <string>

namespace softphone::core {
namespace {

[[nodiscard]] bool listHas(const std::vector<std::string> &all, const std::string &one)
{
    return std::find(all.begin(), all.end(), one) != all.end();
}

[[nodiscard]] std::optional<long long> numberIn(const std::string &value)
{
    try {
        std::size_t used = 0;
        const auto parsed = std::stoll(value, &used);
        return used == value.size() ? std::optional<long long>{parsed} : std::nullopt;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

}

std::string_view toString(ConditionField field) noexcept
{
    switch (field) {
    case ConditionField::Direction:
        return "direction";
    case ConditionField::Account:
        return "account";
    case ConditionField::Duration:
        return "duration";
    case ConditionField::Number:
        return "number";
    case ConditionField::Category:
        return "category";
    case ConditionField::Tag:
        return "tag";
    case ConditionField::Language:
        return "language";
    case ConditionField::RedFlag:
        return "red_flag";
    case ConditionField::Hour:
        return "hour";
    case ConditionField::Source:
        return "source";
    }
    return "category";
}

std::optional<ConditionField> parseConditionField(std::string_view text) noexcept
{
    static constexpr std::array<std::pair<std::string_view, ConditionField>, 10> kAll{{
        {"direction", ConditionField::Direction},
        {"account", ConditionField::Account},
        {"duration", ConditionField::Duration},
        {"number", ConditionField::Number},
        {"category", ConditionField::Category},
        {"tag", ConditionField::Tag},
        {"language", ConditionField::Language},
        {"red_flag", ConditionField::RedFlag},
        {"hour", ConditionField::Hour},
        {"source", ConditionField::Source},
    }};
    for (const auto &[name, field] : kAll) {
        if (text == name) {
            return field;
        }
    }
    return std::nullopt;
}

std::string_view toString(ConditionOp op) noexcept
{
    switch (op) {
    case ConditionOp::Is:
        return "is";
    case ConditionOp::IsNot:
        return "is_not";
    case ConditionOp::Contains:
        return "contains";
    case ConditionOp::AtLeast:
        return "at_least";
    case ConditionOp::AtMost:
        return "at_most";
    case ConditionOp::Present:
        return "present";
    case ConditionOp::Absent:
        return "absent";
    }
    return "is";
}

std::optional<ConditionOp> parseConditionOp(std::string_view text) noexcept
{
    static constexpr std::array<std::pair<std::string_view, ConditionOp>, 7> kAll{{
        {"is", ConditionOp::Is},
        {"is_not", ConditionOp::IsNot},
        {"contains", ConditionOp::Contains},
        {"at_least", ConditionOp::AtLeast},
        {"at_most", ConditionOp::AtMost},
        {"present", ConditionOp::Present},
        {"absent", ConditionOp::Absent},
    }};
    for (const auto &[name, op] : kAll) {
        if (text == name) {
            return op;
        }
    }
    return std::nullopt;
}

std::string_view toString(Action action) noexcept
{
    switch (action) {
    case Action::Transcribe:
        return "transcribe";
    case Action::Generate:
        return "generate";
    case Action::Tag:
        return "tag";
    case Action::Stop:
        return "stop";
    }
    return "generate";
}

std::optional<Action> parseAction(std::string_view text) noexcept
{
    if (text == "transcribe") {
        return Action::Transcribe;
    }
    if (text == "generate") {
        return Action::Generate;
    }
    if (text == "tag") {
        return Action::Tag;
    }
    if (text == "stop") {
        return Action::Stop;
    }
    return std::nullopt;
}

bool holds(const Condition &condition, const PipelineFacts &facts)
{

    const auto oneOf = [&facts](ConditionField field) -> std::string {
        switch (field) {
        case ConditionField::Direction:
            return facts.direction == CallDirection::Incoming ? "in" : "out";
        case ConditionField::Account:
            return facts.accountId;
        case ConditionField::Duration:
            return std::to_string(facts.duration.count());
        case ConditionField::Number:
            return facts.number;
        case ConditionField::Category:
            return facts.categoryCode;
        case ConditionField::Language:
            return facts.language;
        case ConditionField::Hour:
            return facts.hour < 0 ? std::string{} : std::to_string(facts.hour);
        case ConditionField::Source:
            return std::string{toString(facts.source)};
        case ConditionField::Tag:
        case ConditionField::RedFlag:
            break;
        }
        return {};
    };

    const auto listOf = [&facts](ConditionField field) -> const std::vector<std::string> & {
        static const std::vector<std::string> kNothing;
        switch (field) {
        case ConditionField::Tag:
            return facts.tags;
        case ConditionField::RedFlag:
            return facts.redFlags;
        default:
            break;
        }
        return kNothing;
    };

    const auto isList =
        condition.field == ConditionField::Tag || condition.field == ConditionField::RedFlag;

    const auto isFiledUnder = [&facts](const std::string &code) {
        return facts.categoryCode == code;
    };

    switch (condition.op) {
    case ConditionOp::Present:

        return isList
                   ? (condition.value.empty() ? !listOf(condition.field).empty()
                                              : listHas(listOf(condition.field), condition.value))
                   : !oneOf(condition.field).empty();

    case ConditionOp::Absent:
        return isList
                   ? (condition.value.empty() ? listOf(condition.field).empty()
                                              : !listHas(listOf(condition.field), condition.value))
                   : oneOf(condition.field).empty();

    case ConditionOp::Is:
        if (condition.field == ConditionField::Category) {
            return isFiledUnder(condition.value);
        }
        return isList ? listHas(listOf(condition.field), condition.value)
                      : oneOf(condition.field) == condition.value;

    case ConditionOp::IsNot:
        if (condition.field == ConditionField::Category) {
            return !isFiledUnder(condition.value);
        }
        return isList ? !listHas(listOf(condition.field), condition.value)
                      : oneOf(condition.field) != condition.value;

    case ConditionOp::Contains: {
        if (isList) {
            return listHas(listOf(condition.field), condition.value);
        }
        const auto value = oneOf(condition.field);
        return !condition.value.empty() && value.find(condition.value) != std::string::npos;
    }

    case ConditionOp::AtLeast:
    case ConditionOp::AtMost: {
        const auto wanted = numberIn(condition.value);
        const auto have = numberIn(oneOf(condition.field));
        if (!wanted.has_value() || !have.has_value()) {

            return false;
        }
        return condition.op == ConditionOp::AtLeast ? *have >= *wanted : *have <= *wanted;
    }
    }
    return false;
}

bool matches(const std::vector<Condition> &conditions, const PipelineFacts &facts)
{

    return std::all_of(conditions.begin(), conditions.end(),
                       [&facts](const Condition &one) { return holds(one, facts); });
}

std::string encodeConditions(const std::vector<Condition> &conditions)
{
    if (conditions.empty()) {
        return {};
    }
    auto json = nlohmann::ordered_json::array();
    for (const auto &one : conditions) {
        json.push_back({{"field", std::string{toString(one.field)}},
                        {"op", std::string{toString(one.op)}},
                        {"value", one.value}});
    }
    return json.dump();
}

std::vector<Condition> decodeConditions(std::string_view text)
{
    std::vector<Condition> out;
    if (text.empty()) {
        return out;
    }
    const auto parsed = nlohmann::json::parse(text, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_array()) {
        return out;
    }

    for (const auto &entry : parsed) {
        if (!entry.is_object()) {
            return {};
        }
        const auto field = parseConditionField(entry.value("field", std::string{}));
        const auto op = parseConditionOp(entry.value("op", std::string{}));
        if (!field.has_value() || !op.has_value()) {

            return {};
        }
        Condition one;
        one.field = *field;
        one.op = *op;
        one.value = entry.value("value", std::string{});
        out.push_back(std::move(one));
    }
    return out;
}

std::vector<Automation> shippedAutomations(std::string_view language)
{
    const auto &rows = content::listOf(language, "automations");

    std::vector<Automation> all;
    all.reserve(rows.size());

    for (const auto &row : rows) {
        if (!row.is_object()) {
            continue;
        }
        const auto action = parseAction(row.value("action", std::string{}));
        auto code = row.value("code", std::string{});
        if (code.empty() || !action.has_value()) {
            continue;
        }

        Automation one;
        one.code = std::move(code);
        one.name = row.value("name", std::string{});
        one.action = *action;
        one.ordinal = row.value("ordinal", 0);
        one.templateCode = row.value("prompt", std::string{});
        one.tagCode = row.value("tag", std::string{});
        one.enabled = row.value("enabled", true);
        one.builtIn = true;

        if (const auto clauses = row.find("conditions");
            clauses != row.end() && clauses->is_array()) {
            one.conditions = decodeConditions(clauses->dump());
        }

        all.push_back(std::move(one));
    }
    return all;
}

}
