// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/capture/echo.hpp>

#include <pjlib.h>
#include <pjmedia/echo.h>

#include <cstring>
#include <mutex>

namespace softphone::capture {
namespace {

constexpr unsigned kTailMs = 200;

class Pjlib {
public:
    static Pjlib &one()
    {
        static Pjlib only;
        return only;
    }

    [[nodiscard]] pj_pool_factory *pools()
    {
        return started_ ? &caching_.factory : nullptr;
    }

private:
    Pjlib()
    {
        if (pj_init() != PJ_SUCCESS) {
            return;
        }
        pj_caching_pool_init(&caching_, nullptr, 0);
        started_ = true;
    }

    ~Pjlib() = default;

    pj_caching_pool caching_{};
    bool started_{false};
};

void beKnown()
{
    if (pj_thread_is_registered()) {
        return;
    }
    static thread_local pj_thread_desc desc;
    static thread_local pj_thread_t *thread = nullptr;
    std::memset(&desc, 0, sizeof(desc));
    pj_thread_register("softphone-capture", desc, &thread);
}

class PjmediaCanceller final : public IEchoCanceller {
public:
    PjmediaCanceller(unsigned sampleRate, unsigned samplesPerFrame) : frame_{samplesPerFrame}
    {
        auto *const pools = Pjlib::one().pools();
        if (pools == nullptr) {
            return;
        }

        beKnown();
        pool_ = pj_pool_create(pools, "capture-aec", 4096, 4096, nullptr);
        if (pool_ == nullptr) {
            return;
        }

        if (pjmedia_echo_create(pool_, sampleRate, samplesPerFrame, kTailMs, 0,
                                PJMEDIA_ECHO_DEFAULT, &echo_) != PJ_SUCCESS) {
            echo_ = nullptr;
        }
    }

    ~PjmediaCanceller() override
    {
        beKnown();
        if (echo_ != nullptr) {
            pjmedia_echo_destroy(echo_);
        }
        if (pool_ != nullptr) {
            pj_pool_release(pool_);
        }
    }

    PjmediaCanceller(const PjmediaCanceller &) = delete;
    PjmediaCanceller &operator=(const PjmediaCanceller &) = delete;
    PjmediaCanceller(PjmediaCanceller &&) = delete;
    PjmediaCanceller &operator=(PjmediaCanceller &&) = delete;

    void cancel(std::span<std::int16_t> nearSide, std::span<const std::int16_t> farSide) override
    {
        if (echo_ == nullptr || nearSide.size() != frame_ || farSide.size() != frame_) {
            return;
        }
        beKnown();
        static_cast<void>(pjmedia_echo_cancel(echo_, nearSide.data(), farSide.data(), 0, nullptr));
    }

    [[nodiscard]] unsigned samplesPerFrame() const override
    {
        return frame_;
    }

    [[nodiscard]] const char *name() const override
    {
        return echo_ != nullptr ? "pjmedia" : "none";
    }

private:
    unsigned frame_;
    pj_pool_t *pool_{nullptr};
    pjmedia_echo_state *echo_{nullptr};
};

}

std::unique_ptr<IEchoCanceller> makeEchoCanceller(unsigned sampleRate, unsigned samplesPerFrame)
{
    return std::make_unique<PjmediaCanceller>(sampleRate, samplesPerFrame);
}

}
