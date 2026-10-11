// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/sip/system_default_device.hpp>

#include <pulse/pulseaudio.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace softphone::sip {
namespace {

constexpr pa_usec_t kHandshakeUs = 3 * 1000 * 1000;

constexpr auto kSettling = std::chrono::milliseconds{400};

class Server {
public:
    Server() = default;

    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;
    Server(Server &&) = delete;
    Server &operator=(Server &&) = delete;

    ~Server()
    {
        if (loop_ == nullptr) {
            return;
        }

        if (context_ != nullptr) {
            pa_threaded_mainloop_lock(loop_);
            pa_context_set_subscribe_callback(context_, nullptr, nullptr);
            pa_context_set_state_callback(context_, nullptr, nullptr);
            pa_context_disconnect(context_);
            pa_context_unref(context_);
            context_ = nullptr;
            pa_threaded_mainloop_unlock(loop_);
        }

        pa_threaded_mainloop_stop(loop_);
        pa_threaded_mainloop_free(loop_);
        loop_ = nullptr;
    }

    [[nodiscard]] bool connect()
    {
        loop_ = pa_threaded_mainloop_new();
        if (loop_ == nullptr) {
            return false;
        }
        context_ = pa_context_new(pa_threaded_mainloop_get_api(loop_), "ai-softphone");
        if (context_ == nullptr) {
            return false;
        }

        pa_context_set_state_callback(context_, &awoke, loop_);
        if (pa_context_connect(context_, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0 ||
            pa_threaded_mainloop_start(loop_) < 0) {
            return false;
        }

        pa_threaded_mainloop_lock(loop_);

        const auto limit = pa_rtclock_now() + kHandshakeUs;
        auto *const alarm = pa_context_rttime_new(context_, limit, &rang, loop_);

        bool ready = false;
        for (;;) {
            const auto state = pa_context_get_state(context_);
            if (state == PA_CONTEXT_READY) {
                ready = true;
                break;
            }
            if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED ||
                pa_rtclock_now() >= limit) {
                break;
            }
            pa_threaded_mainloop_wait(loop_);
        }

        if (alarm != nullptr) {
            pa_threaded_mainloop_get_api(loop_)->time_free(alarm);
        }
        pa_threaded_mainloop_unlock(loop_);
        return ready;
    }

    [[nodiscard]] pa_threaded_mainloop *loop() const noexcept
    {
        return loop_;
    }

    [[nodiscard]] pa_context *context() const noexcept
    {
        return context_;
    }

private:
    static void awoke(pa_context *asked, void *self) noexcept
    {
        switch (pa_context_get_state(asked)) {
        case PA_CONTEXT_READY:
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            pa_threaded_mainloop_signal(static_cast<pa_threaded_mainloop *>(self), 0);
            break;
        default:
            break;
        }
    }

    static void rang(pa_mainloop_api * , pa_time_event * ,
                     const struct timeval * , void *self) noexcept
    {
        pa_threaded_mainloop_signal(static_cast<pa_threaded_mainloop *>(self), 0);
    }

    pa_threaded_mainloop *loop_{nullptr};
    pa_context *context_{nullptr};
};

struct Asking {
    pa_threaded_mainloop *loop{nullptr};
    bool done{false};

    std::string address;

    std::string words;
};

void onServer(pa_context * , const pa_server_info *info, void *self) noexcept
{
    auto *const asking = static_cast<Asking *>(self);
    if (info != nullptr) {

        asking->address = info->default_sink_name != nullptr ? info->default_sink_name : "";
    }
    asking->done = true;
    pa_threaded_mainloop_signal(asking->loop, 0);
}

void onServerSource(pa_context * , const pa_server_info *info, void *self) noexcept
{
    auto *const asking = static_cast<Asking *>(self);
    if (info != nullptr) {
        asking->address = info->default_source_name != nullptr ? info->default_source_name : "";
    }
    asking->done = true;
    pa_threaded_mainloop_signal(asking->loop, 0);
}

void onSinkWords(pa_context * , const pa_sink_info *info, int eol, void *self) noexcept
{
    auto *const asking = static_cast<Asking *>(self);
    if (eol != 0) {
        asking->done = true;
        pa_threaded_mainloop_signal(asking->loop, 0);
        return;
    }
    if (info != nullptr && info->description != nullptr) {
        asking->words = info->description;
    }
}

void onSourceWords(pa_context * , const pa_source_info *info, int eol,
                   void *self) noexcept
{
    auto *const asking = static_cast<Asking *>(self);
    if (eol != 0) {
        asking->done = true;
        pa_threaded_mainloop_signal(asking->loop, 0);
        return;
    }
    if (info != nullptr && info->description != nullptr) {
        asking->words = info->description;
    }
}

void waitFor(Asking &asking, pa_operation *running)
{
    if (running == nullptr) {
        return;
    }
    while (!asking.done) {
        pa_threaded_mainloop_wait(asking.loop);
    }
    pa_operation_unref(running);
}

}

std::optional<std::string> systemDefaultDeviceName(core::DeviceRole role)
{
    Server server;
    if (!server.connect()) {
        return std::nullopt;
    }

    Asking asking;
    asking.loop = server.loop();

    pa_threaded_mainloop_lock(server.loop());

    asking.done = false;
    waitFor(asking, role == core::DeviceRole::Capture
                        ? pa_context_get_server_info(server.context(), &onServerSource, &asking)
                        : pa_context_get_server_info(server.context(), &onServer, &asking));

    if (!asking.address.empty()) {
        asking.done = false;
        waitFor(asking, role == core::DeviceRole::Capture
                            ? pa_context_get_source_info_by_name(
                                  server.context(), asking.address.c_str(), &onSourceWords, &asking)
                            : pa_context_get_sink_info_by_name(
                                  server.context(), asking.address.c_str(), &onSinkWords, &asking));
    }

    pa_threaded_mainloop_unlock(server.loop());

    auto name = asking.words.empty() ? asking.address : asking.words;
    if (name.empty()) {
        return std::nullopt;
    }
    return name;
}

namespace {

constexpr auto kWhatMoves = static_cast<pa_subscription_mask_t>(
    PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SOURCE | PA_SUBSCRIPTION_MASK_SERVER);

class SoundServerWatch final : public DeviceWatch {
public:
    SoundServerWatch(std::function<void()> moved, std::unique_ptr<Server> server)
        : moved_{std::move(moved)}, server_{std::move(server)}
    {
        pa_threaded_mainloop_lock(server_->loop());
        pa_context_set_subscribe_callback(server_->context(), &heard, this);
        if (auto *const asking =
                pa_context_subscribe(server_->context(), kWhatMoves, nullptr, nullptr)) {
            pa_operation_unref(asking);
        }
        pa_threaded_mainloop_unlock(server_->loop());

        settling_ = std::thread{[this]() { settle(); }};
    }

    SoundServerWatch(const SoundServerWatch &) = delete;
    SoundServerWatch &operator=(const SoundServerWatch &) = delete;
    SoundServerWatch(SoundServerWatch &&) = delete;
    SoundServerWatch &operator=(SoundServerWatch &&) = delete;

    ~SoundServerWatch() override
    {

        pa_threaded_mainloop_lock(server_->loop());
        pa_context_set_subscribe_callback(server_->context(), nullptr, nullptr);
        pa_threaded_mainloop_unlock(server_->loop());

        watching_.store(false, std::memory_order_release);
        wake_.notify_all();
        if (settling_.joinable()) {
            settling_.join();
        }

        server_.reset();
    }

private:
    static void heard(pa_context * , pa_subscription_event_type_t ,
                      std::uint32_t , void *owner) noexcept
    {

        auto *const self = static_cast<SoundServerWatch *>(owner);
        self->something_.store(true, std::memory_order_release);
        self->wake_.notify_one();
    }

    void settle()
    {
        while (watching_.load(std::memory_order_acquire)) {
            {
                std::unique_lock sleeping{waking_};
                wake_.wait(sleeping, [this] {
                    return something_.load(std::memory_order_acquire) ||
                           !watching_.load(std::memory_order_acquire);
                });
            }
            if (!watching_.load(std::memory_order_acquire)) {
                return;
            }

            std::this_thread::sleep_for(kSettling);
            something_.store(false, std::memory_order_release);
            if (watching_.load(std::memory_order_acquire) && moved_) {
                moved_();
            }
        }
    }

    std::function<void()> moved_;
    std::unique_ptr<Server> server_;
    std::thread settling_;
    std::atomic<bool> watching_{true};
    std::atomic<bool> something_{false};
    std::mutex waking_;
    std::condition_variable wake_;
};

}

std::unique_ptr<DeviceWatch> watchTheMachinesDevices(std::function<void()> moved)
{
    if (!moved) {
        return {};
    }

    auto server = std::make_unique<Server>();
    if (!server->connect()) {
        return {};
    }

    return std::make_unique<SoundServerWatch>(std::move(moved), std::move(server));
}

}
