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
#include <softphone/core/recordings.hpp>
#include <softphone/core/store.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::core {

enum class ConditionField {
    Direction,
    Account,

    Duration,

    Number,

    Category,
    Tag,

    Language,
    RedFlag,

    Hour,

    Source,
};

[[nodiscard]] std::string_view toString(ConditionField field) noexcept;
[[nodiscard]] std::optional<ConditionField> parseConditionField(std::string_view text) noexcept;

enum class ConditionOp {
    Is,
    IsNot,
    Contains,
    AtLeast,
    AtMost,

    Present,
    Absent,
};

[[nodiscard]] std::string_view toString(ConditionOp op) noexcept;
[[nodiscard]] std::optional<ConditionOp> parseConditionOp(std::string_view text) noexcept;

struct Condition {
    ConditionField field{ConditionField::Category};
    ConditionOp op{ConditionOp::Is};

    std::string value;
};

enum class Action {
    Transcribe,
    Generate,

    Tag,

    Stop,
};

[[nodiscard]] std::string_view toString(Action action) noexcept;
[[nodiscard]] std::optional<Action> parseAction(std::string_view text) noexcept;

struct Automation {
    std::string id;

    std::string code;

    std::string name;
    bool enabled{true};

    int ordinal{0};

    std::vector<Condition> conditions;
    Action action{Action::Generate};

    std::string templateCode;

    std::string tagCode;

    bool builtIn{false};

    std::chrono::system_clock::time_point createdAt{};
    std::chrono::system_clock::time_point updatedAt{};
};

struct PipelineFacts {
    std::string recordingId;
    CallDirection direction{CallDirection::Outgoing};
    std::string accountId;
    std::chrono::seconds duration{};
    std::string number;

    std::string categoryCode;
    std::vector<std::string> tags;
    std::vector<std::string> redFlags;

    std::string language;

    int hour{-1};

    RecordingSource source{RecordingSource::Native};
};

[[nodiscard]] bool holds(const Condition &condition, const PipelineFacts &facts);

[[nodiscard]] bool matches(const std::vector<Condition> &conditions, const PipelineFacts &facts);

[[nodiscard]] std::string encodeConditions(const std::vector<Condition> &conditions);
[[nodiscard]] std::vector<Condition> decodeConditions(std::string_view text);

[[nodiscard]] std::vector<Automation> shippedAutomations(std::string_view language = "en");

struct AutomationRun {
    std::string automationId;
    std::string recordingId;
    std::chrono::system_clock::time_point firedAt{};
    std::string jobId;
};

class IAutomationStore {
public:
    IAutomationStore() = default;
    IAutomationStore(const IAutomationStore &) = delete;
    IAutomationStore &operator=(const IAutomationStore &) = delete;
    IAutomationStore(IAutomationStore &&) = delete;
    IAutomationStore &operator=(IAutomationStore &&) = delete;
    virtual ~IAutomationStore() = default;

    [[nodiscard]] virtual StoreStatus save(Automation &automation) = 0;

    [[nodiscard]] virtual StoreStatus automations(std::vector<Automation> &out) = 0;

    [[nodiscard]] virtual StoreStatus automation(const std::string &id, Automation &out) = 0;

    [[nodiscard]] virtual StoreStatus removeAutomation(const std::string &id) = 0;

    [[nodiscard]] virtual StoreStatus restoreShippedAutomations() = 0;

    [[nodiscard]] virtual StoreStatus noteRun(const AutomationRun &run) = 0;

    [[nodiscard]] virtual StoreStatus hasRun(const std::string &automationId,
                                             const std::string &recordingId, bool &already) = 0;

    [[nodiscard]] virtual StoreStatus runsFor(const std::string &recordingId,
                                              std::vector<AutomationRun> &out) = 0;

    [[nodiscard]] virtual StoreStatus forgetRuns(const std::string &recordingId) = 0;
};

}
