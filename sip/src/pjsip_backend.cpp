// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/sip/pjsip_backend.hpp>

#include <softphone/sip/dialog_info.hpp>
#include <softphone/sip/system_default_device.hpp>

#if defined(SOFTPHONE_HAS_PULSE)
#include <softphone/sip/pulse_audio_device.hpp>

extern "C" pjmedia_aud_dev_factory *pjmedia_alsa_factory(pj_pool_factory *pf);
#endif

#include "canceller_starvation.hpp"
#include "device_descriptions.hpp"
#include "tls_roots.hpp"

#include <softphone/core/auto_answer.hpp>
#include <softphone/core/busy_lamp.hpp>
#include <softphone/core/call.hpp>
#include <softphone/core/identity.hpp>
#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>
#include <softphone/core/version.hpp>

#include <pjsua2.hpp>

#include <pj/pool_buf.h>
#include <pjlib-util/xml.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace softphone::sip {
namespace {

[[nodiscard]] bool mentionsResolverFailure(const std::string &reason)
{
    return reason.find("PJ_ERESOLVE") != std::string::npos;
}

[[nodiscard]] bool mentionsAudioDeviceFailure(const std::string &reason)
{
    return reason.find("PJMEDIA_EAUD_") != std::string::npos;
}

[[nodiscard]] core::LocalFailure localFailureIn(const std::string &reason)
{
    return mentionsAudioDeviceFailure(reason) ? core::LocalFailure::NoAudioDevice
                                              : core::LocalFailure::None;
}

[[nodiscard]] core::TransportFailure transportFailureFor(int code, const std::string &reason)
{

    if (reason.find("PJSIP_TLS_ECERTVERIF") != std::string::npos) {
        return core::TransportFailure::CertificateRejected;
    }

    if (mentionsResolverFailure(reason)) {
        return core::TransportFailure::DnsFailure;
    }

    if (code == PJSIP_SC_REQUEST_TIMEOUT) {
        return core::TransportFailure::Timeout;
    }

    if (code == PJSIP_SC_BAD_GATEWAY) {
        return core::TransportFailure::ConnectionFailed;
    }

    if (code == 0) {
        return core::TransportFailure::ConnectionFailed;
    }

    return core::TransportFailure::None;
}

[[nodiscard]] core::RegistrationOutcome outcomeFrom(const pj::OnRegStateParam &prm)
{
    core::RegistrationOutcome outcome;
    outcome.sipStatus = static_cast<int>(prm.code);
    outcome.reason = prm.reason;

    outcome.server = core::headerInMessage(prm.rdata.wholeMsg, "server:");
    if (outcome.server.empty()) {
        outcome.server = core::headerInMessage(prm.rdata.wholeMsg, "user-agent:");
    }

    outcome.registered = prm.code >= 200 && prm.code < 300 && prm.expiration > 0;

    if (!outcome.registered) {
        outcome.transportFailure = transportFailureFor(static_cast<int>(prm.code), outcome.reason);
    }

    return outcome;
}

[[nodiscard]] std::uint16_t portFrom(const std::string &hostPort)
{
    const auto colon = hostPort.rfind(':');
    if (colon == std::string::npos || colon + 1 >= hostPort.size()) {
        return 0;
    }

    unsigned port = 0;
    const char *first = hostPort.data() + colon + 1;
    const char *last = hostPort.data() + hostPort.size();
    if (std::from_chars(first, last, port).ec != std::errc{}) {
        return 0;
    }

    return static_cast<std::uint16_t>(port);
}

[[nodiscard]] core::LogLevel logLevelFor(int pjsipLevel) noexcept
{
    switch (pjsipLevel) {
    case 0:
    case 1:
        return core::LogLevel::Error;
    case 2:
        return core::LogLevel::Warning;
    case 3:
        return core::LogLevel::Info;
    default:
        break;
    }

    return core::LogLevel::Trace;
}

[[nodiscard]] std::string asLogText(std::string_view raw)
{
    while (!raw.empty() && (raw.back() == '\n' || raw.back() == '\r')) {
        raw.remove_suffix(1);
    }

    std::string text;
    text.reserve(raw.size());
    for (const char letter : raw) {
        if (letter != '\r') {
            text.push_back(letter);
        }
    }

    return core::shortenBuildPaths(asText(std::move(text)));
}

extern "C" void writePjsipLog(int level, const char *data, int length)
{
    if (data == nullptr || length <= 0) {
        return;
    }

    auto text = asLogText({data, static_cast<std::size_t>(length)});
    if (CancellerStarvation::counted(text)) {
        return;
    }
    core::Logger::instance().log(logLevelFor(level), "pjsip", text);
}

class PjsipLogWriter final : public pj::LogWriter {
public:
    void write(const pj::LogEntry &entry) override
    {
        auto text = asLogText(entry.msg);
        if (CancellerStarvation::counted(text)) {
            return;
        }
        core::Logger::instance().log(logLevelFor(entry.level), "pjsip", text);
    }
};

void noteCall(core::LogLevel level, std::string_view callId, const std::string &what)
{
    const core::LogAttribute fields[] = {{"call", callId}};
    core::Logger::instance().log(level, "sip", what, fields);
}

void noteCall(core::LogLevel level, std::string_view callId, std::string_view line,
              const std::string &what)
{
    const core::LogAttribute fields[] = {{"call", callId}, {"line", line}};
    core::Logger::instance().log(level, "sip", what, fields);
}

[[nodiscard]] core::BackendCallState callStateFor(pjsip_inv_state state) noexcept
{
    switch (state) {
    case PJSIP_INV_STATE_CALLING:
    case PJSIP_INV_STATE_INCOMING:
        return core::BackendCallState::Calling;
    case PJSIP_INV_STATE_EARLY:
    case PJSIP_INV_STATE_CONNECTING:
        return core::BackendCallState::Ringing;
    case PJSIP_INV_STATE_CONFIRMED:
        return core::BackendCallState::Connected;
    case PJSIP_INV_STATE_NULL:
    case PJSIP_INV_STATE_DISCONNECTED:
        break;
    }
    return core::BackendCallState::Disconnected;
}

[[nodiscard]] std::string registrarUriFor(const core::AccountConfig &config)
{
    auto uri = "sip:" + config.effectiveRegistrar() + ':' + std::to_string(config.port);
    if (config.transport != core::SipTransport::Udp) {
        uri += ";transport=" + std::string{core::toString(config.transport)};
    }
    return uri;
}

[[nodiscard]] std::vector<std::string> addressesOf(const std::string &host)
{
    if (host.empty()) {
        return {};
    }

    std::vector<std::string> addresses{host};

    unsigned count = 8;
    std::array<pj_addrinfo, 8> found{};
    auto name = pj_str(const_cast<char *>(host.c_str()));
    if (pj_getaddrinfo(pj_AF_UNSPEC(), &name, &count, found.data()) != PJ_SUCCESS) {

        return addresses;
    }

    for (unsigned at = 0; at < count; ++at) {
        std::array<char, PJ_INET6_ADDRSTRLEN> text{};
        if (pj_sockaddr_print(&found.at(at).ai_addr, text.data(), static_cast<int>(text.size()),
                              0) != nullptr) {
            addresses.emplace_back(text.data());
        }
    }

    std::sort(addresses.begin(), addresses.end());
    addresses.erase(std::unique(addresses.begin(), addresses.end()), addresses.end());
    return addresses;
}

[[nodiscard]] std::vector<std::string> headerValues(const pjsip_msg *msg, const char *name)
{
    std::vector<std::string> values;

    auto hname = pj_str(const_cast<char *>(name));
    const void *from = nullptr;
    while (true) {
        const auto *header = static_cast<const pjsip_hdr *>(pjsip_msg_find_hdr_by_name(
            const_cast<pjsip_msg *>(msg), &hname, const_cast<void *>(from)));
        if (header == nullptr) {
            break;
        }

        std::array<char, 512> buffer{};
        const auto length =
            pjsip_hdr_print_on(const_cast<pjsip_hdr *>(header), buffer.data(), buffer.size() - 1);
        if (length > 0) {
            const std::string printed{buffer.data(), static_cast<std::size_t>(length)};
            if (const auto colon = printed.find(':'); colon != std::string::npos) {
                auto value = printed.substr(colon + 1);
                if (const auto first = value.find_first_not_of(" \t"); first != std::string::npos) {
                    values.push_back(value.substr(first));
                }
            }
        }

        if (header->next == nullptr || header->next == &msg->hdr) {
            break;
        }
        from = header->next;
    }

    return values;
}

constexpr const char *kSeanceHeader = "X-Seance-Id";

[[nodiscard]] std::string seanceIn(const pjsip_msg *msg)
{
    const auto values = headerValues(msg, kSeanceHeader);
    return values.empty() ? std::string{} : values.front();
}

void addSeanceHeader(pj::CallOpParam &options, const std::string &seance)
{
    if (seance.empty()) {
        return;
    }
    pj::SipHeader header;
    header.hName = kSeanceHeader;
    header.hValue = seance;
    options.txOption.headers.push_back(header);
}

[[nodiscard]] std::string withSeance(const std::string &target, const std::string &seance)
{
    if (seance.empty() || target.empty()) {
        return target;
    }
    auto uri = target;
    if (uri.size() >= 2 && uri.front() == '<' && uri.back() == '>') {
        uri = uri.substr(1, uri.size() - 2);
    }
    uri += uri.find('?') == std::string::npos ? '?' : '&';
    uri += std::string{kSeanceHeader} + '=' + seance;
    return '<' + uri + '>';
}

[[nodiscard]] pj::CallOpParam audioOnlyOptions()
{
    pj::CallOpParam options{true};
    options.opt.textCount = 0;
    return options;
}

}

void setStackLogLevel(int pjsipLevel) noexcept
{

    pj_log_set_level(pjsipLevel);
}

namespace {

std::atomic<core::ISipTraceSink *> traceSink{nullptr};

std::atomic<std::function<void(std::string)> *> switchNamed{nullptr};

[[nodiscard]] std::string parsedHeader(const pjsip_msg *msg, const char *name)
{
    if (msg == nullptr) {
        return {};
    }
    pj_str_t wanted = pj_str(const_cast<char *>(name));
    const auto *found = static_cast<const pjsip_generic_string_hdr *>(
        pjsip_msg_find_hdr_by_name(msg, &wanted, nullptr));
    if (found == nullptr) {
        return {};
    }
    return std::string{found->hvalue.ptr, static_cast<std::size_t>(found->hvalue.slen)};
}

[[nodiscard]] std::string text(const pj_str_t &value)
{
    return value.ptr == nullptr ? std::string{}
                                : std::string{value.ptr, static_cast<std::size_t>(value.slen)};
}

[[nodiscard]] std::string summarise(pjsip_msg *message)
{
    if (message == nullptr) {
        return {};
    }

    if (message->type == PJSIP_REQUEST_MSG) {
        std::array<char, 256> uri{};
        const auto printed = pjsip_uri_print(PJSIP_URI_IN_REQ_URI, message->line.req.uri,
                                             uri.data(), uri.size() - 1);
        auto summary = text(message->line.req.method.name);
        if (printed > 0) {
            summary += ' ';
            summary.append(uri.data(), static_cast<std::size_t>(printed));
        }
        return summary;
    }

    auto summary =
        std::to_string(message->line.status.code) + ' ' + text(message->line.status.reason);
    if (const auto *cseq =
            static_cast<const pjsip_cseq_hdr *>(pjsip_msg_find_hdr(message, PJSIP_H_CSEQ, nullptr));
        cseq != nullptr) {
        summary += " (" + text(cseq->method.name) + ')';
    }
    return summary;
}

[[nodiscard]] std::string callIdOf(pjsip_msg *message)
{
    if (message == nullptr) {
        return {};
    }
    const auto *header =
        static_cast<const pjsip_cid_hdr *>(pjsip_msg_find_hdr(message, PJSIP_H_CALL_ID, nullptr));
    return header == nullptr ? std::string{} : text(header->id);
}

[[nodiscard]] std::string address(const pj_str_t &host, int port)
{
    return text(host) + ':' + std::to_string(port);
}

class DialogTransports {
public:
    static void noted(const std::string &callId, const void *transport)
    {
        const std::lock_guard<std::mutex> held{lock()};
        where()[callId] = transport;
    }

    static void forget(const std::string &callId)
    {
        const std::lock_guard<std::mutex> held{lock()};
        where().erase(callId);
    }

    [[nodiscard]] static std::vector<std::string> strandedBy(const void *transport)
    {
        std::vector<std::string> stranded;
        const std::lock_guard<std::mutex> held{lock()};
        for (auto it = where().begin(); it != where().end();) {
            if (it->second == transport) {
                stranded.push_back(it->first);
                remember(it->first);
                it = where().erase(it);
            } else {
                ++it;
            }
        }
        return stranded;
    }

    [[nodiscard]] static bool wasStranded(const std::string &callId)
    {
        const std::lock_guard<std::mutex> held{lock()};
        return marooned().erase(callId) > 0;
    }

    static void forgetEverything()
    {
        const std::lock_guard<std::mutex> held{lock()};
        where().clear();
        marooned().clear();
        order().clear();
    }

private:

    static std::mutex &lock()
    {
        static std::mutex one;
        return one;
    }

    static std::map<std::string, const void *> &where()
    {
        static std::map<std::string, const void *> all;
        return all;
    }

    static std::set<std::string> &marooned()
    {
        static std::set<std::string> all;
        return all;
    }

    static std::deque<std::string> &order()
    {
        static std::deque<std::string> all;
        return all;
    }

    static void remember(const std::string &callId)
    {
        if (marooned().insert(callId).second) {
            order().push_back(callId);
        }

        while (order().size() > 64) {
            marooned().erase(order().front());
            order().pop_front();
        }
    }
};

void noteDialogTransport(pjsip_tx_data *tdata)
{
    if (tdata == nullptr || tdata->msg == nullptr || tdata->tp_info.transport == nullptr) {
        return;
    }

    if (tdata->msg->type != PJSIP_REQUEST_MSG ||
        tdata->msg->line.req.method.id != PJSIP_INVITE_METHOD) {
        return;
    }

    const auto callId = callIdOf(tdata->msg);
    if (!callId.empty()) {
        DialogTransports::noted(callId, tdata->tp_info.transport);
    }
}

pj_bool_t onSipMessageReceived(pjsip_rx_data *rdata)
{

    if (auto *named = switchNamed.load(std::memory_order_acquire);
        named != nullptr && rdata != nullptr && rdata->msg_info.msg != nullptr &&
        rdata->msg_info.msg->type == PJSIP_RESPONSE_MSG) {
        if (auto said = parsedHeader(rdata->msg_info.msg, "Server"); !said.empty()) {
            (*named)(std::move(said));
        }
    }

    auto *sink = traceSink.load(std::memory_order_acquire);
    if (sink == nullptr || rdata == nullptr) {

        return PJ_FALSE;
    }

    core::SipMessage message;
    message.at = std::chrono::system_clock::now();
    message.direction = core::SipDirection::Incoming;
    message.transport = rdata->tp_info.transport == nullptr
                            ? std::string{}
                            : std::string{rdata->tp_info.transport->type_name};
    message.localAddress = rdata->tp_info.transport == nullptr
                               ? std::string{}
                               : address(rdata->tp_info.transport->local_name.host,
                                         rdata->tp_info.transport->local_name.port);
    message.remoteAddress =
        std::string{rdata->pkt_info.src_name} + ':' + std::to_string(rdata->pkt_info.src_port);
    message.summary = summarise(rdata->msg_info.msg);
    message.callId = callIdOf(rdata->msg_info.msg);
    message.text = core::redact(
        std::string_view{rdata->msg_info.msg_buf, static_cast<std::size_t>(rdata->msg_info.len)});

    sink->onSipMessage(std::move(message));

    return PJ_FALSE;
}

pj_status_t onSipMessageSent(pjsip_tx_data *tdata)
{
    noteDialogTransport(tdata);

    auto *sink = traceSink.load(std::memory_order_acquire);
    if (sink == nullptr || tdata == nullptr) {
        return PJ_SUCCESS;
    }

    core::SipMessage message;
    message.at = std::chrono::system_clock::now();
    message.direction = core::SipDirection::Outgoing;
    message.transport = tdata->tp_info.transport == nullptr
                            ? std::string{}
                            : std::string{tdata->tp_info.transport->type_name};
    message.localAddress = tdata->tp_info.transport == nullptr
                               ? std::string{}
                               : address(tdata->tp_info.transport->local_name.host,
                                         tdata->tp_info.transport->local_name.port);

    message.remoteAddress =
        tdata->tp_info.dst_name[0] == '\0'
            ? std::string{}
            : std::string{tdata->tp_info.dst_name} + ':' + std::to_string(tdata->tp_info.dst_port);
    message.summary = summarise(tdata->msg);
    message.callId = callIdOf(tdata->msg);

    if (tdata->buf.start != nullptr && tdata->buf.cur > tdata->buf.start) {
        message.text = core::redact(std::string_view{
            tdata->buf.start, static_cast<std::size_t>(tdata->buf.cur - tdata->buf.start)});
    }

    sink->onSipMessage(std::move(message));
    return PJ_SUCCESS;
}

pjsip_module traceModule = {
    nullptr,
    nullptr,
    {const_cast<char *>("mod-softphone-trace"), 19},
    -1,
    PJSIP_MOD_PRIORITY_TRANSPORT_LAYER - 1,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    &onSipMessageReceived,
    &onSipMessageReceived,
    &onSipMessageSent,
    &onSipMessageSent,
    nullptr,
};

pj_status_t onSipResponseSent(pjsip_tx_data *tdata)
{
    if (tdata == nullptr || tdata->msg == nullptr) {
        return PJ_SUCCESS;
    }

    pj_str_t name = {const_cast<char *>("Server"), 6};

    if (pjsip_msg_find_hdr_by_name(tdata->msg, &name, nullptr) != nullptr) {
        return PJ_SUCCESS;
    }

    const auto agent = core::userAgent();
    pj_str_t value = {const_cast<char *>(agent.data()), static_cast<pj_ssize_t>(agent.size())};

    auto *header = pjsip_generic_string_hdr_create(tdata->pool, &name, &value);
    if (header != nullptr) {
        pjsip_msg_add_hdr(tdata->msg, reinterpret_cast<pjsip_hdr *>(header));
    }

    return PJ_SUCCESS;
}

pjsip_module identityModule = {
    nullptr,
    nullptr,
    {const_cast<char *>("mod-softphone-identity"), 22},
    -1,
    PJSIP_MOD_PRIORITY_APPLICATION,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    &onSipResponseSent,
    nullptr,
};

void markSdpAsOurs(pjmedia_sdp_session *sdp)
{
    if (sdp == nullptr) {
        return;
    }

    const auto name = core::productName();
    sdp->name.ptr = const_cast<char *>(name.data());
    sdp->name.slen = static_cast<pj_ssize_t>(name.size());

    static pjmedia_sdp_attr tool = [] {
        const auto agent = core::userAgent();
        pjmedia_sdp_attr attribute{};
        attribute.name.ptr = const_cast<char *>("tool");
        attribute.name.slen = 4;
        attribute.value.ptr = const_cast<char *>(agent.data());
        attribute.value.slen = static_cast<pj_ssize_t>(agent.size());
        return attribute;
    }();

    if (pjmedia_sdp_attr_find(sdp->attr_count, sdp->attr, &tool.name, nullptr) == nullptr) {
        static_cast<void>(pjmedia_sdp_attr_add(&sdp->attr_count, sdp->attr, &tool));
    }
}

}

class PjsipBackend::Endpoint final : public pj::Endpoint {
public:

    std::function<void(core::DeviceRole)> onDeviceLost;

    std::function<void(const std::string &, const std::string &, std::vector<std::string>)>
        onTransportGone;

    std::function<int(pjsip_rx_data *, int)> chooseTheLine;

    void onSelectAccount(pj::OnSelectAccountParam &prm) override
    {
        auto *rdata = static_cast<pjsip_rx_data *>(prm.rdata.pjRxData);
        if (!chooseTheLine || rdata == nullptr) {
            return;
        }

        prm.accountIndex = chooseTheLine(rdata, prm.accountIndex);
    }

    void onMediaEvent(pj::OnMediaEventParam &prm) override
    {
        if (prm.ev.type != PJMEDIA_EVENT_AUD_DEV_ERROR || !onDeviceLost) {
            return;
        }

        onDeviceLost(prm.ev.data.audDevError.dir == PJMEDIA_DIR_CAPTURE
                         ? core::DeviceRole::Capture
                         : core::DeviceRole::Playback);
    }

    void onTransportState(const pj::OnTransportStateParam &prm) override
    {

        if (prm.state == PJSIP_TP_STATE_DISCONNECTED && prm.lastError == PJSIP_TLS_ECERTVERIF) {
            std::string why;
            for (const auto &message : prm.tlsInfo.verifyMsgs) {
                why += (why.empty() ? "" : "; ") + message;
            }
            core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                         "the switch's certificate was refused: " +
                                             (why.empty() ? std::string{"no reason given"} : why));
        }

        if (prm.state != PJSIP_TP_STATE_DISCONNECTED || !onTransportGone) {
            return;
        }

        std::string farEnd;
        if (prm.hnd != nullptr) {
            const auto *transport = static_cast<const pjsip_transport *>(prm.hnd);
            farEnd = std::string{transport->remote_name.host.ptr,
                                 static_cast<std::size_t>(transport->remote_name.host.slen)} +
                     ':' + std::to_string(transport->remote_name.port);
        }
        onTransportGone(prm.type, farEnd, DialogTransports::strandedBy(prm.hnd));
    }
};

void raiseIfFailed(pj_status_t status, const std::string &operation)
{
    if (status != PJ_SUCCESS) {
        throw pj::Error{status, operation, std::string{}, __FILE__, __LINE__};
    }
}

class RingerDevice final : public pj::ExtraAudioDevice {
public:
    explicit RingerDevice(int playbackDevice)
        : pj::ExtraAudioDevice(playbackDevice, PJMEDIA_AUD_INVALID_DEV)
    {}

    void openForPlayback()
    {
        if (isOpened()) {
            return;
        }

        pjsua_conf_port_info master{};
        raiseIfFailed(pjsua_conf_get_port_info(0, &master), "pjsua_conf_get_port_info");

        pjmedia_snd_port_param param;
        pjmedia_snd_port_param_default(&param);
        raiseIfFailed(pjmedia_aud_dev_default_param(playDev, &param.base),
                      "pjmedia_aud_dev_default_param");

        param.base.dir = PJMEDIA_DIR_PLAYBACK;
        param.base.play_id = playDev;
        param.base.clock_rate = master.clock_rate;
        param.base.channel_count = 1;
        param.base.samples_per_frame = master.samples_per_frame / master.channel_count;
        param.base.bits_per_sample = master.bits_per_sample;

        pjsua_ext_snd_dev *device = nullptr;
        raiseIfFailed(pjsua_ext_snd_dev_create(&param, &device), "pjsua_ext_snd_dev_create");
        ext_snd_dev = device;

        registerMediaPort(nullptr);
        id = pjsua_ext_snd_dev_get_conf_port(device);
    }
};

struct PjsipBackend::Impl {
    Impl(core::EventLoop &eventLoop, const core::ICredentialProvider &credentialProvider,
         Options backendOptions)
        : loop{eventLoop}, credentials{credentialProvider}, options{backendOptions}
    {}

    core::EventLoop &loop;
    const core::ICredentialProvider &credentials;
    Options options;

    std::atomic<bool> refusingIncoming{false};

    std::atomic<bool> refusingSecond{false};

    core::ISipBackend::Observer *observer{nullptr};
    core::ICallBackend::Observer *callObserver{nullptr};
    std::unique_ptr<PjsipBackend::Endpoint> endpoint;

    std::unique_ptr<pj_thread_desc[]> stackThread;

    core::IAudioDeviceBackend::DeviceLostCallback deviceLost;
    core::IAudioDeviceBackend::DevicesChangedCallback devicesChanged;

    std::unique_ptr<DeviceWatch> watchingTheMachine;

    core::Volumes volumes;

    std::atomic_bool recovering{false};
    std::map<std::string, std::unique_ptr<Call>> calls;

    std::map<std::string, core::CallMediaStats> mediaStats;
    std::mutex mediaStatsLock;

    std::atomic_bool samplingMedia{false};

    [[nodiscard]] std::optional<core::CallMediaStats> readMediaOf(Call &call) const;
    void sampleTheMediaOf(const std::string &callId, Call &call);
    void armTheMediaSampler();
    void sampleEveryCallsMedia();
    void forgetMediaOf(const std::string &callId);

    std::map<std::string, std::string> pendingSeances;

    std::vector<std::string> routedCallIds;

    void connectGroup();

    [[nodiscard]] bool isRouted(const std::string &callId) const
    {
        return std::find(routedCallIds.begin(), routedCallIds.end(), callId) != routedCallIds.end();
    }

    void forgetRouted(const std::string &callId)
    {
        routedCallIds.erase(std::remove(routedCallIds.begin(), routedCallIds.end(), callId),
                            routedCallIds.end());
        refreshSignalTaps();
    }

    unsigned nextIncomingNumber{0};

    unsigned nextTransferNumber{0};
    std::map<std::string, std::unique_ptr<Account>> accounts;

    std::map<std::string, std::unique_ptr<WatchedLine>> watches;

    std::vector<std::unique_ptr<WatchedLine>> retiredWatches;

    static constexpr std::size_t kRetiredWarningLevel = 170;
    core::ILineWatchBackend::Observer *lineWatchObserver{nullptr};

    void publishLineState(const std::string &watchId, core::LineState state)
    {
        loop.post([this, watchId, state]() {
            if (lineWatchObserver != nullptr) {
                lineWatchObserver->onLineState(watchId, state);
            }
        });
    }

    void publishWatchFailed(const std::string &watchId, int sipStatus)
    {
        loop.post([this, watchId, sipStatus]() {
            if (lineWatchObserver != nullptr) {
                lineWatchObserver->onWatchFailed(watchId, sipStatus);
            }
        });
    }

    std::uint16_t boundPort{0};

    pj::TransportId tcpTransport{PJSUA_INVALID_ID};

    pj::TransportId tlsVerifying{PJSUA_INVALID_ID};
    pj::TransportId tlsTrusting{PJSUA_INVALID_ID};

    bool running{false};

    void publish(const std::string &accountId, const core::RegistrationOutcome &outcome)
    {
        static_cast<void>(claimOutcome(accountId));

        loop.post([this, accountId, outcome]() {
            if (observer != nullptr) {
                observer->onRegistrationOutcome(accountId, outcome);
            }
        });
    }

    [[nodiscard]] bool switchAsks(const std::string &source, const std::string &user,
                                  const std::string &sipCallId, core::SwitchRequest what);

    [[nodiscard]] int lineForRequest(pjsip_rx_data *rdata, int chosen);

    [[nodiscard]] std::string callWithSipCallId(const std::string &sipCallId) const;

    void noteCompletedElsewhere(const std::string &sipCallId);
    [[nodiscard]] bool wasCompletedElsewhere(const std::string &sipCallId);

    mutable std::mutex elsewhereLock;
    std::set<std::string> completedElsewhere;

    void publishCallProgress(const core::CallProgress &progress)
    {
        loop.post([this, progress]() {
            if (callObserver != nullptr) {
                callObserver->onCallProgress(progress);
            }
        });
    }

    void publishTransferProgress(const core::TransferProgress &progress)
    {
        loop.post([this, progress]() {
            if (callObserver != nullptr) {
                callObserver->onTransferProgress(progress);
            }
        });
    }

    void publishTransferRequested(const std::string &callId, const std::string &newCallId,
                                  const std::string &target, const std::string &referredBy)
    {
        loop.post([this, callId, newCallId, target, referredBy]() {
            if (callObserver != nullptr) {
                callObserver->onTransferRequested(callId, newCallId, target, referredBy);
            }
        });
    }

    void publishCallReplaced(const std::string &oldCallId, const std::string &newCallId)
    {
        loop.post([this, oldCallId, newCallId]() {
            if (callObserver != nullptr) {
                callObserver->onCallReplaced(oldCallId, newCallId);
            }
        });
    }

    void publishIncomingCall(const core::IncomingCall &incoming)
    {
        loop.post([this, incoming]() {
            if (callObserver != nullptr) {
                callObserver->onIncomingCall(incoming);
            }
        });
    }

    void retryHold(const std::string &callId, bool held, std::chrono::milliseconds delay);

    void releaseTransferAfter(const std::string &callId, std::chrono::milliseconds delay);

    void publishRemoteHold(const std::string &callId, bool held)
    {
        loop.post([this, callId, held]() {
            if (callObserver != nullptr) {
                callObserver->onRemoteHoldChanged(callId, held);
            }
        });
    }

    void publishEarlyMedia(const std::string &callId, bool arriving)
    {
        loop.post([this, callId, arriving]() {
            if (callObserver != nullptr) {
                callObserver->onEarlyMediaChanged(callId, arriving);
            }
        });
    }

    void publishRemoteIdentity(const std::string &callId, const core::SipAddress &identity)
    {
        loop.post([this, callId, identity]() {
            if (callObserver != nullptr) {
                callObserver->onRemoteIdentityChanged(callId, identity);
            }
        });
    }

    void markInFlight(const std::string &accountId)
    {
        const std::lock_guard lock{inFlightMutex};
        inFlight.insert(accountId);
    }

    [[nodiscard]] bool claimOutcome(const std::string &accountId)
    {
        const std::lock_guard lock{inFlightMutex};
        return inFlight.erase(accountId) > 0;
    }

    std::mutex inFlightMutex;
    std::set<std::string> inFlight;

    std::unique_ptr<pj::AudioMediaPlayer> tonePlayer;

    struct Notice {
        std::unique_ptr<pj::AudioMediaPlayer> player;

        std::vector<std::string> targetCalls;

        bool toTheSpeakers{false};

        std::vector<std::string> calls;
        std::string file;

        std::vector<std::string> files;

        bool waiting{false};
        std::chrono::seconds interval{0};
        core::IScheduler::TimerId timer{};
        bool repeating{false};

        std::function<void(core::NoticeProgress)> onProgress;

        bool intoTheRecording{false};
    };

    std::map<std::string, Notice> notices;

    [[nodiscard]] Notice *noticeFor(const std::string &noticeId)
    {
        const auto found = notices.find(noticeId);
        return found == notices.end() ? nullptr : &found->second;
    }

    [[nodiscard]] pjsua_conf_port_id noticeSlotFor(const std::string &recordingId)
    {
        const auto found = notices.find(recordingId);
        if (found == notices.end() || !found->second.intoTheRecording || !found->second.player) {
            return PJSUA_INVALID_ID;
        }

        try {
            return static_cast<pjsua_conf_port_id>(found->second.player->getPortId());
        } catch (const pj::Error &) {
            return PJSUA_INVALID_ID;
        }
    }

    void noticeFinished(const std::string &noticeId)
    {
        loop.post([this, noticeId]() {
            Notice *notice = noticeFor(noticeId);
            if (notice == nullptr) {
                return;
            }
            auto answer = std::exchange(notice->onProgress, nullptr);
            closeNotice(noticeId);
            if (answer) {

                answer(core::NoticeProgress::Given);
            }
        });
    }

    void closeNotice(const std::string &noticeId);

    enum class Played {

        Yes,

        NotYet,

        No,
    };

    Played playNotice(const std::string &noticeId, const std::vector<std::string> &callIds,
                      const std::vector<std::string> &files);

    void waitForAPort(const std::string &noticeId);
    void tryTheNoticeAgain(const std::string &noticeId);

    void noticeBegan(const std::string &noticeId)
    {
        if (Notice *notice = noticeFor(noticeId); notice != nullptr && notice->onProgress) {
            notice->onProgress(core::NoticeProgress::Began);
        }
    }

    static constexpr auto kNoticeRetry = std::chrono::milliseconds{500};

    std::unique_ptr<RingerDevice> ringerDevice;

    std::vector<core::AudioDeviceInfo> lastDevices;

    int ringerIndex{-1};

    struct PlayingTone {
        std::string path;
        bool loop{false};
        core::ToneOutput output{core::ToneOutput::Speakers};
        core::ToneLevel level{core::ToneLevel::Notifications};
    };

    std::optional<PlayingTone> playing;

    void toneFinished()
    {
        loop.post([this]() {
            if (onToneFinished) {
                onToneFinished();
            }
        });
    }

    std::function<void()> onToneFinished;

    std::function<void()> toneEnded;

    SoundDeviceWanted soundDeviceWanted;

    bool theMachineMovedMeanwhile{false};

    bool playbackHoldsTheDevice{false};

    int reasonsForTheSoundDevice{0};

    class OneReason {
    public:
        explicit OneReason(Impl &owner) : owner_{&owner}
        {
            owner_->wantTheSoundDevice(true);
        }

        OneReason(const OneReason &) = delete;
        OneReason &operator=(const OneReason &) = delete;
        OneReason(OneReason &&) = delete;
        OneReason &operator=(OneReason &&) = delete;

        ~OneReason()
        {
            if (!kept_) {
                owner_->wantTheSoundDevice(false);
            }
        }

        void keep() noexcept
        {
            kept_ = true;
        }

    private:
        Impl *owner_;
        bool kept_{false};
    };

    void sayTheMachineMoved()
    {
        if (pjsua_snd_is_active() && !onlyTheMeterHoldsTheDevice()) {
            theMachineMovedMeanwhile = true;
            return;
        }
        theMachineMovedMeanwhile = false;
        if (devicesChanged) {
            devicesChanged();
        }
    }

    void wantTheSoundDevice(bool wanted)
    {
        const auto before = reasonsForTheSoundDevice;
        reasonsForTheSoundDevice = std::max(0, before + (wanted ? 1 : -1));

        const bool held = before > 0;
        const bool holding = reasonsForTheSoundDevice > 0;
        if (held == holding) {
            return;
        }
        if (soundDeviceWanted) {
            soundDeviceWanted(holding);
        }

        if (!holding && theMachineMovedMeanwhile) {
            sayTheMachineMoved();
        }
    }

    [[nodiscard]] float gainFor(core::ToneLevel level) const noexcept
    {
        switch (level) {
        case core::ToneLevel::Conversation:
            return core::volumeGain(volumes.speakers);
        case core::ToneLevel::Ring:
            return core::volumeGain(volumes.ring);
        case core::ToneLevel::Notifications:
            break;
        }
        return core::volumeGain(volumes.notifications);
    }

    PjsipLogWriter *logWriter{nullptr};

    [[nodiscard]] pj::AudioMedia *toneDestination(core::ToneOutput output)
    {
        if (output != core::ToneOutput::Ringer || ringerIndex < 0) {
            return &endpoint->audDevManager().getPlaybackDevMedia();
        }

        if (ringerDevice) {
            return ringerDevice.get();
        }

        const auto began = std::chrono::steady_clock::now();

        try {

            auto device = std::make_unique<RingerDevice>(ringerIndex);
            device->openForPlayback();
            ringerDevice = std::move(device);
        } catch (const pj::Error &error) {

            core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                         "the ringer device would not open, ringing through "
                                         "the speakers instead: " +
                                             error.info());
            ringerDevice.reset();
            return &endpoint->audDevManager().getPlaybackDevMedia();
        }

        const auto took = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - began);
        core::Logger::instance().log(core::LogLevel::Info, "sip",
                                     "opened the ringer device in " + std::to_string(took.count()) +
                                         " ms");
        return ringerDevice.get();
    }

    pj_pool_t *meterPool{nullptr};
    pjmedia_port *meterPort{nullptr};
    pjsua_conf_port_id meterSlot{PJSUA_INVALID_ID};

    void openMeter()
    {
        if (meterPort || !endpoint) {
            return;
        }

        if (options.useNullAudioDevice) {

            core::Logger::instance().log(core::LogLevel::Debug, "sip",
                                         "no level to meter: the null audio device is in use");
            return;
        }

        ensureThreadRegistered();

        OneReason reason{*this};

        const auto began = std::chrono::steady_clock::now();

        pjsua_conf_port_info master{};
        pj_pool_t *pool = nullptr;
        pjmedia_port *port = nullptr;
        pjsua_conf_port_id slot = PJSUA_INVALID_ID;

        auto giveUp = [&](const std::string &operation, pj_status_t status) {
            if (slot != PJSUA_INVALID_ID) {
                pjsua_conf_remove_port(slot);
            }
            if (port) {
                pjmedia_port_destroy(port);
            }
            if (pool) {
                pj_pool_release(pool);
            }
            core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                         "the level meter would not start: " + operation + " " +
                                             std::to_string(status));
        };

        if (const auto status = pjsua_conf_get_port_info(0, &master); status != PJ_SUCCESS) {
            giveUp("pjsua_conf_get_port_info", status);
            return;
        }

        pool = pjsua_pool_create("meter", 512, 512);
        if (!pool) {
            giveUp("pjsua_pool_create", PJ_ENOMEM);
            return;
        }

        if (const auto status =
                pjmedia_null_port_create(pool, master.clock_rate, master.channel_count,
                                         master.samples_per_frame, master.bits_per_sample, &port);
            status != PJ_SUCCESS) {
            giveUp("pjmedia_null_port_create", status);
            return;
        }

        if (const auto status = pjsua_conf_add_port(pool, port, &slot); status != PJ_SUCCESS) {
            giveUp("pjsua_conf_add_port", status);
            return;
        }

        if (const auto status = pjsua_conf_connect(0, slot); status != PJ_SUCCESS) {
            giveUp("pjsua_conf_connect", status);
            return;
        }

        meterPool = pool;
        meterPort = port;
        meterSlot = slot;
        reason.keep();

        const auto took = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - began);
        core::Logger::instance().log(core::LogLevel::Info, "sip",
                                     "opened the sound device for the level meter in " +
                                         std::to_string(took.count()) + " ms");
    }

    [[nodiscard]] bool onlyTheMeterHoldsTheDevice() const
    {

        return meterPort != nullptr && calls.empty() && !playing.has_value() &&
               ringerDevice == nullptr && playback.port == nullptr;
    }

    struct BridgeTap {
        pj_pool_t *pool{nullptr};
        pjmedia_port *port{nullptr};
        pjsua_conf_port_id slot{PJSUA_INVALID_ID};

        std::vector<pjsua_conf_port_id> sources;
    };

    std::optional<core::CallSignals> feed;
    unsigned signalRate{16000};

    BridgeTap localTap;
    BridgeTap remoteTap;

    std::string watchedCall;

    struct RecordingSession {

        core::RecordingFeed *rings{nullptr};

        BridgeTap nearTap;
        BridgeTap farTap;

        std::map<std::string, BridgeTap> stemTaps;

        std::vector<std::string> callIds;

        bool separately{false};
    };

    std::map<std::string, RecordingSession> recordings;

    std::vector<std::unique_ptr<core::RecordingFeed>> feeds;
    std::vector<core::RecordingFeed *> idleFeeds;

    core::RecordingFeed *takeFeed()
    {

        for (auto idle = idleFeeds.begin(); idle != idleFeeds.end(); ++idle) {
            if ((*idle)->sampleRate() == signalRate &&
                (*idle)->frameSamples() == signalFrameSamples) {
                core::RecordingFeed *reused = *idle;
                idleFeeds.erase(idle);
                return reused;
            }
        }

        feeds.push_back(std::make_unique<core::RecordingFeed>(signalRate, signalFrameSamples));
        return feeds.back().get();
    }

    unsigned signalFrameSamples{320};

    void tuneSignals()
    {
        unsigned rate = 16000;
        pjsua_conf_port_info master{};
        if (pjsua_conf_get_port_info(0, &master) == PJ_SUCCESS && master.clock_rate != 0) {
            rate = master.clock_rate;
        }
        if (master.samples_per_frame != 0) {
            signalFrameSamples = master.samples_per_frame;
        }
        if (!feed || rate != signalRate) {
            feed.emplace(rate);
            signalRate = rate;
        }
    }

    core::CallSignals &ensureSignals()
    {
        if (!feed) {
            feed.emplace(signalRate);
        }
        return *feed;
    }

    void refreshRecordingTaps(RecordingSession &session);

    void refreshRecordingTaps();

    struct TapData {
        core::SignalRing *drawing{nullptr};
        core::RecordingRing *recording{nullptr};

        std::atomic<core::RealtimeRing *> listening{nullptr};

        std::size_t frameSamples{0};

        pj_pool_t *pool{nullptr};
    };

    struct PlaybackPort {
        pj_pool_t *pool{nullptr};
        pjmedia_port *port{nullptr};
        pjsua_conf_port_id slot{PJSUA_INVALID_ID};
    };

    PlaybackPort playback;
    core::IPlaybackSource *playbackSource{nullptr};

    struct PlaybackData {
        core::IPlaybackSource *source{nullptr};
        pj_pool_t *pool{nullptr};
    };

    static pj_status_t playbackGetFrame(pjmedia_port *port, pjmedia_frame *frame)
    {
        auto *data = static_cast<PlaybackData *>(port->port_data.pdata);
        if (data == nullptr || data->source == nullptr || frame == nullptr ||
            frame->buf == nullptr) {
            if (frame != nullptr) {
                frame->type = PJMEDIA_FRAME_TYPE_NONE;
                frame->size = 0;
            }
            return PJ_SUCCESS;
        }

        const std::size_t wanted = PJMEDIA_PIA_SPF(&port->info);
        auto *samples = static_cast<std::int16_t *>(frame->buf);

        const std::size_t got = data->source->pull(std::span<std::int16_t>{samples, wanted});

        if (got < wanted) {
            std::fill(samples + got, samples + wanted, std::int16_t{0});
        }

        frame->type = PJMEDIA_FRAME_TYPE_AUDIO;
        frame->size = wanted * sizeof(std::int16_t);
        return PJ_SUCCESS;
    }

    static pj_status_t playbackPutFrame(pjmedia_port * , pjmedia_frame * )
    {
        return PJ_SUCCESS;
    }

    static pj_status_t playbackOnDestroy(pjmedia_port *port)
    {
        auto *data = static_cast<PlaybackData *>(port->port_data.pdata);
        if (data != nullptr && data->pool != nullptr) {
            pj_pool_t *pool = data->pool;
            port->port_data.pdata = nullptr;
            pj_pool_release(pool);
        }
        return PJ_SUCCESS;
    }

    void closePlayback()
    {
        if (playback.port == nullptr) {
            playbackSource = nullptr;
            return;
        }

        ensureThreadRegistered();

        if (auto *data = static_cast<PlaybackData *>(playback.port->port_data.pdata);
            data != nullptr) {
            data->source = nullptr;
        }

        pjsua_conf_disconnect(playback.slot, 0);
        pjsua_conf_remove_port(playback.slot);
        pjmedia_port_dec_ref(playback.port);

        playback = PlaybackPort{};
        playbackSource = nullptr;

        if (std::exchange(playbackHoldsTheDevice, false)) {
            wantTheSoundDevice(false);
        }
    }

    static pj_status_t tapPutFrame(pjmedia_port *port, pjmedia_frame *frame)
    {
        auto *data = static_cast<TapData *>(port->port_data.pdata);
        if (data == nullptr || frame == nullptr) {
            return PJ_SUCCESS;
        }

        const bool audio =
            frame->type == PJMEDIA_FRAME_TYPE_AUDIO && frame->buf != nullptr && frame->size > 0;

        if (data->drawing != nullptr && audio) {
            data->drawing->write(static_cast<const std::int16_t *>(frame->buf),
                                 frame->size / sizeof(std::int16_t));
        }

        if (data->recording != nullptr) {

            if (audio) {
                data->recording->write(frame->timestamp.u64,
                                       static_cast<const std::int16_t *>(frame->buf),
                                       frame->size / sizeof(std::int16_t));
            } else {
                data->recording->write(frame->timestamp.u64, nullptr, data->frameSamples);
            }
        }

        if (auto *const listening = data->listening.load(std::memory_order_acquire);
            listening != nullptr) {

            if (audio) {
                listening->write(frame->timestamp.u64,
                                 static_cast<const std::int16_t *>(frame->buf),
                                 frame->size / sizeof(std::int16_t));
            } else {
                listening->write(frame->timestamp.u64, nullptr, data->frameSamples);
            }
        }

        return PJ_SUCCESS;
    }

    static pj_status_t tapOnDestroy(pjmedia_port *port)
    {
        auto *data = static_cast<TapData *>(port->port_data.pdata);
        if (data != nullptr && data->pool != nullptr) {
            pj_pool_t *pool = data->pool;
            port->port_data.pdata = nullptr;
            pj_pool_release(pool);
        }
        return PJ_SUCCESS;
    }

    static pj_status_t tapGetFrame(pjmedia_port * , pjmedia_frame *frame)
    {
        if (frame != nullptr) {
            frame->type = PJMEDIA_FRAME_TYPE_NONE;
            frame->size = 0;
        }
        return PJ_SUCCESS;
    }

    void openTap(BridgeTap &tap, const std::vector<pjsua_conf_port_id> &sources,
                 core::SignalRing *drawing, core::RecordingRing *evidence,
                 core::RealtimeRing *listening = nullptr)
    {
        if (!running || sources.empty()) {
            return;
        }

        const auto feeding = [](pjsua_conf_port_id source, pjsua_conf_port_id sink) {
            if (sink == PJSUA_INVALID_ID) {
                return false;
            }
            pjsua_conf_port_info info{};
            if (pjsua_conf_get_port_info(source, &info) != PJ_SUCCESS) {
                return false;
            }
            return std::any_of(info.listeners, info.listeners + info.listener_cnt,
                               [sink](pjsua_conf_port_id listener) { return listener == sink; });
        };

        if (tap.port != nullptr) {
            if (auto *const data = static_cast<TapData *>(tap.port->port_data.pdata);
                data != nullptr) {
                data->listening.store(listening, std::memory_order_release);
            }
        }

        if (tap.port != nullptr && tap.sources == sources &&
            std::all_of(sources.begin(), sources.end(),
                        [&](pjsua_conf_port_id source) { return feeding(source, tap.slot); })) {
            return;
        }

        ensureThreadRegistered();

        const bool forTheFile = evidence != nullptr;
        auto complain = [forTheFile](const std::string &operation, pj_status_t status) {
            core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                         (forTheFile ? "the recording has no audio: "
                                                     : "the call card's drawing has no audio: ") +
                                             operation + " " + std::to_string(status));
        };

        if (tap.port == nullptr) {
            pjsua_conf_port_info master{};
            if (const auto status = pjsua_conf_get_port_info(0, &master); status != PJ_SUCCESS) {
                complain("pjsua_conf_get_port_info", status);
                return;
            }

            pj_pool_t *pool = pjsua_pool_create(forTheFile ? "record-tap" : "signal-tap", 512, 512);
            if (pool == nullptr) {
                complain("pjsua_pool_create", PJ_ENOMEM);
                return;
            }

            auto *port = PJ_POOL_ZALLOC_T(pool, pjmedia_port);
            pj_str_t name = pj_str(const_cast<char *>(forTheFile ? "record-tap" : "signal-tap"));
            if (const auto status = pjmedia_port_info_init(
                    &port->info, &name, PJMEDIA_SIG_CLASS_PORT_AUD('s', 'g'), master.clock_rate,
                    master.channel_count, master.bits_per_sample, master.samples_per_frame);
                status != PJ_SUCCESS) {
                complain("pjmedia_port_info_init", status);
                pj_pool_release(pool);
                return;
            }

            auto *data = PJ_POOL_ZALLOC_T(pool, TapData);

            data = new (data) TapData{};

            data->drawing = drawing;
            data->recording = evidence;
            data->listening.store(listening, std::memory_order_relaxed);
            data->frameSamples = master.samples_per_frame;
            data->pool = pool;

            port->put_frame = &Impl::tapPutFrame;
            port->get_frame = &Impl::tapGetFrame;
            port->on_destroy = &Impl::tapOnDestroy;
            port->port_data.pdata = data;

            if (const auto status = pjmedia_port_init_grp_lock(port, pool, nullptr);
                status != PJ_SUCCESS) {
                complain("pjmedia_port_init_grp_lock", status);
                pj_pool_release(pool);
                return;
            }

            pjsua_conf_port_id slot = PJSUA_INVALID_ID;
            if (const auto status = pjsua_conf_add_port(pool, port, &slot); status != PJ_SUCCESS) {
                complain("pjsua_conf_add_port", status);
                pjmedia_port_dec_ref(port);
                return;
            }

            tap.pool = pool;
            tap.port = port;
            tap.slot = slot;
        }

        for (const pjsua_conf_port_id existing : tap.sources) {
            if (std::find(sources.begin(), sources.end(), existing) == sources.end()) {
                pjsua_conf_disconnect(existing, tap.slot);

                if (drawing != nullptr) {
                    drawing->clear();
                }
            }
        }

        std::vector<pjsua_conf_port_id> connected;
        for (const pjsua_conf_port_id wanted : sources) {

            if (feeding(wanted, tap.slot)) {
                connected.push_back(wanted);
                continue;
            }
            if (const auto status = pjsua_conf_connect(wanted, tap.slot); status != PJ_SUCCESS) {
                complain("pjsua_conf_connect", status);
                continue;
            }
            connected.push_back(wanted);
        }

        tap.sources = connected;
    }

    void undoTheSlider(BridgeTap &tap, int percent)
    {
        if (tap.slot == PJSUA_INVALID_ID) {
            return;
        }

        const float gain = core::volumeGain(percent);
        const float undo = gain > 0.01F ? 1.0F / gain : 1.0F;
        static_cast<void>(pjsua_conf_adjust_tx_level(tap.slot, undo));
    }

    void closeTap(BridgeTap &tap)
    {
        if (tap.port == nullptr) {
            return;
        }

        ensureThreadRegistered();

        for (const pjsua_conf_port_id source : tap.sources) {
            pjsua_conf_disconnect(source, tap.slot);
        }
        pjsua_conf_remove_port(tap.slot);

        pjmedia_port_dec_ref(tap.port);

        tap = BridgeTap{};
    }

    void refreshSignalTaps();

    void closeMeter()
    {
        if (!meterPort) {
            return;
        }

        ensureThreadRegistered();

        pjsua_conf_disconnect(0, meterSlot);
        pjsua_conf_remove_port(meterSlot);
        pjmedia_port_destroy(meterPort);
        pj_pool_release(meterPool);

        meterSlot = PJSUA_INVALID_ID;
        meterPort = nullptr;
        meterPool = nullptr;

        wantTheSoundDevice(false);
    }

    void closeRinger()
    {
        if (!ringerDevice) {
            return;
        }

        ensureThreadRegistered();

        try {
            ringerDevice->close();
        } catch (const pj::Error &error) {
            core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                         "the ringer device would not close: " + error.info());
        }

        ringerDevice.reset();
    }

    void ensureThreadRegistered()
    {
        if (endpoint && !endpoint->libIsThreadRegistered()) {
            endpoint->libRegisterThread("softphone");
        }
    }
};

const pjsip_method kRefer = {PJSIP_OTHER_METHOD, {const_cast<char *>("REFER"), 5}};
const pjsip_method kNotify = {PJSIP_OTHER_METHOD, {const_cast<char *>("NOTIFY"), 6}};

class TonePlayer final : public pj::AudioMediaPlayer {
public:
    explicit TonePlayer(std::function<void()> finished) : finished_{std::move(finished)} {}

    void onEof2() override
    {
        if (finished_) {
            finished_();
        }
    }

private:
    std::function<void()> finished_;
};

class PjsipBackend::Call final : public pj::Call {
public:
    Call(Impl &owner, pj::Account &account, std::string accountId, std::string callId,
         int existingCallId = PJSUA_INVALID_ID)
        : pj::Call(account, existingCallId), owner_{owner}, accountId_{std::move(accountId)},
          callId_{std::move(callId)},

          weOwnTheDialog_{existingCallId == PJSUA_INVALID_ID}
    {}

    [[nodiscard]] std::string sipCallId() const
    {
        try {
            return getInfo().callIdString;
        } catch (const pj::Error &) {
            return {};
        }
    }

    [[nodiscard]] const std::string &ourCallId() const noexcept
    {
        return callId_;
    }

    void note(core::LogLevel level, const std::string &what) const
    {
        const core::LogAttribute fields[] = {{"call", callId_}, {"line", accountId_}};
        core::Logger::instance().log(level, "sip", what, fields);
    }

    void onCallSdpCreated(pj::OnCallSdpCreatedParam &prm) override
    {
        markSdpAsOurs(static_cast<pjmedia_sdp_session *>(prm.sdp.pjSdpSession));
    }

    void onCallState(pj::OnCallStateParam & ) override
    {
        pj::CallInfo info;
        try {
            info = getInfo();
        } catch (const pj::Error &) {
            return;
        }

        core::CallProgress progress;
        progress.callId = callId_;
        progress.state = callStateFor(info.state);
        progress.sipStatus = static_cast<int>(info.lastStatusCode);

        if (info.state == PJSIP_INV_STATE_DISCONNECTED) {
            progress.completedElsewhere = owner_.wasCompletedElsewhere(info.callIdString);
        }
        progress.reason = info.lastReason;
        progress.endedLocally = endedLocally_;

        if (info.state == PJSIP_INV_STATE_DISCONNECTED &&
            (strandedByTransport_ || DialogTransports::wasStranded(info.callIdString))) {
            progress.sipStatus = 0;
            progress.reason = "the signalling connection went away";
            progress.endedLocally = false;
        }

        if (info.state == PJSIP_INV_STATE_CONFIRMED && earlyMedia_) {
            earlyMedia_ = false;
            owner_.publishEarlyMedia(callId_, false);

            if (owner_.isRouted(callId_)) {
                try {
                    owner_.connectGroup();
                } catch (const pj::Error &) {
                }
            }
        }

        if (!strandedByTransport_ || info.state != PJSIP_INV_STATE_DISCONNECTED) {
            owner_.publishCallProgress(progress);
        }

        if (info.state == PJSIP_INV_STATE_DISCONNECTED) {

            DialogTransports::forget(info.callIdString);

            const auto finished = callId_;
            owner_.loop.post([&owner = owner_, finished]() {
                owner.forgetMediaOf(finished);
                owner.calls.erase(finished);
                owner.refreshSignalTaps();
                owner.refreshRecordingTaps();
            });
        }
    }

    void onCallMediaState(pj::OnCallMediaStateParam & ) override
    {
        pj::CallInfo info;
        try {
            info = getInfo();
        } catch (const pj::Error &) {
            return;
        }

        auto remoteHold = false;
        auto audible = -1;

        for (unsigned i = 0; i < info.media.size(); ++i) {
            if (info.media[i].type != PJMEDIA_TYPE_AUDIO) {
                continue;
            }

            const auto status = info.media[i].status;

            if (status == PJSUA_CALL_MEDIA_REMOTE_HOLD) {
                remoteHold = true;
            }

            if (status == PJSUA_CALL_MEDIA_ACTIVE || status == PJSUA_CALL_MEDIA_REMOTE_HOLD) {
                audible = static_cast<int>(i);
            }
        }

        remoteHeld_ = remoteHold;

        const auto early = info.state == PJSIP_INV_STATE_EARLY && audible >= 0;
        if (early != earlyMedia_) {
            earlyMedia_ = early;
            owner_.publishEarlyMedia(callId_, early);
        }

        if (slot() != PJSUA_INVALID_ID && owner_.isRouted(callId_)) {
            owner_.connectGroup();
        }

        if (remoteHold != reportedRemoteHold_) {
            reportedRemoteHold_ = remoteHold;
            owner_.publishRemoteHold(callId_, remoteHold);
        }

        owner_.sampleTheMediaOf(callId_, *this);
    }

    void requestHold(bool held)
    {
        holdAttempts_ = 0;
        applyHold(held);
    }

    void applyHold(bool held)
    {
        holdWanted_ = held;

        try {
            if (held) {
                disconnectAudio();

                auto prm = audioOnlyOptions();
                setHold(prm);
                offerInFlight_ = true;
                return;
            }

            auto prm = audioOnlyOptions();
            prm.opt.flag = PJSUA_CALL_UNHOLD;
            reinvite(prm);
            offerInFlight_ = true;
        } catch (const pj::Error &) {

        }
    }

    void setSeance(std::string seance)
    {
        seance_ = std::move(seance);
    }

    [[nodiscard]] const std::string &seance() const noexcept
    {
        return seance_;
    }

    void transferTo(const std::string &target)
    {
        if (offerInFlight_) {
            note(core::LogLevel::Info, "an offer of ours is still outstanding on this dialog; "
                                       "the transfer to " +
                                           target + " waits for it");
            transferTo_ = target;

            owner_.releaseTransferAfter(callId_, kHeldBackTransferGrace);
            return;
        }

        sendTransfer(target);
    }

    void sendHeldBackTransfer()
    {
        if (!transferTo_.has_value()) {
            return;
        }

        const auto target = *transferTo_;
        transferTo_.reset();
        sendTransfer(target);
    }

    void sendTransfer(const std::string &target)
    {
        note(core::LogLevel::Info, "transferring it to " + target);

        try {
            pj::CallOpParam options;
            xfer(withSeance(target, seance_), options);
        } catch (const pj::Error &error) {

            note(core::LogLevel::Warning,
                 "could not transfer it to " + target + ": " + error.info());
            owner_.publishTransferProgress(core::TransferProgress{
                callId_, PJSIP_SC_INTERNAL_SERVER_ERROR, true, error.info()});
        }
    }

    void offerSettled()
    {
        offerInFlight_ = false;
        sendHeldBackTransfer();
    }

    void onCallTsxState(pj::OnCallTsxStateParam &prm) override
    {
        readIdentityFrom(prm.e);
        readReferredByFrom(prm.e);
        traceTransferNotify(prm.e);

        const auto &tsx = prm.e.body.tsxState.tsx;

        if (tsx.role == PJSIP_ROLE_UAC && (tsx.method == "INVITE" || tsx.method == "UPDATE")) {
            if (tsx.statusCode < 200) {
                offerInFlight_ = true;
            } else if (tsx.statusCode != PJSIP_SC_REQUEST_PENDING) {
                offerSettled();
            }
        }

        if (tsx.role != PJSIP_ROLE_UAC || tsx.method != "INVITE" ||
            tsx.statusCode != PJSIP_SC_REQUEST_PENDING) {
            return;
        }

        if (holdAttempts_ >= kMaxHoldAttempts) {

            offerSettled();
            note(core::LogLevel::Warning, "gave up holding it after " +
                                              std::to_string(holdAttempts_) +
                                              " attempts; the far end stayed busy");
            return;
        }

        ++holdAttempts_;

        const auto base = weOwnTheDialog_ ? kGlareWaitAsOwner : kGlareWaitAsGuest;
        const auto range = weOwnTheDialog_ ? kGlareSpreadAsOwner : kGlareSpreadAsGuest;
        const auto seed = callId_ + ':' + std::to_string(holdAttempts_);
        const auto spread =
            std::chrono::milliseconds{static_cast<int>(std::hash<std::string>{}(seed) % range)};
        const auto backoff = kGlareBackoffStep * (holdAttempts_ - 1);
        const auto wait = base + spread + backoff;

        note(core::LogLevel::Info,
             std::string{"the far end was busy with an INVITE; retrying the hold in "} +
                 std::to_string(wait.count()) + "ms (attempt " + std::to_string(holdAttempts_ + 1) +
                 " of " + std::to_string(kMaxHoldAttempts + 1) + ")");

        owner_.retryHold(callId_, holdWanted_, wait);
    }

    void onCallTransferStatus(pj::OnCallTransferStatusParam &prm) override
    {

        note(core::LogLevel::Info, "transfer is at " +
                                       std::to_string(static_cast<int>(prm.statusCode)) + " " +
                                       prm.reason + (prm.finalNotify ? " (final)" : ""));

        owner_.publishTransferProgress(core::TransferProgress{
            callId_, static_cast<int>(prm.statusCode), prm.finalNotify, prm.reason});
    }

    void onCallTransferRequest(pj::OnCallTransferRequestParam &prm) override
    {
        const auto account = owner_.accounts.find(accountId_);
        if (account == owner_.accounts.end()) {
            prm.statusCode = PJSIP_SC_SERVICE_UNAVAILABLE;
            return;
        }

        const auto newCallId = "xfer-" + std::to_string(++owner_.nextTransferNumber);

        note(core::LogLevel::Info,
             "the far end is transferring us to " + prm.dstUri + "; the new call is " + newCallId);

        auto call = std::make_unique<Call>(owner_, *account->second, accountId_, newCallId);
        prm.newCall = call.get();
        owner_.calls[newCallId] = std::move(call);

        owner_.publishTransferRequested(callId_, newCallId, core::parseSipAddress(prm.dstUri).uri,
                                        referredBy_);
    }

    void onCallReplaceRequest(pj::OnCallReplaceRequestParam &prm) override
    {
        const auto account = owner_.accounts.find(accountId_);
        if (account == owner_.accounts.end()) {
            prm.statusCode = PJSIP_SC_SERVICE_UNAVAILABLE;
            return;
        }

        const auto newCallId = "repl-" + std::to_string(++owner_.nextTransferNumber);

        note(core::LogLevel::Info, "being replaced by " + newCallId);

        auto call = std::make_unique<Call>(owner_, *account->second, accountId_, newCallId);
        prm.newCall = call.get();
        owner_.calls[newCallId] = std::move(call);

        core::IncomingCall incoming;
        incoming.callId = newCallId;
        incoming.accountId = accountId_;

        if (auto *rdata = static_cast<pjsip_rx_data *>(prm.rdata.pjRxData);
            rdata != nullptr && rdata->msg_info.msg != nullptr) {
            const auto from = headerValues(rdata->msg_info.msg, "From");
            if (!from.empty()) {
                const auto address = core::parseSipAddress(from.front());
                incoming.remoteUri = address.uri;
                incoming.remoteDisplayName = address.displayName;
            }
            incoming.seanceId = seanceIn(rdata->msg_info.msg);
        }

        owner_.publishIncomingCall(incoming);
    }

    void onCallReplaced(pj::OnCallReplacedParam &prm) override
    {
        const auto *replacement = static_cast<const Call *>(prm.newCall);
        if (replacement == nullptr) {
            return;
        }

        note(core::LogLevel::Info, "has been replaced by " + replacement->coreCallId());

        owner_.publishCallReplaced(callId_, replacement->coreCallId());
    }

    [[nodiscard]] pjsua_conf_port_id slot() const
    {

        const auto stackId = getId();
        if (stackId < 0) {
            return PJSUA_INVALID_ID;
        }

        pjsua_call_info info{};
        if (pjsua_call_get_info(stackId, &info) != PJ_SUCCESS) {
            return PJSUA_INVALID_ID;
        }

        for (unsigned i = 0; i < info.media_cnt; ++i) {
            if (info.media[i].type != PJMEDIA_TYPE_AUDIO) {
                continue;
            }
            if (info.media[i].status == PJSUA_CALL_MEDIA_ACTIVE ||
                info.media[i].status == PJSUA_CALL_MEDIA_REMOTE_HOLD) {
                return info.media[i].stream.aud.conf_slot;
            }
        }

        return PJSUA_INVALID_ID;
    }

    static constexpr pjsua_conf_port_id kSoundDevice = 0;

    void setMuted(bool muted)
    {
        muted_ = muted;

        const auto mine = slot();
        if (mine == PJSUA_INVALID_ID) {
            return;
        }

        if (muted || remoteHeld_ || earlyMedia_) {
            static_cast<void>(pjsua_conf_disconnect(kSoundDevice, mine));
        } else {
            static_cast<void>(pjsua_conf_connect(kSoundDevice, mine));
        }
    }

    void applyConversationLevel()
    {
        const auto mine = slot();
        if (mine == PJSUA_INVALID_ID) {
            return;
        }

        if (const auto status =
                pjsua_conf_adjust_rx_level(mine, core::volumeGain(owner_.volumes.speakers));
            status != PJ_SUCCESS) {
            core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                         "could not set the call volume: " +
                                             std::to_string(status));
        }
    }

    void connectAudio()
    {
        const auto mine = slot();
        if (mine == PJSUA_INVALID_ID) {
            return;
        }

        static_cast<void>(pjsua_conf_connect(mine, kSoundDevice));

        applyConversationLevel();

        if (!muted_ && !remoteHeld_ && !earlyMedia_) {
            static_cast<void>(pjsua_conf_connect(kSoundDevice, mine));
        }
    }

    void connectTo(Call &other)
    {
        const auto mine = slot();
        const auto theirs = other.slot();
        if (mine == PJSUA_INVALID_ID || theirs == PJSUA_INVALID_ID) {
            return;
        }

        static_cast<void>(pjsua_conf_connect(mine, theirs));
        static_cast<void>(pjsua_conf_connect(theirs, mine));
    }

    void disconnectFrom(Call &other)
    {
        const auto mine = slot();
        const auto theirs = other.slot();
        if (mine == PJSUA_INVALID_ID || theirs == PJSUA_INVALID_ID) {
            return;
        }

        static_cast<void>(pjsua_conf_disconnect(mine, theirs));
        static_cast<void>(pjsua_conf_disconnect(theirs, mine));
    }

    [[nodiscard]] bool muted() const noexcept
    {
        return muted_;
    }

    [[nodiscard]] pjsua_conf_port_id audioSlot() const
    {
        return slot();
    }

    void disconnectAudio()
    {
        const auto mine = slot();
        if (mine == PJSUA_INVALID_ID) {
            return;
        }

        static_cast<void>(pjsua_conf_disconnect(mine, kSoundDevice));
        static_cast<void>(pjsua_conf_disconnect(kSoundDevice, mine));
    }

    [[nodiscard]] const std::string &accountId() const noexcept
    {
        return accountId_;
    }

    [[nodiscard]] const std::string &coreCallId() const noexcept
    {
        return callId_;
    }

    void markEndedLocally()
    {
        endedLocally_ = true;
    }

    void markStrandedByTransport()
    {
        strandedByTransport_ = true;
    }

private:

    void readIdentityFrom(const pj::SipEvent &event)
    {
        if (event.body.tsxState.type != PJSIP_EVENT_RX_MSG) {
            return;
        }

        auto *rdata = static_cast<pjsip_rx_data *>(event.body.tsxState.src.rdata.pjRxData);
        if (rdata == nullptr || rdata->msg_info.msg == nullptr) {
            return;
        }

        core::AssertedIdentity asserted;
        asserted.pAssertedIdentity = headerValues(rdata->msg_info.msg, "P-Asserted-Identity");
        asserted.remotePartyId = headerValues(rdata->msg_info.msg, "Remote-Party-ID");
        asserted.privacy = headerValues(rdata->msg_info.msg, "Privacy");

        const auto direction =
            weOwnTheDialog_ ? core::CallDirection::Outgoing : core::CallDirection::Incoming;
        const auto identity = core::identityToShow(asserted, direction);
        if (!identity.has_value() || *identity == reportedIdentity_) {
            return;
        }

        reportedIdentity_ = *identity;
        note(core::LogLevel::Info,
             "the far end is now " + (identity->displayName.empty()
                                          ? identity->uri
                                          : identity->displayName + " <" + identity->uri + ">"));
        owner_.publishRemoteIdentity(callId_, *identity);
    }

    void readReferredByFrom(const pj::SipEvent &event)
    {
        if (event.body.tsxState.type != PJSIP_EVENT_RX_MSG) {
            return;
        }

        auto *rdata = static_cast<pjsip_rx_data *>(event.body.tsxState.src.rdata.pjRxData);
        if (rdata == nullptr || rdata->msg_info.msg == nullptr) {
            return;
        }

        const auto *msg = rdata->msg_info.msg;
        if (msg->type != PJSIP_REQUEST_MSG ||
            pjsip_method_cmp(&msg->line.req.method, &kRefer) != 0) {
            return;
        }

        const auto values = headerValues(msg, "Referred-By");
        referredBy_ = values.empty() ? std::string{} : core::parseSipAddress(values.front()).uri;
    }

    void traceTransferNotify(const pj::SipEvent &event)
    {
        if (event.body.tsxState.type != PJSIP_EVENT_RX_MSG) {
            return;
        }

        auto *rdata = static_cast<pjsip_rx_data *>(event.body.tsxState.src.rdata.pjRxData);
        if (rdata == nullptr || rdata->msg_info.msg == nullptr) {
            return;
        }

        const auto *msg = rdata->msg_info.msg;
        if (msg->type != PJSIP_REQUEST_MSG ||
            pjsip_method_cmp(&msg->line.req.method, &kNotify) != 0) {
            return;
        }

        const auto subscription = headerValues(msg, "Subscription-State");
        std::string body;
        if (msg->body != nullptr && msg->body->data != nullptr) {
            body.assign(static_cast<const char *>(msg->body->data), msg->body->len);
            while (!body.empty() && (body.back() == '\n' || body.back() == '\r')) {
                body.pop_back();
            }
        }

        note(core::LogLevel::Debug,
             "NOTIFY, Subscription-State: " +
                 (subscription.empty() ? std::string{"(absent)"} : subscription.front()) +
                 ", body: " + (body.empty() ? std::string{"(empty)"} : body));
    }

    Impl &owner_;
    std::string accountId_;
    std::string callId_;
    std::string seance_;

    static constexpr unsigned kMaxHoldAttempts = 4;

    static constexpr auto kHeldBackTransferGrace = std::chrono::milliseconds{2000};

    static constexpr auto kGlareBackoffStep = std::chrono::milliseconds{500};

    static constexpr auto kGlareWaitAsOwner = std::chrono::milliseconds{2100};
    static constexpr auto kGlareSpreadAsOwner = 1900U;
    static constexpr auto kGlareWaitAsGuest = std::chrono::milliseconds{0};
    static constexpr auto kGlareSpreadAsGuest = 2000U;

    const bool weOwnTheDialog_;

    std::atomic<bool> muted_{false};

    std::atomic<bool> remoteHeld_{false};

    std::atomic<bool> earlyMedia_{false};

    bool reportedRemoteHold_{false};

    core::SipAddress reportedIdentity_{};

    std::string referredBy_{};
    bool holdWanted_{false};
    unsigned holdAttempts_{0};

    bool offerInFlight_{false};

    std::optional<std::string> transferTo_{};
    bool endedLocally_{false};

    bool strandedByTransport_{false};
};

void PjsipBackend::Impl::refreshSignalTaps()
{
    auto slotOf = [this](const std::string &callId) {
        const auto it = calls.find(callId);
        return it == calls.end() ? PJSUA_INVALID_ID : it->second->audioSlot();
    };

    std::vector<pjsua_conf_port_id> remoteSources;
    if (!watchedCall.empty() && isRouted(watchedCall)) {
        for (const auto &callId : routedCallIds) {
            if (const auto slot = slotOf(callId); slot != PJSUA_INVALID_ID) {
                remoteSources.push_back(slot);
            }
        }
    }

    if (remoteSources.empty()) {
        closeTap(localTap);
        closeTap(remoteTap);
        if (feed) {

            feed->local.ring().clear();
            feed->remote.ring().clear();
        }
        return;
    }

    auto &both = ensureSignals();

    const bool speaking =
        std::none_of(routedCallIds.begin(), routedCallIds.end(), [this](const std::string &callId) {
            const auto it = calls.find(callId);
            return it != calls.end() && it->second->muted();
        });

    if (speaking) {

        openTap(localTap, {0}, &both.local.ring(), nullptr);

        undoTheSlider(localTap, volumes.microphone);
    } else {
        closeTap(localTap);

        both.local.ring().clear();
    }

    openTap(remoteTap, remoteSources, &both.remote.ring(), nullptr);

    undoTheSlider(remoteTap, volumes.speakers);
}

void PjsipBackend::Impl::refreshRecordingTaps()
{
    for (auto &recording : recordings) {
        refreshRecordingTaps(recording.second);
    }
}

void PjsipBackend::Impl::refreshRecordingTaps(RecordingSession &session)
{

    std::string id;
    for (const auto &recording : recordings) {
        if (&recording.second == &session) {
            id = recording.first;
            break;
        }
    }

    auto slotOf = [this](const std::string &callId) {
        const auto it = calls.find(callId);
        return it == calls.end() ? PJSUA_INVALID_ID : it->second->audioSlot();
    };

    std::vector<pjsua_conf_port_id> remoteSources;
    for (const auto &callId : session.callIds) {
        if (const auto slot = slotOf(callId); slot != PJSUA_INVALID_ID) {
            remoteSources.push_back(slot);
        }
    }

    for (auto stem = session.stemTaps.begin(); stem != session.stemTaps.end();) {
        const bool wanted =
            session.separately && std::find(session.callIds.begin(), session.callIds.end(),
                                            stem->first) != session.callIds.end();
        if (wanted) {
            ++stem;
            continue;
        }
        closeTap(stem->second);
        stem = session.stemTaps.erase(stem);
    }

    if (session.callIds.empty()) {
        closeTap(session.nearTap);
        closeTap(session.farTap);
        return;
    }

    if (remoteSources.empty()) {
        closeTap(session.farTap);
    }

    core::RecordingFeed &rings = *session.rings;

    std::vector<pjsua_conf_port_id> nearSources{0};
    if (const auto notice = noticeSlotFor(id); notice != PJSUA_INVALID_ID) {
        nearSources.push_back(notice);
    }

    core::RealtimeRing *const farListening = rings.listening() ? rings.farListening() : nullptr;

    const bool speaking = std::none_of(session.callIds.begin(), session.callIds.end(),
                                       [this](const std::string &callId) {
                                           const auto it = calls.find(callId);
                                           return it != calls.end() && it->second->muted();
                                       });
    core::RealtimeRing *const nearListening =
        rings.listening() && speaking ? rings.nearListening() : nullptr;

    openTap(session.nearTap, nearSources, nullptr, &rings.nearSide(), nearListening);

    if (!remoteSources.empty()) {
        openTap(session.farTap, remoteSources, nullptr, &rings.farSide(), farListening);

        undoTheSlider(session.farTap, volumes.speakers);
    }

    if (!session.separately) {
        return;
    }

    for (const auto &callId : session.callIds) {
        const auto slot = slotOf(callId);
        if (slot == PJSUA_INVALID_ID) {
            continue;
        }
        openTap(session.stemTaps[callId], {slot}, nullptr, &rings.openStem(callId));
        undoTheSlider(session.stemTaps[callId], volumes.speakers);
    }
}

void PjsipBackend::Impl::connectGroup()
{
    for (const auto &callId : routedCallIds) {
        if (const auto it = calls.find(callId); it != calls.end()) {
            try {
                it->second->connectAudio();
            } catch (const pj::Error &) {
            }
        }
    }

    for (std::size_t first = 0; first < routedCallIds.size(); ++first) {
        for (std::size_t second = first + 1; second < routedCallIds.size(); ++second) {
            const auto one = calls.find(routedCallIds[first]);
            const auto other = calls.find(routedCallIds[second]);
            if (one != calls.end() && other != calls.end()) {
                one->second->connectTo(*other->second);
            }
        }
    }

    refreshSignalTaps();
    refreshRecordingTaps();
}

void PjsipBackend::Impl::releaseTransferAfter(const std::string &callId,
                                              std::chrono::milliseconds delay)
{
    static_cast<void>(loop.scheduleAfter(delay, [this, callId]() {
        const auto it = calls.find(callId);
        if (it == calls.end()) {
            return;
        }
        it->second->sendHeldBackTransfer();
    }));
}

void PjsipBackend::Impl::retryHold(const std::string &callId, bool held,
                                   std::chrono::milliseconds delay)
{
    static_cast<void>(loop.scheduleAfter(delay, [this, callId, held]() {
        const auto it = calls.find(callId);
        if (it == calls.end()) {
            return;
        }

        ensureThreadRegistered();
        it->second->applyHold(held);
    }));
}

class PjsipBackend::WatchedLine final : public pj::Buddy {
public:
    WatchedLine(Impl &owner, std::string watchId) : owner_{owner}, watchId_{std::move(watchId)} {}

    void retire() noexcept
    {
        live_.store(false, std::memory_order_release);
    }

    void releaseFromPjsua() noexcept
    {

        const auto stackId = getId();
        if (stackId == PJSUA_INVALID_ID || pjsua_buddy_is_valid(stackId) == PJ_FALSE) {
            return;
        }
        static_cast<void>(pjsua_buddy_del(stackId));
    }

    [[nodiscard]] bool live() const noexcept
    {
        return live_.load(std::memory_order_acquire);
    }

    void onBuddyDlgEventState() override
    {
        if (!live()) {
            return;
        }

        if (readABody_) {
            return;
        }

        pjsua_buddy_dlg_event_info info;
        pj_bzero(&info, sizeof(info));

        if (pjsua_buddy_get_dlg_event_info(getId(), &info) != PJ_SUCCESS) {
            return;
        }

        if (info.sub_state != PJSIP_EVSUB_STATE_ACTIVE &&
            info.sub_state != PJSIP_EVSUB_STATE_PENDING) {
            return;
        }

        owner_.publishLineState(watchId_, lineStateOf(info));
    }

    void onBuddyEvSubDlgEventState(pj::OnBuddyEvSubStateParam &prm) override
    {
        if (!live()) {
            return;
        }

        if (prm.e.type != PJSIP_EVENT_TSX_STATE) {
            return;
        }

        if (prm.e.body.tsxState.type == PJSIP_EVENT_RX_MSG) {
            if (const auto notice = dialogInfoIn(prm.e.body.tsxState.src.rdata.wholeMsg)) {
                readABody_ = true;
                sayAndPublish(*notice);
                return;
            }
        }

        pjsua_buddy_dlg_event_info info;
        pj_bzero(&info, sizeof(info));
        if (pjsua_buddy_get_dlg_event_info(getId(), &info) != PJ_SUCCESS) {
            return;
        }

        if (info.sub_state == PJSIP_EVSUB_STATE_TERMINATED) {

            known_.forget();
            readABody_ = false;
            owner_.publishWatchFailed(watchId_, static_cast<int>(info.sub_term_code));
            return;
        }
    }

private:

    void sayAndPublish(const core::DialogInfoNotice &notice)
    {
        const auto showing = known_.heard(notice);

        const auto complete = notice.complete;
        const auto named = notice.dialogs.size();
        const auto onIt = known_.dialogs().size();
        owner_.loop.post([watchId = watchId_, complete, named, onIt, showing]() {
            core::Logger::instance().log(
                core::LogLevel::Debug, "sip",
                "watched line " + watchId + ": " + (complete ? "the whole of" : "a change to") +
                    " its dialogs, " + std::to_string(named) + " named, " + std::to_string(onIt) +
                    " on it now — " + std::string{core::toString(showing)});
        });

        owner_.publishLineState(watchId_, showing);
    }

    [[nodiscard]] static core::LineState lineStateOf(const pjsua_buddy_dlg_event_info &info)
    {
        if (info.sub_state != PJSIP_EVSUB_STATE_ACTIVE &&
            info.sub_state != PJSIP_EVSUB_STATE_PENDING) {
            return core::LineState::Unknown;
        }

        if (info.dialog_state.ptr == nullptr || info.dialog_state.slen <= 0) {
            return core::LineState::Idle;
        }

        const std::string state{info.dialog_state.ptr,
                                static_cast<std::size_t>(info.dialog_state.slen)};

        if (state == "confirmed") {
            return core::LineState::Busy;
        }
        if (state == "trying" || state == "proceeding" || state == "early") {
            return core::LineState::Ringing;
        }
        return core::LineState::Idle;
    }

    Impl &owner_;
    std::string watchId_;

    core::WatchedLine known_;

    bool readABody_{false};

    std::atomic<bool> live_{true};
};

class PjsipBackend::Account final : public pj::Account {
public:
    Account(Impl &owner, std::string accountId) : owner_{owner}, accountId_{std::move(accountId)} {}

    void askToRegister()
    {
        if (awaitingAnswer_.exchange(true, std::memory_order_acq_rel)) {
            const core::LogAttribute fields[] = {{"line", accountId_}};
            core::Logger::instance().log(
                core::LogLevel::Debug, "sip",
                "a REGISTER is already on the wire; its answer is this one's too", fields);
            return;
        }

        try {
            setRegistration(true);
        } catch (const pj::Error &error) {
            awaitingAnswer_.store(false, std::memory_order_release);

            if (error.status != PJSIP_EBUSY) {
                throw;
            }

            const core::LogAttribute fields[] = {{"line", accountId_}};
            core::Logger::instance().log(
                core::LogLevel::Debug, "sip",
                "the stack was already sending a REGISTER of its own; waiting for its answer",
                fields);
        }
    }

    void setSwitchHost(std::string host)
    {
        const std::lock_guard guard{trust_};
        if (host == switchHost_ && !switchAddresses_.empty()) {
            return;
        }
        switchHost_ = std::move(host);
        switchAddresses_ = addressesOf(switchHost_);
    }

    [[nodiscard]] bool answersFor(const std::string &user) const
    {
        const std::lock_guard guard{trust_};
        return user == username_;
    }

    void setUsername(std::string user)
    {
        const std::lock_guard guard{trust_};
        username_ = std::move(user);
    }

    [[nodiscard]] std::string username() const
    {
        const std::lock_guard guard{trust_};
        return username_;
    }

    [[nodiscard]] std::vector<std::string> switchAddresses() const
    {
        const std::lock_guard guard{trust_};
        return registered_ ? switchAddresses_ : std::vector<std::string>{};
    }

    ~Account() override
    {
        try {
            shutdown();
        } catch (const pj::Error &) {
        }
    }

    Account(const Account &) = delete;
    Account &operator=(const Account &) = delete;
    Account(Account &&) = delete;
    Account &operator=(Account &&) = delete;

    [[nodiscard]] const std::string &accountId() const noexcept
    {
        return accountId_;
    }

    void onRegState(pj::OnRegStateParam &prm) override
    {

        awaitingAnswer_.store(false, std::memory_order_release);

        const auto outcome = outcomeFrom(prm);
        {
            const std::lock_guard guard{trust_};
            registered_ = outcome.registered;
            if (registered_ && switchAddresses_.empty()) {
                switchAddresses_ = addressesOf(switchHost_);
            }
        }
        owner_.publish(accountId_, outcome);
    }

    void onIncomingCall(pj::OnIncomingCallParam &prm) override
    {
        auto callId = "in-" + std::to_string(++owner_.nextIncomingNumber);
        noteCall(core::LogLevel::Info, callId, accountId_, "an invitation arrived");

        auto call = std::make_unique<Call>(owner_, *this, accountId_, callId, prm.callId);

        core::IncomingCall incoming;
        incoming.callId = callId;
        incoming.accountId = accountId_;

        try {

            const auto info = call->getInfo();
            const auto address = core::parseSipAddress(info.remoteUri);
            incoming.remoteUri = address.uri;
            incoming.remoteDisplayName = address.displayName;

            incoming.fromThisLine = answersFor(core::sipUserPart(incoming.remoteUri));
        } catch (const pj::Error &) {
        }

        if (auto *rdata = static_cast<pjsip_rx_data *>(prm.rdata.pjRxData);
            rdata != nullptr && rdata->msg_info.msg != nullptr) {
            const auto values = headerValues(rdata->msg_info.msg, "Referred-By");
            if (!values.empty()) {
                incoming.referredBy = core::parseSipAddress(values.front()).uri;
            }

            incoming.seanceId = seanceIn(rdata->msg_info.msg);

            readAutoAnswerRequest(incoming, rdata);
        }

        const auto refusal = [this, &incoming]() {
            if (owner_.refusingIncoming.load(std::memory_order_relaxed)) {
                return core::CallEndReason::DoNotDisturb;
            }
            if (owner_.refusingSecond.load(std::memory_order_relaxed) &&
                !incoming.autoAnswer.asked()) {
                return core::CallEndReason::CallWaitingOff;
            }
            return core::CallEndReason::None;
        }();

        if (refusal != core::CallEndReason::None) {
            noteCall(core::LogLevel::Info, callId, accountId_,
                     refusal == core::CallEndReason::DoNotDisturb
                         ? "busy to an invitation: do not disturb is on"
                         : "busy to an invitation: call waiting is off and this phone is on a "
                           "call");

            try {
                call->markEndedLocally();

                pj::CallOpParam busy;
                busy.statusCode = PJSIP_SC_BUSY_HERE;
                call->hangup(busy);
            } catch (const pj::Error &error) {
                noteCall(core::LogLevel::Warning, callId, accountId_,
                         "could not say it is busy: " + error.info());
            }

            incoming.refusedBeforeItRang = refusal;
            owner_.publishIncomingCall(incoming);
            noteCall(core::LogLevel::Info, callId, accountId_,
                     "published as refused, from " + incoming.remoteUri);
            return;
        }

        try {
            pj::CallOpParam ringing;
            ringing.statusCode = PJSIP_SC_RINGING;
            call->answer(ringing);
        } catch (const pj::Error &error) {
            noteCall(core::LogLevel::Warning, callId, accountId_,
                     "could not say it is ringing: " + error.info());
        }

        owner_.calls[callId] = std::move(call);
        owner_.publishIncomingCall(incoming);
        noteCall(core::LogLevel::Info, callId, accountId_,
                 "published as incoming, from " + incoming.remoteUri);
    }

private:

    void readAutoAnswerRequest(core::IncomingCall &incoming, pjsip_rx_data *rdata)
    {
        core::InvitationHeaders headers;
        headers.callInfo = headerValues(rdata->msg_info.msg, "Call-Info");
        headers.alertInfo = headerValues(rdata->msg_info.msg, "Alert-Info");
        headers.answerMode = headerValues(rdata->msg_info.msg, "Answer-Mode");

        const auto asked = core::autoAnswerRequest(headers);
        if (!asked.asked()) {
            return;
        }

        const std::string source{rdata->pkt_info.src_name};
        const auto trusted = switchAddresses();
        if (!core::fromTheSwitch(source, trusted)) {

            noteCall(core::LogLevel::Info, incoming.callId, accountId_,
                     std::string{core::toString(asked.spelling)} +
                         " asked for an auto-answer, ignored — " + source +
                         " is not the switch this line is registered to" +
                         (trusted.empty() ? " (it is not registered)" : ""));
            return;
        }

        incoming.autoAnswer = asked;
        noteCall(core::LogLevel::Info, incoming.callId, accountId_,
                 "the switch asked for an auto-answer after " +
                     std::to_string(asked.after.count()) + "s, by " +
                     std::string{core::toString(asked.spelling)});
    }

    Impl &owner_;
    std::string accountId_;

    std::atomic<bool> awaitingAnswer_{false};

    mutable std::mutex trust_;
    std::string username_;
    std::string switchHost_;
    std::vector<std::string> switchAddresses_;
    bool registered_{false};
};

std::string PjsipBackend::Impl::callWithSipCallId(const std::string &sipCallId) const
{
    if (sipCallId.empty()) {
        return {};
    }

    for (const auto &[ourId, call] : calls) {
        if (call != nullptr && call->sipCallId() == sipCallId) {
            return ourId;
        }
    }
    return {};
}

bool PjsipBackend::Impl::switchAsks(const std::string &source, const std::string &user,
                                    const std::string &sipCallId, core::SwitchRequest what)
{
    for (const auto &[accountId, account] : accounts) {
        if (account == nullptr) {
            continue;
        }
        if (!core::fromTheSwitch(source, account->switchAddresses())) {
            continue;
        }
        if (!user.empty() && !account->answersFor(user)) {
            continue;
        }

        core::Logger::instance().log(core::LogLevel::Info, "sip",
                                     "the switch asked account " + accountId + " to " +
                                         std::string{core::toString(what)});

        const auto id = accountId;
        const auto named = callWithSipCallId(sipCallId);
        loop.post([this, id, what, named]() {
            if (callObserver != nullptr) {
                callObserver->onSwitchRequest(id, what, named);
            }
        });
        return true;
    }

    core::Logger::instance().log(core::LogLevel::Info, "sip",
                                 "refused a " + std::string{core::toString(what)} +
                                     " request from " + source +
                                     ": no registered account takes orders from there");
    return false;
}

namespace {

std::atomic<PjsipBackend::Impl *> switchTarget{nullptr};

[[nodiscard]] std::string requestUser(const pjsip_msg *message)
{
    if (message == nullptr || message->type != PJSIP_REQUEST_MSG) {
        return {};
    }

    const auto *uri = static_cast<const pjsip_uri *>(
        pjsip_uri_get_uri(const_cast<pjsip_uri *>(message->line.req.uri)));
    if (uri == nullptr || (!PJSIP_URI_SCHEME_IS_SIP(uri) && !PJSIP_URI_SCHEME_IS_SIPS(uri))) {
        return {};
    }

    return text(reinterpret_cast<const pjsip_sip_uri *>(uri)->user);
}

pj_bool_t onUnsolicitedRequest(pjsip_rx_data *rdata)
{
    auto *target = switchTarget.load(std::memory_order_acquire);
    if (target == nullptr || rdata == nullptr || rdata->msg_info.msg == nullptr) {
        return PJ_FALSE;
    }

    auto *message = rdata->msg_info.msg;
    if (message->type != PJSIP_REQUEST_MSG) {
        return PJ_FALSE;
    }

    if (pjsip_method_cmp(&message->line.req.method, pjsip_get_cancel_method()) == 0) {
        if (core::saysCompletedElsewhere(headerValues(message, "Reason"))) {
            target->noteCompletedElsewhere(callIdOf(message));
        }
        return PJ_FALSE;
    }

    if (pjsip_method_cmp(&message->line.req.method, &kNotify) != 0) {
        return PJ_FALSE;
    }

    const auto inDialog = rdata->msg_info.to != nullptr && rdata->msg_info.to->tag.slen > 0;

    const auto events = headerValues(message, "Event");
    const auto asked = events.empty() ? std::nullopt : core::parseSwitchEvent(events.front());
    if (!asked.has_value()) {
        if (inDialog) {
            return PJ_FALSE;
        }
        static_cast<void>(pjsip_endpt_respond_stateless(
            pjsua_get_pjsip_endpt(), rdata, PJSIP_SC_BAD_EVENT, nullptr, nullptr, nullptr));
        return PJ_TRUE;
    }

    const std::string source{rdata->pkt_info.src_name};
    const auto obeyed = target->switchAsks(source, requestUser(message), callIdOf(message), *asked);

    static_cast<void>(pjsip_endpt_respond_stateless(pjsua_get_pjsip_endpt(), rdata,
                                                    obeyed ? PJSIP_SC_OK : PJSIP_SC_FORBIDDEN,
                                                    nullptr, nullptr, nullptr));
    return PJ_TRUE;
}

pjsip_module switchModule = {
    nullptr,
    nullptr,
    {const_cast<char *>("mod-softphone-switch"), 20},
    -1,
    PJSIP_MOD_PRIORITY_UA_PROXY_LAYER - 1,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    &onUnsolicitedRequest,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};

[[nodiscard]] std::string addressedUser(const pjsip_rx_data *rdata)
{
    if (rdata == nullptr || rdata->msg_info.to == nullptr) {
        return {};
    }

    const auto *uri = static_cast<const pjsip_uri *>(pjsip_uri_get_uri(
        const_cast<pjsip_uri *>(static_cast<const pjsip_uri *>(rdata->msg_info.to->uri))));
    if (uri != nullptr && (PJSIP_URI_SCHEME_IS_SIP(uri) || PJSIP_URI_SCHEME_IS_SIPS(uri))) {
        return text(reinterpret_cast<const pjsip_sip_uri *>(uri)->user);
    }

    return requestUser(rdata->msg_info.msg);
}

}

void PjsipBackend::Impl::noteCompletedElsewhere(const std::string &sipCallId)
{
    if (sipCallId.empty()) {
        return;
    }

    const std::lock_guard held{elsewhereLock};

    constexpr std::size_t kRemembered = 64;
    if (completedElsewhere.size() >= kRemembered) {
        completedElsewhere.erase(completedElsewhere.begin());
    }
    completedElsewhere.insert(sipCallId);
}

bool PjsipBackend::Impl::wasCompletedElsewhere(const std::string &sipCallId)
{
    if (sipCallId.empty()) {
        return false;
    }

    const std::lock_guard held{elsewhereLock};
    return completedElsewhere.erase(sipCallId) > 0;
}

int PjsipBackend::Impl::lineForRequest(pjsip_rx_data *rdata, int chosen)
{
    if (rdata == nullptr || accounts.size() < 2) {

        return chosen;
    }

    const std::string source{rdata->pkt_info.src_name};
    const auto user = addressedUser(rdata);

    std::vector<core::LineOnTheWire> lines;
    std::vector<const Account *> objects;
    lines.reserve(accounts.size());
    objects.reserve(accounts.size());
    for (const auto &[accountId, account] : accounts) {
        if (account == nullptr) {
            continue;
        }
        lines.push_back(
            core::LineOnTheWire{accountId, account->switchAddresses(), account->username()});
        objects.push_back(account.get());
    }

    const auto owner = core::lineThatOwns(source, user, lines);
    if (!owner.has_value()) {
        return chosen;
    }

    const auto id = objects[*owner]->getId();
    if (id == PJSUA_INVALID_ID || id == chosen) {
        return chosen;
    }

    core::Logger::instance().log(core::LogLevel::Info, "sip",
                                 "the invitation from " + source + " belongs to " +
                                     lines[*owner].id + " rather than the line the address named");
    return id;
}

PjsipBackend::PjsipBackend(core::EventLoop &loop, const core::ICredentialProvider &credentials,
                           Options options)
    : impl_{std::make_unique<Impl>(loop, credentials, options)}
{}

namespace {

constexpr unsigned kMediaSampleMsec = 500;

}

std::optional<core::CallMediaStats> PjsipBackend::Impl::readMediaOf(Call &call) const
{
    try {

        const auto about = call.getInfo();

        std::optional<unsigned> audio;
        for (std::size_t at = 0; at < about.media.size(); ++at) {
            const auto &medium = about.media[at];

            if (medium.type == PJMEDIA_TYPE_AUDIO && medium.status != PJSUA_CALL_MEDIA_NONE &&
                medium.status != PJSUA_CALL_MEDIA_ERROR) {
                audio = static_cast<unsigned>(at);
                break;
            }
        }
        if (!audio.has_value()) {

            return std::nullopt;
        }

        const auto stream = call.getStreamStat(*audio);

        core::CallMediaStats stats;

        stats.sent.packets = stream.rtcp.txStat.pkt;
        stats.sent.bytes = stream.rtcp.txStat.bytes;
        stats.sent.lost = stream.rtcp.txStat.loss;
        stats.sent.discarded = stream.rtcp.txStat.discard;
        stats.sent.jitterMeanUsec =
            static_cast<unsigned>(std::max(0, stream.rtcp.txStat.jitterUsec.mean));
        stats.sent.jitterMaxUsec =
            static_cast<unsigned>(std::max(0, stream.rtcp.txStat.jitterUsec.max));

        stats.received.packets = stream.rtcp.rxStat.pkt;
        stats.received.bytes = stream.rtcp.rxStat.bytes;
        stats.received.lost = stream.rtcp.rxStat.loss;
        stats.received.discarded = stream.rtcp.rxStat.discard;
        stats.received.jitterMeanUsec =
            static_cast<unsigned>(std::max(0, stream.rtcp.rxStat.jitterUsec.mean));
        stats.received.jitterMaxUsec =
            static_cast<unsigned>(std::max(0, stream.rtcp.rxStat.jitterUsec.max));

        stats.roundTripMeanUsec = static_cast<unsigned>(std::max(0, stream.rtcp.rttUsec.mean));
        stats.roundTripMaxUsec = static_cast<unsigned>(std::max(0, stream.rtcp.rttUsec.max));

        const auto info = call.getStreamInfo(*audio);
        stats.codec = info.codecName;
        stats.clockRate = info.codecClockRate;
        stats.sentPayloadType = info.txPt;
        stats.receivedPayloadType = info.rxPt;

        const auto transport = call.getMedTransportInfo(*audio);
        stats.localAddress = transport.localRtpName;
        stats.remoteAddress =
            transport.srcRtpName.empty() ? info.remoteRtpAddress : transport.srcRtpName;

        pjmedia_transport_info mediaTransport;
        pjmedia_transport_info_init(&mediaTransport);
        if (pjsua_call_get_med_transport_info(call.getId(), *audio, &mediaTransport) ==
            PJ_SUCCESS) {
            const auto *srtp = static_cast<const pjmedia_srtp_info *>(
                pjmedia_transport_info_get_spc_info(&mediaTransport, PJMEDIA_TRANSPORT_TYPE_SRTP));
            stats.encrypted = srtp != nullptr && srtp->active != PJ_FALSE;
        }

        return stats;
    } catch (const pj::Error &) {

        return std::nullopt;
    }
}

void PjsipBackend::Impl::armTheMediaSampler()
{
    if (!samplingMedia.load(std::memory_order_acquire)) {
        return;
    }

    const auto scheduled = pjsua_schedule_timer2(
        [](void *user) {
            auto *const impl = static_cast<Impl *>(user);
            if (impl == nullptr || !impl->samplingMedia.load(std::memory_order_acquire)) {
                return;
            }
            impl->sampleEveryCallsMedia();
            impl->armTheMediaSampler();
        },
        this, kMediaSampleMsec);

    if (scheduled != PJ_SUCCESS) {
        core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                     "the media sampler could not be scheduled; the diagnostics "
                                     "window will show the last reading it has");
    }
}

void PjsipBackend::Impl::sampleTheMediaOf(const std::string &callId, Call &call)
{
    auto reading = readMediaOf(call);

    const std::lock_guard held{mediaStatsLock};
    if (reading) {
        mediaStats[callId] = std::move(*reading);
    } else {
        mediaStats.erase(callId);
    }
}

void PjsipBackend::Impl::sampleEveryCallsMedia()
{

    std::map<std::string, core::CallMediaStats> fresh;

    pjsua_call_id ids[PJSUA_MAX_CALLS];
    unsigned howMany = PJSUA_MAX_CALLS;
    if (pjsua_enum_calls(ids, &howMany) != PJ_SUCCESS) {
        return;
    }

    for (unsigned at = 0; at < howMany; ++at) {
        auto *const known = pj::Call::lookup(ids[at]);
        if (known == nullptr) {
            continue;
        }

        auto *const call = static_cast<Call *>(known);
        if (auto reading = readMediaOf(*call)) {
            fresh.emplace(call->ourCallId(), std::move(*reading));
        }
    }

    const std::lock_guard held{mediaStatsLock};
    mediaStats = std::move(fresh);
}

void PjsipBackend::Impl::forgetMediaOf(const std::string &callId)
{
    const std::lock_guard held{mediaStatsLock};
    mediaStats.erase(callId);
}

PjsipBackend::~PjsipBackend()
{

    switchNamed.store(nullptr, std::memory_order_release);
    stop();
}

void PjsipBackend::start()
{
    if (impl_->running) {
        return;
    }

    try {

        pj_log_set_log_func(&writePjsipLog);
        pj_log_set_level(impl_->options.logLevel);

        if (pj_init() == PJ_SUCCESS && pj_thread_is_registered() == PJ_FALSE) {

            impl_->stackThread = std::make_unique<pj_thread_desc[]>(1);
            pj_thread_t *registered = nullptr;
            static_cast<void>(
                pj_thread_register("softphone-stack", impl_->stackThread[0], &registered));
        }

        impl_->endpoint = std::make_unique<PjsipBackend::Endpoint>();
        impl_->endpoint->libCreate();

        impl_->endpoint->chooseTheLine = [this](pjsip_rx_data *rdata, int chosen) {
            return impl_->lineForRequest(rdata, chosen);
        };

        impl_->endpoint->onDeviceLost = [this](core::DeviceRole role) {
            core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                         std::string{"the "} + std::string{toString(role)} +
                                             " device stopped working");

            if (impl_->recovering.exchange(true)) {
                return;
            }

            impl_->loop.post([this, role]() {
                impl_->recovering = false;
                if (impl_->deviceLost) {
                    impl_->deviceLost(role);
                }
            });
        };

        impl_->watchingTheMachine = watchTheMachinesDevices(
            [this]() { impl_->loop.post([this]() { impl_->sayTheMachineMoved(); }); });

        impl_->endpoint->onTransportGone = [this](const std::string &kind,
                                                  const std::string &farEnd,
                                                  std::vector<std::string> stranded) {

            impl_->loop.post([kind, farEnd]() {
                core::Logger::instance().log(
                    core::LogLevel::Warning, "sip",
                    "the " + kind + " connection to " +
                        (farEnd.empty() ? std::string{"the switch"} : farEnd) +
                        " went away; a call still being set up on it "
                        "will not complete");
            });

            static_cast<void>(impl_->loop.scheduleAfter(
                std::chrono::seconds{1},
                [this, stranded = std::move(stranded)]() { endCallsStrandedBy(stranded); }));
        };

        pj::EpConfig endpointConfig;
        endpointConfig.logConfig.level = static_cast<unsigned>(impl_->options.logLevel);

        endpointConfig.uaConfig.userAgent = std::string{core::userAgent()};

        endpointConfig.logConfig.consoleLevel = PJ_LOG_MAX_LEVEL;

        impl_->logWriter = new PjsipLogWriter{};
        endpointConfig.logConfig.writer = impl_->logWriter;

        endpointConfig.medConfig.noVad = impl_->options.sendsWhileSilent;

        impl_->endpoint->libInit(endpointConfig);

#if defined(SOFTPHONE_HAS_PULSE)

        if (softphone_pulse_aud_available() != 0) {
            if (const auto status = pjmedia_aud_register_factory(&softphone_pulse_aud_factory);
                status == PJ_SUCCESS) {
                static_cast<void>(pjmedia_aud_unregister_factory(&pjmedia_alsa_factory));
                core::Logger::instance().log(core::LogLevel::Info, "sip",
                                             "sound devices come from the sound server");
            } else {
                core::Logger::instance().log(
                    core::LogLevel::Warning, "sip",
                    "the sound server's devices could not be registered; ALSA's are what "
                    "this phone will show");
            }
        }
#endif

        if (traceModule.id < 0) {
            static_cast<void>(pjsip_endpt_register_module(pjsua_get_pjsip_endpt(), &traceModule));
        }
        if (switchModule.id < 0) {
            static_cast<void>(pjsip_endpt_register_module(pjsua_get_pjsip_endpt(), &switchModule));
        }
        if (identityModule.id < 0) {
            static_cast<void>(
                pjsip_endpt_register_module(pjsua_get_pjsip_endpt(), &identityModule));
        }
        switchTarget.store(impl_.get(), std::memory_order_release);

        pj::TransportConfig transportConfig;
        transportConfig.port = impl_->options.localPort;

        const auto udpTransport =
            impl_->endpoint->transportCreate(PJSIP_TRANSPORT_UDP, transportConfig);

        auto tcpConfig = transportConfig;
        tcpConfig.port = portFrom(impl_->endpoint->transportGetInfo(udpTransport).localName);

        try {
            impl_->tcpTransport = impl_->endpoint->transportCreate(PJSIP_TRANSPORT_TCP, tcpConfig);
        } catch (const pj::Error &error) {
            core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                         "could not bind TCP to the same port as UDP (" +
                                             std::to_string(tcpConfig.port) + "): " + error.info() +
                                             "; taking any free port instead");
            impl_->tcpTransport =
                impl_->endpoint->transportCreate(PJSIP_TRANSPORT_TCP, transportConfig);
        }

        const auto tlsListener = [this](bool verifying) -> pj::TransportId {
            pj::TransportConfig tlsConfig;
            tlsConfig.tlsConfig.verifyServer = verifying;
            std::string trust;
            if (verifying) {
                trust = trustTheMachine(tlsConfig.tlsConfig, impl_->options.certificateAuthority);
            }

            try {
                const auto id = impl_->endpoint->transportCreate(PJSIP_TRANSPORT_TLS, tlsConfig);
                if (verifying) {
                    core::Logger::instance().log(core::LogLevel::Info, "sip",
                                                 "TLS through " + std::string{tlsImplementation()} +
                                                     ", " + trust);
                }
                return id;
            } catch (const pj::Error &error) {
                core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                             "TLS is not available in this build: " + error.info());
                return PJSUA_INVALID_ID;
            }
        };
        impl_->tlsVerifying = tlsListener(true);
        if (impl_->tlsVerifying != PJSUA_INVALID_ID) {
            impl_->tlsTrusting = tlsListener(false);
        }

        impl_->endpoint->libStart();

        auto &devices = impl_->endpoint->audDevManager();
        if (impl_->options.useNullAudioDevice) {
            devices.setNullDev();
            core::Logger::instance().log(core::LogLevel::Info, "sip",
                                         "audio: null device (no hardware in use)");
        } else if (devices.getDevCount() == 0) {

            impl_->options.useNullAudioDevice = true;
            devices.setNullDev();
            core::Logger::instance().log(
                core::LogLevel::Warning, "sip",
                "audio: this machine has no sound device; the null device is in use and calls "
                "will be silent");
        } else {

            try {

                const auto capture = devices.getDevInfo(devices.getCaptureDev());
                const auto playback = devices.getDevInfo(devices.getPlaybackDev());

                core::Logger::instance().log(
                    core::LogLevel::Info, "sip",
                    "audio: capture '" + asText(capture.name) + "' (" + capture.driver +
                        "), playback '" + asText(playback.name) + "' (" + playback.driver + ")");
            } catch (const pj::Error &error) {

                impl_->options.useNullAudioDevice = true;
                devices.setNullDev();
                core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                             "audio: no usable device (" + error.info() +
                                                 "); the null device is in use and calls will be "
                                                 "silent");
            }
        }

        const auto info = impl_->endpoint->transportGetInfo(udpTransport);
        impl_->boundPort = portFrom(info.localName);

        impl_->running = true;

        impl_->samplingMedia.store(true, std::memory_order_release);
        impl_->armTheMediaSampler();

        impl_->tuneSignals();
    } catch (const pj::Error &error) {
        impl_->endpoint.reset();
        throw std::runtime_error{"failed to start the SIP stack: " + error.info()};
    }
}

void PjsipBackend::stop()
{

    setTraceSink(nullptr);

    CancellerStarvation::flush();

    impl_->samplingMedia.store(false, std::memory_order_release);

    impl_->watchingTheMachine.reset();

    switchTarget.store(nullptr, std::memory_order_release);

    DialogTransports::forgetEverything();

    if (!impl_->endpoint) {
        return;
    }

    impl_->ensureThreadRegistered();

    stopTone();
    impl_->closeMeter();
    impl_->closeTap(impl_->localTap);
    impl_->closeTap(impl_->remoteTap);

    std::vector<std::string> announcing;
    announcing.reserve(impl_->notices.size());
    for (const auto &notice : impl_->notices) {
        announcing.push_back(notice.first);
    }
    for (const std::string &noticeId : announcing) {
        stopAnnouncing(noticeId);
    }

    impl_->closePlayback();

    for (auto &recording : impl_->recordings) {
        impl_->closeTap(recording.second.nearTap);
        impl_->closeTap(recording.second.farTap);
        for (auto &stem : recording.second.stemTaps) {
            impl_->closeTap(stem.second);
        }
    }
    impl_->watchedCall.clear();

    std::vector<std::string> watched;
    watched.reserve(impl_->watches.size());
    for (const auto &entry : impl_->watches) {
        watched.push_back(entry.first);
    }
    for (const auto &watchId : watched) {
        stopWatching(watchId);
    }

    impl_->calls.clear();
    {
        const std::lock_guard held{impl_->mediaStatsLock};
        impl_->mediaStats.clear();
    }

    for (auto &watch : impl_->retiredWatches) {
        watch->releaseFromPjsua();
    }
    for (auto &[watchId, watch] : impl_->watches) {
        watch->releaseFromPjsua();
    }

    impl_->accounts.clear();

    try {
        impl_->endpoint->libStopWorkerThreads();
    } catch (const pj::Error &) {

    }

    impl_->retiredWatches.clear();
    impl_->watches.clear();

    try {
        impl_->endpoint->libDestroy();
    } catch (const pj::Error &) {

    }

    impl_->endpoint.reset();
    impl_->running = false;
    impl_->boundPort = 0;

    impl_->tcpTransport = PJSUA_INVALID_ID;
    impl_->tlsVerifying = PJSUA_INVALID_ID;
    impl_->tlsTrusting = PJSUA_INVALID_ID;
}

bool PjsipBackend::running() const noexcept
{
    return impl_->running;
}

void PjsipBackend::setTraceSink(core::ISipTraceSink *sink) noexcept
{

    traceSink.store(sink, std::memory_order_release);
}

void PjsipBackend::setSwitchListener(SwitchNamed told)
{
    switchNamed_ = std::move(told);
    switchNamed.store(switchNamed_ ? &switchNamed_ : nullptr, std::memory_order_release);
}

std::uint16_t PjsipBackend::boundPort() const noexcept
{
    return impl_->boundPort;
}

void PjsipBackend::setObserver(core::ISipBackend::Observer *observer)
{
    impl_->observer = observer;
}

void PjsipBackend::setCallObserver(core::ICallBackend::Observer *observer)
{
    impl_->callObserver = observer;
}

core::ICallBackend::Observer *PjsipBackend::callObserver() const noexcept
{
    return impl_->callObserver;
}

void PjsipBackend::noteSeance(const std::string &callId, const std::string &seanceId)
{

    if (const auto it = impl_->calls.find(callId); it != impl_->calls.end()) {
        it->second->setSeance(seanceId);
        return;
    }
    impl_->pendingSeances[callId] = seanceId;
}

void PjsipBackend::placeCall(const std::string &callId, const std::string &accountId,
                             const std::string &target)
{
    std::string seance;
    if (const auto pending = impl_->pendingSeances.find(callId);
        pending != impl_->pendingSeances.end()) {
        seance = std::move(pending->second);
        impl_->pendingSeances.erase(pending);
    }

    const auto account = impl_->accounts.find(accountId);
    if (!impl_->running || account == impl_->accounts.end()) {

        noteCall(core::LogLevel::Warning, callId, accountId,
                 std::string{"cannot place it: "} + (impl_->running
                                                         ? "this line is not registered"
                                                         : "the SIP stack is still starting"));
        impl_->publishCallProgress(core::CallProgress{callId, core::BackendCallState::Disconnected,
                                                      0, "no registered account", false});
        return;
    }

    impl_->ensureThreadRegistered();

    try {
        noteCall(core::LogLevel::Info, callId, accountId, "placing a call to " + target);

        auto call = std::make_unique<Call>(*impl_, *account->second, accountId, callId);
        call->setSeance(seance);

        auto options = audioOnlyOptions();
        addSeanceHeader(options, seance);
        call->makeCall(target, options);

        impl_->calls[callId] = std::move(call);
    } catch (const pj::Error &error) {
        impl_->forgetMediaOf(callId);
        impl_->calls.erase(callId);

        const auto reason = error.info();
        noteCall(core::LogLevel::Warning, callId, accountId, "could not place it: " + reason);
        impl_->publishCallProgress(core::CallProgress{callId, core::BackendCallState::Disconnected,
                                                      0, reason, false, localFailureIn(reason)});
    }
}

void PjsipBackend::answerCall(const std::string &callId)
{
    const auto it = impl_->calls.find(callId);
    if (it == impl_->calls.end()) {
        noteCall(core::LogLevel::Warning, callId, "asked to answer a call the stack has not got");
        return;
    }

    noteCall(core::LogLevel::Info, callId, "answering");

    impl_->ensureThreadRegistered();

    try {

        auto options = audioOnlyOptions();
        options.statusCode = PJSIP_SC_OK;
        addSeanceHeader(options, it->second->seance());
        it->second->answer(options);
    } catch (const pj::Error &error) {

        const auto reason = error.info();
        noteCall(core::LogLevel::Warning, callId, "could not answer it: " + reason);
        impl_->publishCallProgress(core::CallProgress{callId, core::BackendCallState::Disconnected,
                                                      0, reason, false, localFailureIn(reason)});
    }
}

void PjsipBackend::refuseIncomingCalls(bool refusing)
{
    impl_->refusingIncoming.store(refusing, std::memory_order_relaxed);
}

void PjsipBackend::refuseSecondCalls(bool refusing)
{
    impl_->refusingSecond.store(refusing, std::memory_order_relaxed);
}

void PjsipBackend::rejectCall(const std::string &callId, int sipStatus)
{
    const auto it = impl_->calls.find(callId);
    if (it == impl_->calls.end()) {
        noteCall(core::LogLevel::Warning, callId, "asked to reject a call the stack has not got");
        return;
    }

    noteCall(core::LogLevel::Info, callId, "rejecting it with " + std::to_string(sipStatus));

    impl_->ensureThreadRegistered();

    try {
        it->second->markEndedLocally();

        pj::CallOpParam options;
        options.statusCode = static_cast<pjsip_status_code>(sipStatus);
        it->second->hangup(options);
    } catch (const pj::Error &error) {

        noteCall(core::LogLevel::Warning, callId, "the reject was refused: " + error.info());
    }
}

void PjsipBackend::hangUpCall(const std::string &callId)
{
    const auto it = impl_->calls.find(callId);
    if (it == impl_->calls.end()) {
        noteCall(core::LogLevel::Warning, callId, "asked to hang up a call the stack has not got");
        return;
    }

    noteCall(core::LogLevel::Info, callId, "hanging up");

    impl_->ensureThreadRegistered();

    try {
        it->second->markEndedLocally();

        pj::CallOpParam options;
        options.statusCode = PJSIP_SC_DECLINE;
        it->second->hangup(options);
    } catch (const pj::Error &error) {
        noteCall(core::LogLevel::Warning, callId, "the hang up was refused: " + error.info());
    }
}

void PjsipBackend::endCallsStrandedBy(const std::vector<std::string> &dialogs)
{
    if (dialogs.empty() || !impl_->running) {
        return;
    }

    impl_->ensureThreadRegistered();

    for (const auto &[callId, call] : impl_->calls) {
        pj::CallInfo info;
        try {
            info = call->getInfo();
        } catch (const pj::Error &) {
            continue;
        }

        if (std::find(dialogs.begin(), dialogs.end(), info.callIdString) == dialogs.end()) {
            continue;
        }

        if (info.state == PJSIP_INV_STATE_CONFIRMED || info.state == PJSIP_INV_STATE_DISCONNECTED) {
            continue;
        }

        noteCall(core::LogLevel::Warning, callId,
                 "the connection it was being set up on went away; ending it rather than "
                 "waiting out the ring timeout");

        call->markStrandedByTransport();

        impl_->publishCallProgress(core::CallProgress{callId, core::BackendCallState::Disconnected,
                                                      0, "the signalling connection went away",
                                                      false, core::LocalFailure::None});

        try {

            pj::CallOpParam options;
            options.statusCode = PJSIP_SC_REQUEST_TERMINATED;
            call->hangup(options);
        } catch (const pj::Error &error) {
            noteCall(core::LogLevel::Warning, callId,
                     "the stranded call would not end: " + error.info());
        }
    }
}

void PjsipBackend::setToneEndedCallback(std::function<void()> callback)
{
    impl_->toneEnded = std::move(callback);
}

void PjsipBackend::setSoundDeviceWantedCallback(SoundDeviceWanted callback)
{
    impl_->soundDeviceWanted = std::move(callback);
}

void PjsipBackend::startTone(const std::string &wavPath, bool loop, core::ToneOutput output,
                             core::ToneLevel level)
{
    if (!impl_->running || impl_->options.useNullAudioDevice) {
        return;
    }

    stopTone();

    impl_->ensureThreadRegistered();

    Impl::OneReason reason{*impl_};

    try {

        impl_->onToneFinished = loop ? std::function<void()>{} : [this]() {
            stopTone();
            if (impl_->toneEnded) {
                impl_->toneEnded();
            }
        };

        auto player = std::make_unique<TonePlayer>([this]() { impl_->toneFinished(); });

        const unsigned playerOptions = loop ? 0U : static_cast<unsigned>(PJMEDIA_FILE_NO_LOOP);

        player->createPlayer(core::forNarrowFileApi(core::fromUtf8(wavPath)), playerOptions);

        player->adjustTxLevel(impl_->gainFor(level));

        player->startTransmit(*impl_->toneDestination(output));

        impl_->tonePlayer = std::move(player);
        impl_->playing = Impl::PlayingTone{wavPath, loop, output, level};
        reason.keep();
        core::Logger::instance().log(core::LogLevel::Info, "sip",
                                     "playing tone " + wavPath + " on the " +
                                         (impl_->ringerDevice ? "ringer" : "speakers"));
    } catch (const pj::Error &error) {

        core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                     "could not play a tone: " + error.info());
        impl_->closeRinger();
    }
}

void PjsipBackend::stopTone()
{
    if (!impl_->tonePlayer) {

        impl_->closeRinger();
        return;
    }

    impl_->ensureThreadRegistered();

    try {

        if (impl_->ringerDevice) {
            impl_->tonePlayer->stopTransmit(*impl_->ringerDevice);
        } else {
            impl_->tonePlayer->stopTransmit(impl_->endpoint->audDevManager().getPlaybackDevMedia());
        }
    } catch (const pj::Error &) {

    }

    impl_->tonePlayer.reset();
    impl_->onToneFinished = nullptr;
    impl_->playing.reset();
    impl_->closeRinger();

    impl_->wantTheSoundDevice(false);
    core::Logger::instance().log(core::LogLevel::Info, "sip", "stopped the tone");
}

bool PjsipBackend::ringerOpen() const noexcept
{
    return impl_->ringerDevice != nullptr;
}

std::vector<core::AudioDeviceInfo> PjsipBackend::devices()
{
    std::vector<core::AudioDeviceInfo> devices;
    if (!impl_->running) {
        return devices;
    }

    impl_->ensureThreadRegistered();

    if (pjsua_snd_is_active() && !impl_->onlyTheMeterHoldsTheDevice()) {
        core::Logger::instance().log(core::LogLevel::Info, "sip",
                                     "not re-reading the sound devices while a call has one open; "
                                     "offering the " +
                                         std::to_string(impl_->lastDevices.size()) + " last seen");
        return impl_->lastDevices;
    }

    try {
        auto &manager = impl_->endpoint->audDevManager();

        manager.refreshDevs();

        const auto found = manager.enumDev2();
        devices.reserve(found.size());

        for (const auto &device : found) {
            devices.push_back(core::AudioDeviceInfo{
                device.driver, device.name, device.inputCount, device.outputCount, {}, {}});
        }

        devices = described(std::move(devices));

    } catch (const pj::Error &error) {
        core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                     "could not enumerate the sound devices: " + error.info());
        return impl_->lastDevices;
    }

    impl_->lastDevices = devices;
    return devices;
}

void PjsipBackend::setDeviceLostCallback(core::IAudioDeviceBackend::DeviceLostCallback callback)
{
    impl_->deviceLost = std::move(callback);
}

void PjsipBackend::setDevicesChangedCallback(
    core::IAudioDeviceBackend::DevicesChangedCallback callback)
{
    impl_->devicesChanged = std::move(callback);
}

std::string PjsipBackend::systemDefaultDevice(core::DeviceRole role) const
{
    return systemDefaultDeviceName(role).value_or(std::string{});
}

void PjsipBackend::applyVolumes(const core::Volumes &volumes)
{
    impl_->volumes = volumes;

    if (!impl_->running || impl_->options.useNullAudioDevice) {

        return;
    }

    impl_->ensureThreadRegistered();

    try {
        pj::Endpoint::instance().audDevManager().getCaptureDevMedia().adjustTxLevel(
            core::volumeGain(volumes.microphone));
    } catch (const pj::Error &error) {
        core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                     "could not set the microphone volume: " + error.info());
    }

    for (auto &[id, call] : impl_->calls) {
        static_cast<void>(id);
        call->applyConversationLevel();
    }

    impl_->refreshSignalTaps();
    impl_->refreshRecordingTaps();

    if (impl_->tonePlayer && impl_->playing.has_value()) {
        try {
            impl_->tonePlayer->adjustTxLevel(impl_->gainFor(impl_->playing->level));
        } catch (const pj::Error &error) {
            core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                         "could not set the tone volume: " + error.info());
        }
    }

    core::Logger::instance().log(core::LogLevel::Debug, "sip",
                                 "volumes: microphone " + std::to_string(volumes.microphone) +
                                     ", speakers " + std::to_string(volumes.speakers) + ", ring " +
                                     std::to_string(volumes.ring) + ", other " +
                                     std::to_string(volumes.notifications));
}

core::AudioApplyResult PjsipBackend::applyAudio(const core::AudioSettings &audio)
{

    Impl::OneReason reason{*impl_};

    const auto available = devices();

    core::AudioApplyResult result;
    result.microphone = core::chooseDevice(audio.microphone, available, core::DeviceRole::Capture);
    result.speakers = core::chooseDevice(audio.speakers, available, core::DeviceRole::Playback);
    result = core::planRinger(audio.ringer, available, std::move(result));

    if (!impl_->running) {
        return result;
    }

    if (impl_->options.useNullAudioDevice) {
        core::Logger::instance().log(core::LogLevel::Info, "sip",
                                     "sound device choice ignored: the null device is in use");
        return result;
    }

    impl_->ensureThreadRegistered();

    auto &manager = impl_->endpoint->audDevManager();

    const auto point = [&](core::DeviceSelection &selection, core::DeviceRole role) {

        const auto preferred = systemDefaultPosition(available, role);
        const int fallback =
            preferred ? static_cast<int>(*preferred)
                      : (role == core::DeviceRole::Capture ? PJMEDIA_AUD_DEFAULT_CAPTURE_DEV
                                                           : PJMEDIA_AUD_DEFAULT_PLAYBACK_DEV);

        const bool usable = selection.position && *selection.position < available.size();
        const int index = usable ? static_cast<int>(*selection.position) : fallback;

        if (selection.position && !usable) {
            selection.use = core::DeviceUse::Missing;
            selection.position.reset();
        }

        try {
            if (role == core::DeviceRole::Capture) {
                manager.setCaptureDev(index);
            } else {
                manager.setPlaybackDev(index);
            }
        } catch (const pj::Error &error) {

            core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                         std::string{"could not open the "} +
                                             std::string{toString(role)} +
                                             " device: " + error.info());
            if (selection.position) {
                selection.use = core::DeviceUse::Missing;
                selection.position.reset();
                try {
                    if (role == core::DeviceRole::Capture) {
                        manager.setCaptureDev(fallback);
                    } else {
                        manager.setPlaybackDev(fallback);
                    }
                } catch (const pj::Error &) {

                }
            }
        }
    };

    point(result.microphone, core::DeviceRole::Capture);
    point(result.speakers, core::DeviceRole::Playback);

    const bool ringerUsable = result.ringerOnItsOwnDevice && result.ringer.position &&
                              *result.ringer.position < available.size();
    if (result.ringerOnItsOwnDevice && !ringerUsable) {
        result.ringerOnItsOwnDevice = false;
        result.ringer.use = core::DeviceUse::Missing;
        result.ringer.position.reset();
    }

    const int wantedRinger = ringerUsable ? static_cast<int>(*result.ringer.position) : -1;
    if (wantedRinger != impl_->ringerIndex) {

        impl_->closeRinger();
        impl_->ringerIndex = wantedRinger;
    }

    core::Logger::instance().log(core::LogLevel::Info, "sip",
                                 std::string{"microphone "} +
                                     std::string{toString(result.microphone.use)} + ", speakers " +
                                     std::string{toString(result.speakers.use)} + ", ringer " +
                                     std::string{toString(result.ringer.use)} +
                                     (result.ringerOnItsOwnDevice ? " on its own device" : ""));

    replayTone();

    return result;
}

void PjsipBackend::replayTone()
{

    if (!impl_->playing || !impl_->playing->loop) {
        return;
    }

    const auto again = *impl_->playing;
    startTone(again.path, again.loop, again.output, again.level);
}

std::vector<core::CodecInfo> PjsipBackend::codecs()
{
    if (!impl_->endpoint) {
        return {};
    }

    impl_->ensureThreadRegistered();

    try {
        auto listed = impl_->endpoint->codecEnum2();

        std::stable_sort(listed.begin(), listed.end(),
                         [](const pj::CodecInfo &left, const pj::CodecInfo &right) {
                             return left.priority > right.priority;
                         });

        std::vector<core::CodecInfo> codecs;
        codecs.reserve(listed.size());
        for (const auto &entry : listed) {
            codecs.push_back(core::describeCodec(entry.codecId));
        }
        return codecs;
    } catch (const pj::Error &error) {
        core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                     "the codec list could not be read: " + error.info());
        return {};
    }
}

void PjsipBackend::applyCodecs(std::span<const core::CodecChoice> wanted)
{
    if (!impl_->endpoint) {
        return;
    }

    impl_->ensureThreadRegistered();

    constexpr unsigned kHighest = 254;
    constexpr unsigned kStep = 2;

    std::string offered;
    unsigned position = 0;
    for (const auto &choice : wanted) {
        const auto priority =
            choice.enabled
                ? static_cast<unsigned>(std::max<int>(1, static_cast<int>(kHighest) -
                                                             static_cast<int>(position * kStep)))
                : 0U;

        try {
            impl_->endpoint->codecSetPriority(choice.id, static_cast<pj_uint8_t>(priority));
        } catch (const pj::Error &error) {
            core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                         "the stack would not take codec " + choice.id + ": " +
                                             error.info());
            continue;
        }

        if (choice.enabled) {
            offered += offered.empty() ? "" : ", ";
            offered += choice.id;
        }
        ++position;
    }

    core::Logger::instance().log(core::LogLevel::Info, "sip",
                                 "offering " +
                                     (offered.empty() ? std::string{"nothing"} : offered));
}

void PjsipBackend::startMetering()
{
    if (!impl_->running) {
        return;
    }
    impl_->openMeter();
}

void PjsipBackend::stopMetering()
{
    impl_->closeMeter();
}

bool PjsipBackend::metering() const noexcept
{
    return impl_->meterPort != nullptr;
}

std::optional<core::AudioLevels> PjsipBackend::audioLevels() const
{
    if (!impl_->running || impl_->options.useNullAudioDevice) {
        return std::nullopt;
    }

    impl_->ensureThreadRegistered();

    unsigned toTheSpeakers = 0;
    unsigned fromTheMicrophone = 0;
    if (pjsua_conf_get_signal_level(0, &toTheSpeakers, &fromTheMicrophone) != PJ_SUCCESS) {
        return std::nullopt;
    }

    return core::AudioLevels{fromTheMicrophone, toTheSpeakers};
}

std::optional<float> PjsipBackend::microphoneLevelOnThePort() const
{
    if (!impl_->running || impl_->options.useNullAudioDevice) {
        return std::nullopt;
    }

    impl_->ensureThreadRegistered();

    pjsua_conf_port_info slot{};
    if (pjsua_conf_get_port_info(0, &slot) != PJ_SUCCESS) {
        return std::nullopt;
    }

    return slot.rx_level_adj;
}

void PjsipBackend::Impl::closeNotice(const std::string &noticeId)
{
    Notice *notice = noticeFor(noticeId);
    if (notice == nullptr || !notice->player) {
        return;
    }

    ensureThreadRegistered();

    for (const std::string &callId : notice->targetCalls) {
        const auto found = calls.find(callId);
        if (found == calls.end()) {

            continue;
        }

        const auto slot = found->second->audioSlot();
        if (slot == PJSUA_INVALID_ID) {
            continue;
        }

        static_cast<void>(pjsua_conf_disconnect(
            static_cast<pjsua_conf_port_id>(notice->player->getPortId()), slot));
    }

    if (notice->toTheSpeakers) {
        try {
            notice->player->stopTransmit(endpoint->audDevManager().getPlaybackDevMedia());
        } catch (const pj::Error &) {

        }
    }

    notice->targetCalls.clear();
    notice->toTheSpeakers = false;
    notice->player.reset();

    if (const auto session = recordings.find(noticeId); session != recordings.end()) {
        refreshRecordingTaps(session->second);
    }
}

PjsipBackend::Impl::Played PjsipBackend::Impl::playNotice(const std::string &noticeId,
                                                          const std::vector<std::string> &callIds,
                                                          const std::vector<std::string> &files)
{
    closeNotice(noticeId);

    Notice &notice = notices[noticeId];

    if (!running) {
        return Played::No;
    }

    std::vector<std::string> candidates;
    for (const std::string &wanted : files) {
        if (!wanted.empty()) {
            candidates.push_back(wanted);
        }
    }
    if (candidates.empty()) {
        return Played::No;
    }

    ensureThreadRegistered();

    struct Destination {
        std::string callId;
        pjsua_conf_port_id slot;
    };

    std::vector<Destination> targets;
    for (const auto &callId : callIds) {
        const auto found = calls.find(callId);
        if (found == calls.end()) {
            continue;
        }
        if (const auto slot = found->second->audioSlot(); slot != PJSUA_INVALID_ID) {
            targets.push_back({callId, slot});
        }
    }

    if (targets.empty()) {
        return Played::NotYet;
    }

    if (!options.useNullAudioDevice) {

        targets.push_back({std::string{}, 0});
    }

    try {
        auto player =
            std::make_unique<TonePlayer>([this, noticeId]() { noticeFinished(noticeId); });

        bool opened = false;
        for (const std::string &candidate : candidates) {
            try {
                player->createPlayer(core::forNarrowFileApi(core::fromUtf8(candidate)),
                                     static_cast<unsigned>(PJMEDIA_FILE_NO_LOOP));
                opened = true;
                break;
            } catch (const pj::Error &error) {
                core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                             "the recording notice " + candidate +
                                                 " would not open: " + error.info());
            }
        }
        if (!opened) {
            closeNotice(noticeId);
            return Played::No;
        }

        player->adjustTxLevel(gainFor(core::ToneLevel::Notifications));

        for (const Destination &target : targets) {
            static_cast<void>(pjsua_conf_connect(
                static_cast<pjsua_conf_port_id>(player->getPortId()), target.slot));
            if (target.callId.empty()) {
                notice.toTheSpeakers = true;
            } else {
                notice.targetCalls.push_back(target.callId);
            }
        }

        notice.player = std::move(player);

        if (notice.intoTheRecording) {
            if (const auto session = recordings.find(noticeId); session != recordings.end()) {
                refreshRecordingTaps(session->second);
            }
        }
    } catch (const pj::Error &error) {
        core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                     "the recording notice would not play: " + error.info());
        closeNotice(noticeId);
        return Played::No;
    }

    return Played::Yes;
}

void PjsipBackend::Impl::waitForAPort(const std::string &noticeId)
{
    Notice *notice = noticeFor(noticeId);
    if (notice == nullptr) {
        return;
    }

    notice->waiting = true;
    notice->timer =
        loop.scheduleAfter(kNoticeRetry, [this, noticeId]() { tryTheNoticeAgain(noticeId); });
}

void PjsipBackend::Impl::tryTheNoticeAgain(const std::string &noticeId)
{
    Notice *notice = noticeFor(noticeId);
    if (notice == nullptr || !notice->waiting) {
        return;
    }

    notice->timer = core::IScheduler::TimerId{};

    const auto legs = notice->calls;
    const auto files = notice->files;

    switch (playNotice(noticeId, legs, files)) {
    case Played::Yes:
        if (Notice *started = noticeFor(noticeId); started != nullptr) {
            started->waiting = false;
        }
        core::Logger::instance().log(core::LogLevel::Info, "sip",
                                     "the conversation is back: playing the recording notice to " +
                                         std::to_string(legs.size()) + " leg(s)");
        noticeBegan(noticeId);
        break;

    case Played::NotYet:
        waitForAPort(noticeId);
        break;

    case Played::No:
        if (Notice *failed = noticeFor(noticeId); failed != nullptr) {
            failed->waiting = false;
            auto answer = std::exchange(failed->onProgress, nullptr);
            if (answer) {
                answer(core::NoticeProgress::Failed);
            }
        }
        break;
    }
}

unsigned PjsipBackend::playbackRate() const noexcept
{
    return impl_->signalRate;
}

void PjsipBackend::playFrom(core::IPlaybackSource *source)
{

    Impl::OneReason reason{*impl_};

    impl_->closePlayback();

    if (source == nullptr || !impl_->running || impl_->options.useNullAudioDevice) {
        return;
    }

    impl_->ensureThreadRegistered();

    auto complain = [](const std::string &operation, pj_status_t status) {
        core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                     "the recording will not play: " + operation + " " +
                                         std::to_string(status));
    };

    pjsua_conf_port_info master{};
    if (const auto status = pjsua_conf_get_port_info(0, &master); status != PJ_SUCCESS) {
        complain("pjsua_conf_get_port_info", status);
        return;
    }

    pj_pool_t *pool = pjsua_pool_create("playback", 512, 512);
    if (pool == nullptr) {
        complain("pjsua_pool_create", PJ_ENOMEM);
        return;
    }

    auto *port = PJ_POOL_ZALLOC_T(pool, pjmedia_port);
    pj_str_t name = pj_str(const_cast<char *>("playback"));
    if (const auto status = pjmedia_port_info_init(
            &port->info, &name, PJMEDIA_SIG_CLASS_PORT_AUD('p', 'l'), master.clock_rate,
            master.channel_count, master.bits_per_sample, master.samples_per_frame);
        status != PJ_SUCCESS) {
        complain("pjmedia_port_info_init", status);
        pj_pool_release(pool);
        return;
    }

    auto *data = PJ_POOL_ZALLOC_T(pool, Impl::PlaybackData);
    data->source = source;
    data->pool = pool;

    port->get_frame = &Impl::playbackGetFrame;
    port->put_frame = &Impl::playbackPutFrame;
    port->on_destroy = &Impl::playbackOnDestroy;
    port->port_data.pdata = data;

    if (const auto status = pjmedia_port_init_grp_lock(port, pool, nullptr); status != PJ_SUCCESS) {
        complain("pjmedia_port_init_grp_lock", status);
        pj_pool_release(pool);
        return;
    }

    pjsua_conf_port_id slot = PJSUA_INVALID_ID;
    if (const auto status = pjsua_conf_add_port(pool, port, &slot); status != PJ_SUCCESS) {
        complain("pjsua_conf_add_port", status);
        pjmedia_port_dec_ref(port);
        return;
    }

    if (const auto status = pjsua_conf_connect(slot, 0); status != PJ_SUCCESS) {
        complain("pjsua_conf_connect", status);
        pjsua_conf_remove_port(slot);
        pjmedia_port_dec_ref(port);
        return;
    }

    impl_->playback.pool = pool;
    impl_->playback.port = port;
    impl_->playback.slot = slot;
    impl_->playbackSource = source;

    impl_->playbackHoldsTheDevice = true;
    reason.keep();
}

void PjsipBackend::announceTo(const std::string &noticeId, const std::vector<std::string> &callIds,
                              const std::vector<std::string> &files, bool intoTheRecording,
                              std::function<void(core::NoticeProgress)> report)
{
    stopAnnouncing(noticeId);

    Impl::Notice &notice = impl_->notices[noticeId];
    notice.onProgress = std::move(report);
    notice.intoTheRecording = intoTheRecording;

    notice.calls = callIds;
    notice.files = files;

    switch (impl_->playNotice(noticeId, callIds, files)) {
    case Impl::Played::Yes:
        core::Logger::instance().log(core::LogLevel::Info, "sip",
                                     "playing the recording notice to " +
                                         std::to_string(callIds.size()) + " leg(s)" +
                                         (intoTheRecording ? ", and into the recording" : ""));
        impl_->noticeBegan(noticeId);
        break;

    case Impl::Played::NotYet:

        core::Logger::instance().log(core::LogLevel::Info, "sip",
                                     "the recording notice has nowhere to play yet: waiting for "
                                     "the conversation to come back");
        impl_->waitForAPort(noticeId);
        break;

    case Impl::Played::No: {

        auto answer = std::exchange(impl_->notices[noticeId].onProgress, nullptr);
        if (answer) {
            answer(core::NoticeProgress::Failed);
        }
        break;
    }
    }
}

void PjsipBackend::beepEvery(const std::string &noticeId, const std::vector<std::string> &callIds,
                             const std::string &file, std::chrono::seconds interval,
                             bool intoTheRecording)
{

    if (Impl::Notice *beeping = impl_->noticeFor(noticeId);
        beeping != nullptr && beeping->repeating && beeping->file == file &&
        beeping->interval == interval && beeping->intoTheRecording == intoTheRecording) {
        beeping->calls = callIds;
        return;
    }

    stopAnnouncing(noticeId);

    Impl::Notice &notice = impl_->notices[noticeId];
    notice.calls = callIds;
    notice.file = file;
    notice.interval = interval;
    notice.repeating = true;
    notice.intoTheRecording = intoTheRecording;

    const bool playing = impl_->playNotice(noticeId, callIds, {file}) == Impl::Played::Yes;

    core::Logger::instance().log(playing ? core::LogLevel::Info : core::LogLevel::Warning, "sip",
                                 playing ? "beeping " + std::to_string(callIds.size()) +
                                               " leg(s) every " + std::to_string(interval.count()) +
                                               "s" +
                                               (intoTheRecording ? ", and into the recording" : "")
                                         : "the recording tone would not play: " +
                                               (file.empty() ? std::string{"no file"} : file));

    notice.timer =
        impl_->loop.scheduleAfter(std::chrono::duration_cast<std::chrono::milliseconds>(interval),
                                  [this, noticeId]() { rearmBeep(noticeId); });
}

void PjsipBackend::rearmBeep(const std::string &noticeId)
{
    Impl::Notice *notice = impl_->noticeFor(noticeId);
    if (notice == nullptr || !notice->repeating) {
        return;
    }

    static_cast<void>(impl_->playNotice(noticeId, notice->calls, {notice->file}));

    notice = impl_->noticeFor(noticeId);
    if (notice == nullptr || !notice->repeating) {
        return;
    }

    notice->timer = impl_->loop.scheduleAfter(
        std::chrono::duration_cast<std::chrono::milliseconds>(notice->interval),
        [this, noticeId]() { rearmBeep(noticeId); });
}

void PjsipBackend::stopAnnouncing(const std::string &noticeId)
{
    Impl::Notice *notice = impl_->noticeFor(noticeId);
    if (notice == nullptr) {
        return;
    }

    notice->repeating = false;
    notice->waiting = false;
    if (notice->timer != core::IScheduler::TimerId{}) {
        impl_->loop.cancel(notice->timer);
        notice->timer = core::IScheduler::TimerId{};
    }
    notice->onProgress = nullptr;
    impl_->closeNotice(noticeId);

    impl_->notices.erase(noticeId);
}

core::RecordingFeed &PjsipBackend::openRecording(const std::string &recordingId)
{
    auto &session = impl_->recordings[recordingId];
    if (session.rings == nullptr) {
        session.rings = impl_->takeFeed();
    }
    return *session.rings;
}

core::RecordingFeed *PjsipBackend::recordingRings(const std::string &recordingId) const
{
    const auto found = impl_->recordings.find(recordingId);
    return found == impl_->recordings.end() ? nullptr : found->second.rings;
}

void PjsipBackend::recordCalls(const std::string &recordingId,
                               const std::vector<std::string> &callIds, bool separately)
{
    const auto found = impl_->recordings.find(recordingId);
    if (found == impl_->recordings.end()) {

        return;
    }

    Impl::RecordingSession &session = found->second;
    if (session.callIds == callIds && session.separately == separately) {

        impl_->refreshRecordingTaps(session);
        return;
    }

    session.callIds = callIds;
    session.separately = separately;
    impl_->refreshRecordingTaps(session);
}

void PjsipBackend::closeRecording(const std::string &recordingId)
{
    const auto found = impl_->recordings.find(recordingId);
    if (found == impl_->recordings.end()) {
        return;
    }

    Impl::RecordingSession &session = found->second;

    impl_->ensureThreadRegistered();
    impl_->closeTap(session.nearTap);
    impl_->closeTap(session.farTap);
    for (auto &stem : session.stemTaps) {
        impl_->closeTap(stem.second);
    }

    if (session.rings != nullptr) {
        impl_->idleFeeds.push_back(session.rings);
    }

    impl_->recordings.erase(found);
}

void PjsipBackend::watchSignals(const std::string &callId)
{
    if (impl_->watchedCall == callId) {

        impl_->refreshSignalTaps();
        return;
    }

    impl_->watchedCall = callId;

    if (impl_->feed) {
        impl_->feed->clear();
    }
    impl_->refreshSignalTaps();
}

core::CallSignals &PjsipBackend::feed() noexcept
{
    return impl_->ensureSignals();
}

std::optional<core::CallMediaStats> PjsipBackend::callMediaStats(const std::string &callId) const
{

    const std::lock_guard held{impl_->mediaStatsLock};
    const auto it = impl_->mediaStats.find(callId);
    if (it == impl_->mediaStats.end()) {
        return std::nullopt;
    }
    return it->second;
}

void PjsipBackend::setCallMuted(const std::string &callId, bool muted)
{
    const auto it = impl_->calls.find(callId);
    if (it == impl_->calls.end()) {
        return;
    }

    impl_->ensureThreadRegistered();
    it->second->setMuted(muted);

    impl_->refreshSignalTaps();

    impl_->refreshRecordingTaps();
}

void PjsipBackend::holdCall(const std::string &callId)
{
    const auto it = impl_->calls.find(callId);
    if (it == impl_->calls.end()) {
        return;
    }

    impl_->ensureThreadRegistered();
    it->second->requestHold(true);

    impl_->forgetRouted(callId);
}

void PjsipBackend::resumeCall(const std::string &callId)
{
    const auto it = impl_->calls.find(callId);
    if (it == impl_->calls.end()) {
        return;
    }

    impl_->ensureThreadRegistered();
    it->second->requestHold(false);

}

void PjsipBackend::requestRegistration(const core::AccountConfig &config)
{
    if (!impl_->running) {
        impl_->publish(config.id,
                       core::RegistrationOutcome{false, 0, core::TransportFailure::ConnectionFailed,
                                                 "SIP stack not started"});
        return;
    }

    impl_->ensureThreadRegistered();
    impl_->markInFlight(config.id);

    try {
        pj::AccountConfig accountConfig;
        accountConfig.idUri = config.sipUri();
        accountConfig.regConfig.registrarUri = registrarUriFor(config);
        accountConfig.regConfig.timeoutSec = static_cast<unsigned>(config.expiry.count());
        accountConfig.regConfig.registerOnAdd = true;

        accountConfig.regConfig.retryIntervalSec = 0;
        accountConfig.regConfig.firstRetryIntervalSec = 0;

        const auto name = core::credentialName(config);
        auto password =
            name.empty() ? std::optional<std::string>{} : impl_->credentials.password(name);
        if (!password) {
            password = impl_->credentials.password(config.id);
        }

        if (password) {
            accountConfig.sipConfig.authCreds.emplace_back(
                "digest", "*", config.effectiveAuthUser(), PJSIP_CRED_DATA_PLAIN_PASSWD, *password);
        }

        if (const auto proxy = config.effectiveOutboundProxy(); !proxy.empty()) {
            accountConfig.sipConfig.proxies.push_back(proxy);
        }

        if (config.transport == core::SipTransport::Tcp &&
            impl_->tcpTransport != PJSUA_INVALID_ID) {
            accountConfig.sipConfig.transportId = impl_->tcpTransport;
        }

        if (config.transport == core::SipTransport::Tls) {
            const auto listener =
                config.verifyCertificate ? impl_->tlsVerifying : impl_->tlsTrusting;
            if (listener != PJSUA_INVALID_ID) {
                accountConfig.sipConfig.transportId = listener;
            }
        }

        switch (config.effectiveMediaEncryption()) {
        case core::MediaEncryption::Off:
            accountConfig.mediaConfig.srtpUse = PJMEDIA_SRTP_DISABLED;
            break;
        case core::MediaEncryption::Offered:
            accountConfig.mediaConfig.srtpUse = PJMEDIA_SRTP_OPTIONAL;
            break;
        case core::MediaEncryption::Required:
            accountConfig.mediaConfig.srtpUse = PJMEDIA_SRTP_MANDATORY;
            break;
        }
        accountConfig.mediaConfig.srtpOpt.keyings = {PJMEDIA_SRTP_KEYING_SDES};

        auto &slot = impl_->accounts[config.id];

        if (slot && !slot->isValid()) {
            const core::LogAttribute fields[] = {{"line", config.id}};
            core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                         "the stack no longer holds this line; making it again",
                                         fields);
            slot.reset();
        }

        if (!slot) {
            slot = std::make_unique<Account>(*impl_, config.id);
            slot->setUsername(config.username);
            slot->setSwitchHost(core::switchHost(config));
            try {
                slot->create(accountConfig);
            } catch (const pj::Error &) {
                impl_->accounts.erase(config.id);
                throw;
            }
        } else {
            slot->setUsername(config.username);
            slot->setSwitchHost(core::switchHost(config));
            slot->modify(accountConfig);

            slot->askToRegister();
        }
    } catch (const pj::Error &error) {

        if (impl_->claimOutcome(config.id)) {
            const auto reason = error.info();
            impl_->publish(config.id, core::RegistrationOutcome{
                                          false, 0, transportFailureFor(0, reason), reason});
        }
    }
}

void PjsipBackend::reconnectAudio()
{
    if (!impl_->running || impl_->routedCallIds.empty()) {
        return;
    }

    impl_->ensureThreadRegistered();
    impl_->connectGroup();

    core::Logger::instance().log(core::LogLevel::Info, "sip",
                                 "wired " + std::to_string(impl_->routedCallIds.size()) +
                                     " call(s) to the sound device again");
}

void PjsipBackend::routeAudioTo(const std::vector<std::string> &callIds)
{
    if (impl_->routedCallIds == callIds) {
        return;
    }

    impl_->ensureThreadRegistered();

    for (const auto &previousId : impl_->routedCallIds) {
        if (std::find(callIds.begin(), callIds.end(), previousId) != callIds.end()) {
            continue;
        }
        const auto previous = impl_->calls.find(previousId);
        if (previous == impl_->calls.end()) {
            continue;
        }

        previous->second->disconnectAudio();

        for (const auto &remainingId : impl_->routedCallIds) {
            if (remainingId == previousId) {
                continue;
            }
            if (const auto remaining = impl_->calls.find(remainingId);
                remaining != impl_->calls.end()) {
                previous->second->disconnectFrom(*remaining->second);
            }
        }
    }

    impl_->routedCallIds = callIds;

    impl_->connectGroup();
}

void PjsipBackend::setLineWatchObserver(core::ILineWatchBackend::Observer *observer)
{
    impl_->lineWatchObserver = observer;
}

void PjsipBackend::watchLine(const std::string &watchId, const std::string &accountId,
                             const std::string &number)
{
    stopWatching(watchId);

    if (!impl_->running) {
        return;
    }

    const auto account = impl_->accounts.find(accountId);
    if (account == impl_->accounts.end() || !account->second) {
        core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                     "cannot watch " + number + ": no account " + accountId);
        impl_->publishWatchFailed(watchId, 0);
        return;
    }

    impl_->ensureThreadRegistered();

    try {
        auto watch = std::make_unique<WatchedLine>(*impl_, watchId);

        pj::BuddyConfig config;
        config.uri = number;

        config.subscribe = false;

        watch->create(*account->second, config);
        watch->subscribeDlgEvent(true);

        core::Logger::instance().log(core::LogLevel::Info, "sip",
                                     "watching " + number + " on account " + accountId);
        impl_->watches[watchId] = std::move(watch);
    } catch (const pj::Error &error) {
        core::Logger::instance().log(core::LogLevel::Warning, "sip",
                                     "could not watch " + number + ": " + error.info());
        impl_->publishWatchFailed(watchId, 0);
    }
}

void PjsipBackend::stopWatching(const std::string &watchId)
{
    const auto found = impl_->watches.find(watchId);
    if (found == impl_->watches.end()) {
        return;
    }

    found->second->retire();

    if (!impl_->running) {
        impl_->watches.erase(found);
        return;
    }

    impl_->ensureThreadRegistered();

    try {

        found->second->subscribeDlgEvent(false);
    } catch (const pj::Error &error) {
        core::Logger::instance().log(core::LogLevel::Debug, "sip",
                                     "unsubscribing " + watchId + ": " + error.info());
    }

    impl_->retiredWatches.push_back(std::move(found->second));
    impl_->watches.erase(found);

    if (impl_->retiredWatches.size() == Impl::kRetiredWarningLevel) {
        core::Logger::instance().log(
            core::LogLevel::Warning, "sip",
            "this session has retired " + std::to_string(impl_->retiredWatches.size()) +
                " watched lines; pjsua has " + std::to_string(PJSUA_MAX_BUDDIES) +
                " slots and they are freed when the phone restarts");
    }
}

void PjsipBackend::sendDtmf(const std::string &callId, char digit, core::DtmfTransport transport)
{
    const auto it = impl_->calls.find(callId);
    if (it == impl_->calls.end()) {
        return;
    }

    impl_->ensureThreadRegistered();

    try {
        pj::CallSendDtmfParam param;
        param.method = transport == core::DtmfTransport::SipInfo ? PJSUA_DTMF_METHOD_SIP_INFO
                                                                 : PJSUA_DTMF_METHOD_RFC2833;
        param.digits = std::string{digit};
        it->second->sendDtmf(param);
    } catch (const pj::Error &error) {

        noteCall(core::LogLevel::Warning, callId,
                 "could not send a tone via " + std::string{core::toString(transport)} + ": " +
                     error.info());
    }
}

void PjsipBackend::transferCall(const std::string &callId, const std::string &target)
{
    const auto it = impl_->calls.find(callId);
    if (it == impl_->calls.end()) {
        return;
    }

    impl_->ensureThreadRegistered();

    it->second->transferTo(target);
}

void PjsipBackend::transferCallToCall(const std::string &callId,
                                      const std::string &consultationCallId)
{
    const auto call = impl_->calls.find(callId);
    const auto consultation = impl_->calls.find(consultationCallId);
    if (call == impl_->calls.end() || consultation == impl_->calls.end()) {
        return;
    }

    impl_->ensureThreadRegistered();

    noteCall(core::LogLevel::Info, callId, "transferring it onto " + consultationCallId);

    try {
        pj::CallOpParam options;
        call->second->xferReplaces(*consultation->second, options);
    } catch (const pj::Error &error) {

        noteCall(core::LogLevel::Warning, callId,
                 "could not transfer it onto " + consultationCallId + ": " + error.info());
        impl_->publishTransferProgress(
            core::TransferProgress{callId, PJSIP_SC_INTERNAL_SERVER_ERROR, true, error.info()});
    }
}

void PjsipBackend::forgetAccount(const std::string &accountId)
{
    const auto it = impl_->accounts.find(accountId);
    if (it == impl_->accounts.end()) {
        return;
    }

    impl_->ensureThreadRegistered();

    for (auto call = impl_->calls.begin(); call != impl_->calls.end();) {
        if (call->second->accountId() == accountId) {
            call = impl_->calls.erase(call);
        } else {
            ++call;
        }
    }

    impl_->refreshSignalTaps();
    impl_->refreshRecordingTaps();

    impl_->accounts.erase(it);
}

void PjsipBackend::requestUnregistration(const std::string &accountId)
{
    const auto it = impl_->accounts.find(accountId);
    if (it == impl_->accounts.end() || !impl_->running) {
        impl_->publish(accountId, core::RegistrationOutcome{false, 0, core::TransportFailure::None,
                                                            "not registered"});
        return;
    }

    impl_->ensureThreadRegistered();

    try {
        it->second->setRegistration(false);
    } catch (const pj::Error &error) {
        impl_->publish(accountId, core::RegistrationOutcome{false, 0, core::TransportFailure::None,
                                                            error.info()});
    }
}

}
