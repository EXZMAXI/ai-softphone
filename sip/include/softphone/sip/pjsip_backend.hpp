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

#include <softphone/core/account_config.hpp>
#include <softphone/core/audio_devices.hpp>
#include <softphone/core/audio_signal.hpp>
#include <softphone/core/busy_lamp.hpp>
#include <softphone/core/call_backend.hpp>
#include <softphone/core/codecs.hpp>
#include <softphone/core/credentials.hpp>
#include <softphone/core/event_loop.hpp>
#include <softphone/core/level_meter.hpp>
#include <softphone/core/playback.hpp>
#include <softphone/core/recording_consent.hpp>
#include <softphone/core/recording_taps.hpp>
#include <softphone/core/sip_backend.hpp>
#include <softphone/core/sip_trace.hpp>
#include <softphone/core/tones.hpp>

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace softphone::sip {

void setStackLogLevel(int pjsipLevel) noexcept;

struct PjsipOptions {

    std::uint16_t localPort{0};

    int logLevel{0};

    bool useNullAudioDevice{false};

    bool sendsWhileSilent{false};

    std::string certificateAuthority;
};

[[nodiscard]] std::string_view tlsImplementation() noexcept;

using AudioLevels = core::AudioLevels;

class PjsipBackend final : public core::ISipBackend,
                           public core::ICallBackend,
                           public core::IAudioDeviceBackend,
                           public core::ICodecBackend,
                           public core::ISignalTaps,
                           public core::IRecordingTaps,
                           public core::IConsentVoice,
                           public core::IPlaybackDevice,
                           public core::ILineWatchBackend {
public:
    using Options = PjsipOptions;

    PjsipBackend(core::EventLoop &loop, const core::ICredentialProvider &credentials,
                 Options options = Options{});

    PjsipBackend(const PjsipBackend &) = delete;
    PjsipBackend &operator=(const PjsipBackend &) = delete;
    PjsipBackend(PjsipBackend &&) = delete;
    PjsipBackend &operator=(PjsipBackend &&) = delete;
    ~PjsipBackend() override;

    void start();

    void stop();

    [[nodiscard]] bool running() const noexcept;

    [[nodiscard]] std::uint16_t boundPort() const noexcept;

    void setTraceSink(core::ISipTraceSink *sink) noexcept;

    using SwitchNamed = std::function<void(std::string)>;
    void setSwitchListener(SwitchNamed told);

    void setObserver(core::ISipBackend::Observer *observer) override;
    void requestRegistration(const core::AccountConfig &config) override;
    void requestUnregistration(const std::string &accountId) override;

    void forgetAccount(const std::string &accountId) override;
    void addressesChanged() override;

    void setCallObserver(core::ICallBackend::Observer *observer) override;

    [[nodiscard]] core::ICallBackend::Observer *callObserver() const noexcept;
    void placeCall(const std::string &callId, const std::string &accountId,
                   const std::string &target) override;
    void noteSeance(const std::string &callId, const std::string &seanceId) override;
    void answerCall(const std::string &callId) override;
    void rejectCall(const std::string &callId, int sipStatus) override;
    void refuseIncomingCalls(bool refusing) override;
    void refuseSecondCalls(bool refusing) override;
    void hangUpCall(const std::string &callId) override;
    void setCallMuted(const std::string &callId, bool muted) override;

    void holdCall(const std::string &callId) override;
    void resumeCall(const std::string &callId) override;

    void routeAudioTo(const std::vector<std::string> &callIds) override;

    void reconnectAudio() override;

    void sendDtmf(const std::string &callId, char digit, core::DtmfTransport transport) override;

    void setLineWatchObserver(core::ILineWatchBackend::Observer *observer) override;
    void watchLine(const std::string &watchId, const std::string &accountId,
                   const std::string &number) override;
    void stopWatching(const std::string &watchId) override;

    void transferCall(const std::string &callId, const std::string &target) override;
    void transferCallToCall(const std::string &callId,
                            const std::string &consultationCallId) override;
    [[nodiscard]] std::optional<core::CallMediaStats>
    callMediaStats(const std::string &callId) const override;

    [[nodiscard]] std::vector<core::AudioDeviceInfo> devices() override;

    core::AudioApplyResult applyAudio(const core::AudioSettings &audio) override;

    void applyVolumes(const core::Volumes &volumes) override;

    void setDeviceLostCallback(core::IAudioDeviceBackend::DeviceLostCallback callback) override;

    void
    setDevicesChangedCallback(core::IAudioDeviceBackend::DevicesChangedCallback callback) override;

    [[nodiscard]] std::string systemDefaultDevice(core::DeviceRole role) const override;

    void startTone(const std::string &wavPath, bool loop,
                   core::ToneOutput output = core::ToneOutput::Speakers,
                   core::ToneLevel level = core::ToneLevel::Notifications);

    void setToneEndedCallback(std::function<void()> callback);

    using SoundDeviceWanted = std::function<void(bool)>;
    void setSoundDeviceWantedCallback(SoundDeviceWanted callback);

    void stopTone();

    [[nodiscard]] bool ringerOpen() const noexcept;

    [[nodiscard]] std::vector<core::CodecInfo> codecs() override;

    void applyCodecs(std::span<const core::CodecChoice> wanted) override;

    void startMetering() override;

    void stopMetering() override;

    [[nodiscard]] bool metering() const noexcept;

    [[nodiscard]] std::optional<AudioLevels> audioLevels() const override;

    [[nodiscard]] std::optional<float> microphoneLevelOnThePort() const;

    void watchSignals(const std::string &callId) override;

    [[nodiscard]] core::CallSignals &feed() noexcept override;

    void recordCalls(const std::string &recordingId, const std::vector<std::string> &callIds,
                     bool separately) override;

    [[nodiscard]] core::RecordingFeed &openRecording(const std::string &recordingId) override;

    void closeRecording(const std::string &recordingId) override;

    [[nodiscard]] core::RecordingFeed *recordingRings(const std::string &recordingId) const;

    void announceTo(const std::string &noticeId, const std::vector<std::string> &callIds,
                    const std::vector<std::string> &files, bool intoTheRecording,
                    std::function<void(core::NoticeProgress)> report) override;
    void beepEvery(const std::string &noticeId, const std::vector<std::string> &callIds,
                   const std::string &file, std::chrono::seconds interval,
                   bool intoTheRecording) override;
    void stopAnnouncing(const std::string &noticeId) override;

    void playFrom(core::IPlaybackSource *source) override;

    [[nodiscard]] unsigned playbackRate() const noexcept override;

    struct Impl;

private:
    SwitchNamed switchNamed_;

    void replayTone();

    void rearmBeep(const std::string &noticeId);

    void endCallsStrandedBy(const std::vector<std::string> &dialogs);

    class Account;
    class Call;
    class WatchedLine;
    class Endpoint;

    std::unique_ptr<Impl> impl_;
};

}
