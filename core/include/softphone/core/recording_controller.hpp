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
#include <softphone/core/conversation_recorder.hpp>
#include <softphone/core/recording_consent.hpp>
#include <softphone/core/recording_taps.hpp>
#include <softphone/core/recordings.hpp>

#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace softphone::core {

class RecordingController {
public:

    using EncoderMaker = std::function<std::unique_ptr<IAudioEncoder>(
        const std::filesystem::path &file, const EncoderSettings &settings)>;

    struct ConsentPlan {
        ConsentPosture posture{ConsentPosture::Announce};

        std::string announcement;

        std::string shippedAnnouncement;

        std::string tone;

        std::chrono::seconds interval{15};

        bool keepInTheRecording{false};
    };

    struct State {
        bool running{false};
        bool paused{false};

        bool keeping{false};

        std::chrono::system_clock::time_point startedAt{};
        std::string recordingId;
        ConsentPosture consent{ConsentPosture::Announce};

        std::vector<std::string> calls;

        bool conference{false};
    };

    enum class WhenToRecord {

        Never,

        Ask,

        Always,
    };

    struct AccountRecording {
        WhenToRecord when{WhenToRecord::Never};
        ConsentPlan consent;
    };

    void setAccountPolicies(std::map<std::string, AccountRecording> policies);

    using AskCallback = std::function<void(const std::string &callId)>;
    void setAskListener(AskCallback ask);

    using WrittenCallback = std::function<void(const Recording &)>;
    void setWrittenListener(WrittenCallback written);

    using StateCallback = std::function<void(const std::vector<State> &)>;

    struct Finished {
        RecordingOutcome outcome;
        Recording row;
    };

    RecordingController(IRecordingTaps &taps, EncoderMaker maker, std::filesystem::path root,
                        IRecordingStore *store = nullptr, IConsentVoice *voice = nullptr);

    void setListener(StateCallback listener);

    void setRoomCheck(std::function<bool()> hasRoom);

    void setConsent(const ConsentPlan &plan);

    void setKeepStems(bool keeping) noexcept
    {
        keepStems_ = keeping;
    }

    RecordingController(const RecordingController &) = delete;
    RecordingController &operator=(const RecordingController &) = delete;
    RecordingController(RecordingController &&) = delete;
    RecordingController &operator=(RecordingController &&) = delete;
    ~RecordingController();

    [[nodiscard]] EncoderStatus start(const std::string &callId,
                                      const std::vector<CallInfo> &calls);

    void callsChanged(const std::vector<CallInfo> &calls);

    void setPaused(const std::string &callId, bool paused);

    Finished stop(const std::string &callId);

    void stopAll();

    [[nodiscard]] std::vector<State> states() const;

    [[nodiscard]] std::optional<State> state(const std::string &callId) const;

    [[nodiscard]] bool recording() const noexcept
    {
        return !sessions_.empty();
    }

    [[nodiscard]] bool recording(const std::string &callId) const;

    [[nodiscard]] std::size_t count() const noexcept
    {
        return sessions_.size();
    }

    [[nodiscard]] Recording row(const std::string &callId) const;

    [[nodiscard]] std::vector<std::string> subject(const std::string &callId) const;

    [[nodiscard]] bool paused(const std::string &callId) const;

    [[nodiscard]] bool speaking(const std::string &callId) const;

    [[nodiscard]] bool announced(const std::string &callId) const;

    [[nodiscard]] bool keeping(const std::string &callId) const;

    [[nodiscard]] std::chrono::milliseconds elapsed(const std::string &callId) const;

    [[nodiscard]] std::filesystem::path file(const std::string &callId) const;

    [[nodiscard]] const std::filesystem::path &root() const noexcept
    {
        return root_;
    }

    [[nodiscard]] static std::string relativePath(const std::string &recordingId,
                                                  std::chrono::system_clock::time_point when,
                                                  const std::string &callId = {});

private:

    struct Session {

        RecordingFeed *feed{nullptr};

        std::unique_ptr<ConversationRecorder> recorder;

        Recording row;

        std::vector<std::string> subject;

        bool conference{false};

        std::map<std::string, std::string> durable;

        ConsentPlan consent;

        bool keepStems{true};

        bool announcing{false};
        bool announced{true};
        bool live{true};
        bool paused{false};
        bool keeping{true};

        bool speaking{true};

        bool spoke{true};
        bool kept{true};

        bool asked{false};

        std::set<std::string> told;
    };

    void moveRecordings(const std::vector<CallInfo> &calls);

    void formConference(const std::vector<CallInfo> &calls);

    [[nodiscard]] ConsentPlan planForTheRoom(const std::vector<std::string> &group,
                                             const std::vector<CallInfo> &calls) const;

    void considerAnswered(const std::vector<CallInfo> &calls);

    [[nodiscard]] AccountRecording policyFor(const std::string &accountId) const;

    [[nodiscard]] Session *sessionOf(const std::string &callId);
    [[nodiscard]] const Session *sessionOf(const std::string &callId) const;
    [[nodiscard]] Session *sessionById(const std::string &recordingId);

    [[nodiscard]] static State snapshot(const Session &session);

    void apply(Session &session);

    [[nodiscard]] static bool wantsStems(const Session &session) noexcept;

    void describe(Session &session, const std::vector<CallInfo> &calls);

    void obtainConsent(Session &session);

    void tellNewcomers(Session &session);

    Finished close(std::vector<std::unique_ptr<Session>>::iterator where);

    void publish();

    WrittenCallback written_;

    IRecordingTaps &taps_;
    EncoderMaker maker_;
    std::filesystem::path root_;
    IRecordingStore *store_;
    IConsentVoice *voice_;

    ConsentPlan consent_;
    bool keepStems_{true};

    std::map<std::string, AccountRecording> accounts_;

    std::set<std::string> considered_;

    std::vector<std::string> lastGroup_;

    StateCallback listener_;
    AskCallback ask_;
    std::function<bool()> hasRoom_;

    std::vector<std::unique_ptr<Session>> sessions_;
};

}
