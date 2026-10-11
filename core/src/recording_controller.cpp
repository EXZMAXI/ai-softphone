// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/recording_controller.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/uuid.hpp>

#include <algorithm>
#include <cstdio>
#include <ctime>

namespace softphone::core {
namespace {

[[nodiscard]] std::vector<std::string> onTheAudio(const std::vector<CallInfo> &calls)
{
    std::vector<std::string> group;
    for (const CallInfo &call : calls) {
        if (call.holdsTheAudio()) {
            group.push_back(call.id);
        }
    }
    return group;
}

[[nodiscard]] bool holds(const std::vector<std::string> &where, const std::string &what)
{
    return std::find(where.begin(), where.end(), what) != where.end();
}

[[nodiscard]] bool speakingTo(const std::vector<std::string> &subject,
                              const std::vector<CallInfo> &calls)
{
    bool anyone = false;

    for (const std::string &leg : subject) {
        const auto found = std::find_if(calls.begin(), calls.end(),
                                        [&](const CallInfo &call) { return call.id == leg; });
        if (found == calls.end() || found->state == CallState::Ended) {
            continue;
        }
        anyone = true;
        if (!found->muted) {
            return true;
        }
    }

    return !anyone;
}

[[nodiscard]] int strictness(ConsentPosture posture) noexcept
{
    switch (posture) {
    case ConsentPosture::Announce:
        return 3;
    case ConsentPosture::Tone:
        return 2;
    case ConsentPosture::Confirm:
        return 1;
    case ConsentPosture::Silent:
        break;
    }
    return 0;
}

[[nodiscard]] std::string plainly(const std::string &text)
{
    std::string out;
    out.reserve(text.size());
    for (const char letter : text) {
        const bool safe = (letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z') ||
                          (letter >= '0' && letter <= '9') || letter == '-' || letter == '_';
        out.push_back(safe ? letter : '-');
    }
    return out.empty() ? std::string{"unnamed"} : out;
}

[[nodiscard]] std::string naming(const std::vector<std::string> &callIds)
{
    std::string joined;
    for (const auto &callId : callIds) {
        if (!joined.empty()) {
            joined += ' ';
        }
        joined += callId;
    }
    return joined;
}

void note(LogLevel level, std::string_view calls, std::string_view what)
{
    const LogAttribute fields[] = {{"call", calls}};
    Logger::instance().log(level, "recording", what, fields);
}

}

RecordingController::RecordingController(IRecordingTaps &taps, EncoderMaker maker,
                                         std::filesystem::path root, IRecordingStore *store,
                                         IConsentVoice *voice)
    : taps_{taps}, maker_{std::move(maker)}, root_{std::move(root)}, store_{store}, voice_{voice}
{}

void RecordingController::setConsent(const ConsentPlan &plan)
{
    consent_ = plan;
}

void RecordingController::setListener(StateCallback listener)
{
    listener_ = std::move(listener);
}

void RecordingController::setRoomCheck(std::function<bool()> hasRoom)
{
    hasRoom_ = std::move(hasRoom);
}

void RecordingController::setAccountPolicies(std::map<std::string, AccountRecording> policies)
{
    accounts_ = std::move(policies);
}

void RecordingController::setWrittenListener(WrittenCallback written)
{
    written_ = std::move(written);
}

void RecordingController::setAskListener(AskCallback ask)
{
    ask_ = std::move(ask);
}

RecordingController::AccountRecording
RecordingController::policyFor(const std::string &accountId) const
{
    if (const auto found = accounts_.find(accountId); found != accounts_.end()) {
        return found->second;
    }

    return AccountRecording{WhenToRecord::Never, consent_};
}

RecordingController::Session *RecordingController::sessionOf(const std::string &callId)
{
    Session *own = nullptr;
    for (const auto &session : sessions_) {
        if (!holds(session->subject, callId)) {
            continue;
        }
        if (session->conference) {
            return session.get();
        }
        if (own == nullptr) {
            own = session.get();
        }
    }
    return own;
}

const RecordingController::Session *RecordingController::sessionOf(const std::string &callId) const
{
    const Session *own = nullptr;
    for (const auto &session : sessions_) {
        if (!holds(session->subject, callId)) {
            continue;
        }
        if (session->conference) {
            return session.get();
        }
        if (own == nullptr) {
            own = session.get();
        }
    }
    return own;
}

RecordingController::Session *RecordingController::sessionById(const std::string &recordingId)
{
    for (const auto &session : sessions_) {
        if (session->row.id == recordingId) {
            return session.get();
        }
    }
    return nullptr;
}

RecordingController::State RecordingController::snapshot(const Session &session)
{
    State now;
    now.running = session.recorder != nullptr;
    now.paused = session.paused;
    now.keeping = session.keeping;
    now.startedAt = session.row.startedAt;
    now.recordingId = session.row.id;
    now.consent = session.row.consent;
    now.calls = session.subject;
    now.conference = session.conference;
    return now;
}

std::vector<RecordingController::State> RecordingController::states() const
{
    std::vector<State> all;
    all.reserve(sessions_.size());
    for (const auto &session : sessions_) {
        all.push_back(snapshot(*session));
    }
    return all;
}

std::optional<RecordingController::State>
RecordingController::state(const std::string &callId) const
{
    const Session *session = sessionOf(callId);
    return session == nullptr ? std::nullopt : std::optional<State>{snapshot(*session)};
}

bool RecordingController::recording(const std::string &callId) const
{
    return sessionOf(callId) != nullptr;
}

Recording RecordingController::row(const std::string &callId) const
{
    const Session *session = sessionOf(callId);
    return session == nullptr ? Recording{} : session->row;
}

std::vector<std::string> RecordingController::subject(const std::string &callId) const
{
    const Session *session = sessionOf(callId);
    return session == nullptr ? std::vector<std::string>{} : session->subject;
}

bool RecordingController::paused(const std::string &callId) const
{
    const Session *session = sessionOf(callId);
    return session != nullptr && session->paused;
}

bool RecordingController::speaking(const std::string &callId) const
{
    const Session *session = sessionOf(callId);
    return session != nullptr && session->speaking;
}

bool RecordingController::announced(const std::string &callId) const
{
    const Session *session = sessionOf(callId);

    return session == nullptr || session->announced;
}

bool RecordingController::keeping(const std::string &callId) const
{
    const Session *session = sessionOf(callId);
    return session != nullptr && session->keeping;
}

std::chrono::milliseconds RecordingController::elapsed(const std::string &callId) const
{
    const Session *session = sessionOf(callId);
    return session == nullptr || !session->recorder ? std::chrono::milliseconds{0}
                                                    : session->recorder->elapsed();
}

std::filesystem::path RecordingController::file(const std::string &callId) const
{
    const Session *session = sessionOf(callId);
    if (session == nullptr || session->row.filePath.empty()) {
        return {};
    }
    return root_ / session->row.filePath;
}

void RecordingController::publish()
{
    if (listener_) {
        listener_(states());
    }
}

RecordingController::~RecordingController()
{

    listener_ = nullptr;
    stopAll();
}

std::string RecordingController::relativePath(const std::string &recordingId,
                                              std::chrono::system_clock::time_point when,
                                              const std::string &callId)
{

    const auto seconds = std::chrono::system_clock::to_time_t(when);
    std::tm broken{};
#if defined(_WIN32)
    localtime_s(&broken, &seconds);
#else
    localtime_r(&seconds, &broken);
#endif

    char day[64] = {};
    std::snprintf(day, sizeof day, "%04d/%02d/%02d", broken.tm_year + 1900, broken.tm_mon + 1,
                  broken.tm_mday);

    const std::string id = plainly(recordingId);
    if (callId.empty()) {
        return std::string{day} + '/' + id + ".mp3";
    }
    return std::string{day} + '/' + id + '/' + plainly(callId) + ".mp3";
}

void RecordingController::describe(Session &session, const std::vector<CallInfo> &calls)
{
    Recording &row = session.row;
    row.legs.clear();

    const auto elapsed =
        session.recorder ? session.recorder->elapsed() : std::chrono::milliseconds{0};

    for (const std::string &leg : session.subject) {
        const auto found = std::find_if(calls.begin(), calls.end(),
                                        [&](const CallInfo &call) { return call.id == leg; });

        RecordingLeg written;
        written.callId = leg;
        written.joined = elapsed;
        if (found != calls.end()) {
            written.remoteNumber = sipUserPart(found->remoteUri);
            written.remoteUri = found->remoteUri;
            written.remoteName = found->remoteDisplayName;
            if (row.seanceId.empty()) {
                row.seanceId = found->seanceId;
            }

            if (!found->uuid.empty()) {
                session.durable[leg] = found->uuid;
            }
        }

        if (const auto durable = session.durable.find(leg); durable != session.durable.end()) {
            written.callId = durable->second;
        }

        const auto before =
            std::find_if(row.legs.begin(), row.legs.end(),
                         [&](const RecordingLeg &had) { return had.callId == written.callId; });
        if (before != row.legs.end()) {
            written.joined = before->joined;
        }

        row.legs.push_back(std::move(written));
    }
}

EncoderStatus RecordingController::start(const std::string &callId,
                                         const std::vector<CallInfo> &calls)
{

    const auto group = onTheAudio(calls);
    const bool inTheMix = holds(group, callId);
    const bool theRoom = inTheMix && group.size() > 1;

    for (const auto &running : sessions_) {
        if (running->conference == theRoom && holds(running->subject, callId)) {
            return EncoderStatus::Ok;
        }
    }
    if (!maker_) {
        return EncoderStatus::Failed;
    }

    if (hasRoom_ && !hasRoom_()) {
        note(LogLevel::Warning, callId, "not recording: there is not enough room left on the disk");

        return EncoderStatus::NoRoom;
    }

    auto held = std::make_unique<Session>();
    Session &session = *held;

    const auto opener = std::find_if(calls.begin(), calls.end(),
                                     [&](const CallInfo &call) { return call.id == callId; });

    session.conference = theRoom;
    session.consent = theRoom                 ? planForTheRoom(group, calls)
                      : opener != calls.end() ? policyFor(opener->accountId).consent
                                              : consent_;
    session.keepStems = keepStems_;

    session.announced = session.consent.posture != ConsentPosture::Announce;

    session.subject = theRoom ? group : std::vector<std::string>{callId};
    session.live = inTheMix;

    session.speaking = speakingTo(session.subject, calls);
    session.spoke = true;
    session.kept = true;

    const auto now = std::chrono::system_clock::now();

    Recording &row = session.row;
    row.id = makeUuid();
    row.source = RecordingSource::Native;
    row.startedAt = now;
    row.filePath = relativePath(row.id, now);

    if (opener != calls.end()) {
        row.accountId = opener->accountId;
        row.seanceId = opener->seanceId;
        row.direction = opener->direction;
        row.remoteNumber = sipUserPart(opener->remoteUri);
        row.remoteUri = opener->remoteUri;
        row.remoteName = opener->remoteDisplayName;
    }

    RecordingFeed &feed = taps_.openRecording(row.id);
    session.feed = &feed;

    row.sampleRate = feed.sampleRate();
    row.channels = 2;
    row.consent = session.consent.posture;

    if (session.consent.posture == ConsentPosture::Confirm) {
        row.consentAt = now;
    }

    const EncoderSettings settings{feed.sampleRate(), 2, 32};
    const EncoderSettings stemSettings{feed.sampleRate(), 1, 32};
    const std::string recordingId = row.id;
    const bool keepStems = session.keepStems;

    session.recorder = std::make_unique<ConversationRecorder>(
        feed, [this, settings, stemSettings, recordingId, keepStems, now](const std::string &leg) {
            if (leg.empty()) {
                return maker_(root_ / relativePath(recordingId, now), settings);
            }
            if (!keepStems) {
                return std::unique_ptr<IAudioEncoder>{};
            }
            return maker_(root_ / relativePath(recordingId, now, leg), stemSettings);
        });

    if (const auto status = session.recorder->start(); !succeeded(status)) {
        taps_.closeRecording(recordingId);
        return status;
    }

    sessions_.push_back(std::move(held));

    describe(session, calls);

    apply(session);

    row.unfinished = true;
    if (store_ != nullptr) {
        if (const auto status = store_->save(row); !succeeded(status)) {
            note(LogLevel::Error, naming(session.subject),
                 "the recording started and its row was not written: " +
                     std::string{toString(status)});
        }
        static_cast<void>(store_->noteAccess(row.id, RecordingAccess::Created, row.filePath));
    }

    note(LogLevel::Info, naming(session.subject),
         std::string{theRoom ? "recording the conference, " : "recording one call, "} +
             std::to_string(session.subject.size()) + " leg(s) to " + row.filePath + " (" +
             std::to_string(sessions_.size()) + " in progress)");
    return EncoderStatus::Ok;
}

void RecordingController::callsChanged(const std::vector<CallInfo> &calls)
{
    moveRecordings(calls);

    considerAnswered(calls);

    formConference(calls);
}

void RecordingController::moveRecordings(const std::vector<CallInfo> &calls)
{
    if (sessions_.empty()) {
        return;
    }

    const auto group = onTheAudio(calls);

    std::vector<std::string> finished;

    const auto stillThere = [&calls](const std::string &leg) {
        return std::any_of(calls.begin(), calls.end(), [&](const CallInfo &call) {
            return call.id == leg && call.state != CallState::Ended;
        });
    };

    for (const auto &held : sessions_) {
        Session &session = *held;

        if (session.conference) {

            const bool ours =
                std::any_of(session.subject.begin(), session.subject.end(),
                            [&](const std::string &leg) { return holds(group, leg); });

            if (ours) {
                session.subject = group;
            } else {

                std::vector<std::string> left;
                for (const std::string &leg : session.subject) {
                    if (stillThere(leg)) {
                        left.push_back(leg);
                    }
                }
                session.subject = left;
            }

            session.live = ours;
        } else {

            const std::string leg =
                session.subject.empty() ? std::string{} : session.subject.front();
            const bool alive = !leg.empty() && stillThere(leg);
            session.subject = alive ? std::vector<std::string>{leg} : std::vector<std::string>{};
            session.live = alive && holds(group, leg);
        }

        session.speaking = speakingTo(session.subject, calls);

        const bool over = session.subject.empty() ||
                          (session.conference && session.live && session.subject.size() < 2);

        const auto before = session.row.legs;
        describe(session, calls);
        for (const RecordingLeg &gone : before) {
            const bool still =
                std::any_of(session.row.legs.begin(), session.row.legs.end(),
                            [&](const RecordingLeg &leg) { return leg.callId == gone.callId; });
            if (!still) {
                RecordingLeg departed = gone;
                if (departed.left.count() == 0) {
                    departed.left = session.recorder->elapsed();
                }
                session.row.legs.push_back(departed);
            }
        }
        std::sort(session.row.legs.begin(), session.row.legs.end(),
                  [](const RecordingLeg &one, const RecordingLeg &other) {
                      return one.joined < other.joined;
                  });

        if (over) {
            finished.push_back(session.row.id);
            continue;
        }

        apply(session);
    }

    for (const std::string &over : finished) {
        const auto where = std::find_if(
            sessions_.begin(), sessions_.end(),
            [&](const std::unique_ptr<Session> &session) { return session->row.id == over; });
        if (where == sessions_.end()) {
            continue;
        }
        note(LogLevel::Info, naming((*where)->subject),
             (*where)->conference ? "the conference is over: closing its recording"
                                  : "the conversation is over: closing the recording");
        static_cast<void>(close(where));
    }
}

RecordingController::ConsentPlan
RecordingController::planForTheRoom(const std::vector<std::string> &group,
                                    const std::vector<CallInfo> &calls) const
{
    ConsentPlan plan = consent_;
    bool first = true;

    for (const std::string &leg : group) {
        const auto found = std::find_if(calls.begin(), calls.end(),
                                        [&](const CallInfo &call) { return call.id == leg; });
        if (found == calls.end()) {
            continue;
        }

        const ConsentPlan theirs = policyFor(found->accountId).consent;
        if (first) {
            plan = theirs;
            first = false;
            continue;
        }
        if (strictness(theirs.posture) > strictness(plan.posture)) {
            plan.posture = theirs.posture;
        }
    }

    return plan;
}

void RecordingController::formConference(const std::vector<CallInfo> &calls)
{
    const auto group = onTheAudio(calls);

    if (group == lastGroup_) {
        return;
    }
    lastGroup_ = group;

    if (group.size() < 2) {
        return;
    }

    for (const auto &session : sessions_) {
        if (!session->conference) {
            continue;
        }
        if (std::any_of(session->subject.begin(), session->subject.end(),
                        [&](const std::string &leg) { return holds(group, leg); })) {
            return;
        }
    }

    const auto recorded = std::find_if(group.begin(), group.end(), [this](const std::string &leg) {
        return std::any_of(
            sessions_.begin(), sessions_.end(),
            [&](const std::unique_ptr<Session> &session) { return holds(session->subject, leg); });
    });
    if (recorded == group.end()) {
        return;
    }

    if (const auto status = start(*recorded, calls); !succeeded(status)) {
        note(LogLevel::Warning, *recorded,
             "a conference was assembled around a recorded call and the "
             "recording of the room would not start: " +
                 std::string{toString(status)});
    }
}

void RecordingController::considerAnswered(const std::vector<CallInfo> &calls)
{

    for (auto seen = considered_.begin(); seen != considered_.end();) {
        const bool alive = std::any_of(calls.begin(), calls.end(), [&](const CallInfo &call) {
            return call.id == *seen && call.state != CallState::Ended;
        });
        seen = alive ? std::next(seen) : considered_.erase(seen);
    }

    for (const CallInfo &call : calls) {

        if (call.state != CallState::Active) {
            continue;
        }
        if (!considered_.insert(call.id).second) {
            continue;
        }

        if (sessionOf(call.id) != nullptr) {
            continue;
        }

        if (call.holdsTheAudio()) {
            const auto room = onTheAudio(calls);
            const bool recorded =
                std::any_of(room.begin(), room.end(), [&](const std::string &leg) {
                    return leg != call.id && sessionOf(leg) != nullptr;
                });
            if (recorded) {
                continue;
            }
        }

        switch (policyFor(call.accountId).when) {
        case WhenToRecord::Always:
            if (const auto status = start(call.id, calls); !succeeded(status)) {
                note(LogLevel::Warning, call.id,
                     "this line records every call and this one would not start: " +
                         std::string{toString(status)});
            }
            break;

        case WhenToRecord::Ask:
            if (ask_) {
                ask_(call.id);
            }
            break;

        case WhenToRecord::Never:
            break;
        }
    }
}

void RecordingController::obtainConsent(Session &session)
{

    if (session.asked || !session.live) {
        return;
    }

    session.asked = true;

    session.told.insert(session.subject.begin(), session.subject.end());

    if (voice_ == nullptr) {

        session.announced = true;
        apply(session);
        return;
    }

    const std::string recordingId = session.row.id;

    switch (session.consent.posture) {
    case ConsentPosture::Announce:

        voice_->announceTo(
            recordingId, session.subject,
            {session.consent.announcement, session.consent.shippedAnnouncement},
            session.consent.keepInTheRecording, [this, recordingId](NoticeProgress progress) {
                Session *told = sessionById(recordingId);
                if (told == nullptr || !told->recorder) {
                    return;
                }

                switch (progress) {
                case NoticeProgress::Began:

                    told->announcing = true;
                    apply(*told);
                    break;

                case NoticeProgress::Given:
                    told->announcing = false;
                    told->announced = true;
                    told->row.consentAt = std::chrono::system_clock::now();
                    apply(*told);
                    break;

                case NoticeProgress::Failed:

                    told->asked = false;
                    told->told.clear();
                    note(LogLevel::Warning, naming(told->subject),
                         "the recording notice could not be given, so nothing is being kept: it "
                         "is tried again when the conversation next moves");
                    break;
                }
            });
        break;

    case ConsentPosture::Tone:
        session.row.consentAt = std::chrono::system_clock::now();
        voice_->beepEvery(recordingId, session.subject, session.consent.tone,
                          session.consent.interval, session.consent.keepInTheRecording);
        break;

    case ConsentPosture::Confirm:
    case ConsentPosture::Silent:

        break;
    }
}

void RecordingController::tellNewcomers(Session &session)
{
    if (voice_ == nullptr || session.consent.posture != ConsentPosture::Announce) {
        return;
    }

    if (!session.announced || !session.live) {
        return;
    }

    std::vector<std::string> fresh;
    for (const std::string &leg : session.subject) {
        if (session.told.count(leg) == 0) {
            fresh.push_back(leg);
        }
    }
    if (fresh.empty()) {
        return;
    }

    session.told.insert(fresh.begin(), fresh.end());

    const std::string recordingId = session.row.id;
    voice_->announceTo(
        recordingId, fresh, {session.consent.announcement, session.consent.shippedAnnouncement},
        session.consent.keepInTheRecording, [this, recordingId, fresh](NoticeProgress progress) {
            if (progress != NoticeProgress::Failed) {

                return;
            }

            Session *room = sessionById(recordingId);
            if (room == nullptr) {
                return;
            }
            for (const std::string &leg : fresh) {
                room->told.erase(leg);
            }
            note(LogLevel::Warning, naming(room->subject),
                 "somebody joined a recorded conference and could not be told about it yet: it "
                 "is tried again when the conversation next moves");
        });
}

void RecordingController::setPaused(const std::string &callId, bool paused)
{
    Session *session = sessionOf(callId);
    if (session == nullptr) {
        return;
    }

    session->paused = paused;

    if (session->conference) {
        const std::vector<std::string> room = session->subject;
        for (const auto &other : sessions_) {
            if (other.get() == session || other->conference || other->subject.empty()) {
                continue;
            }
            if (holds(room, other->subject.front())) {
                other->paused = paused;
                apply(*other);
            }
        }
    }

    apply(*session);
}

bool RecordingController::wantsStems(const Session &session) noexcept
{

    return session.keepStems && session.conference && session.subject.size() > 1;
}

void RecordingController::apply(Session &session)
{
    taps_.recordCalls(session.row.id, session.subject, wantsStems(session));

    if (voice_ != nullptr && session.consent.posture == ConsentPosture::Tone &&
        !session.subject.empty()) {
        voice_->beepEvery(session.row.id, session.subject, session.consent.tone,
                          session.consent.interval, session.consent.keepInTheRecording);
    }

    if (voice_ != nullptr && session.asked && !session.announced && !session.live &&
        session.consent.posture == ConsentPosture::Announce) {
        voice_->stopAnnouncing(session.row.id);
        session.asked = false;
        session.announcing = false;
    }

    obtainConsent(session);

    tellNewcomers(session);

    const bool told = session.announced || session.announcing;

    const bool keeping = session.live && !session.paused && told;
    if (keeping != session.keeping) {
        session.keeping = keeping;
    }

    const bool far = keeping;
    if (far != session.kept) {
        session.kept = far;
        session.recorder->keepFarSide(far);
    }

    const bool near = session.live && !session.paused && session.speaking && told;
    if (near != session.spoke) {
        session.spoke = near;
        session.recorder->keepNearSide(near);
    }

    publish();
}

RecordingController::Finished
RecordingController::close(std::vector<std::unique_ptr<Session>>::iterator where)
{

    std::unique_ptr<Session> held = std::move(*where);
    sessions_.erase(where);

    Session &session = *held;
    Recording &row = session.row;

    if (voice_ != nullptr) {
        voice_->stopAnnouncing(row.id);
    }

    taps_.recordCalls(row.id, {}, false);

    const auto outcome = session.recorder->stop();
    session.recorder.reset();

    taps_.closeRecording(row.id);

    row.duration = outcome.duration;
    row.bytes = outcome.bytes;
    row.hasGaps = outcome.hasGaps();
    row.partyCount = static_cast<unsigned>(row.legs.size());

    row.stems.clear();
    for (const StemOutcome &written : outcome.stems) {
        RecordingStem stem;
        stem.callId = written.callId;
        stem.filePath = relativePath(row.id, row.startedAt, written.callId);
        stem.offset = written.offset;
        stem.duration = written.duration;
        stem.bytes = written.bytes;

        const auto durable = session.durable.find(written.callId);
        const auto &named = durable != session.durable.end() ? durable->second : written.callId;
        const auto leg = std::find_if(row.legs.begin(), row.legs.end(),
                                      [&](const RecordingLeg &had) { return had.callId == named; });
        if (leg != row.legs.end()) {
            stem.remoteNumber = leg->remoteNumber;
            stem.remoteName = leg->remoteName;
        }

        row.stems.push_back(std::move(stem));
    }

    for (RecordingLeg &leg : row.legs) {
        if (const auto found = session.durable.find(leg.callId); found != session.durable.end()) {
            leg.callId = found->second;
        }
    }
    for (RecordingStem &stem : row.stems) {
        if (const auto found = session.durable.find(stem.callId); found != session.durable.end()) {
            stem.callId = found->second;
        }
    }

    row.unfinished = false;

    bool kept = false;
    if (store_ != nullptr) {

        Recording stored;
        if (succeeded(store_->recording(row.id, stored))) {
            keepWhatAPersonChanged(stored, row);
        }

        if (const auto status = store_->save(row); !succeeded(status)) {
            note(LogLevel::Error, naming(session.subject),
                 "the recording was made and the row was not written: " +
                     std::string{toString(status)});
        } else {
            kept = true;
        }
    }

    publish();

    if (kept && written_) {
        written_(row);
    }

    return Finished{outcome, row};
}

RecordingController::Finished RecordingController::stop(const std::string &callId)
{

    const Session *wanted = sessionOf(callId);
    if (wanted == nullptr) {
        return Finished{};
    }

    const auto where = std::find_if(
        sessions_.begin(), sessions_.end(),
        [&](const std::unique_ptr<Session> &session) { return session.get() == wanted; });
    if (where == sessions_.end()) {
        return Finished{};
    }
    return close(where);
}

void RecordingController::stopAll()
{
    while (!sessions_.empty()) {
        static_cast<void>(close(sessions_.begin()));
    }
}

}
