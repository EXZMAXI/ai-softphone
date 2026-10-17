// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/audio_encoder.hpp>
#include <softphone/media/mp3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <numbers>
#include <random>
#include <span>
#include <string>
#include <vector>

namespace {

constexpr unsigned kRate = 16000;

constexpr std::size_t kChunk = kRate / 10;

constexpr double kRoomNoise = 90.0;

constexpr int kSyllableShortest = 70;
constexpr int kSyllableLongest = 260;
constexpr int kGapShortest = 25;
constexpr int kGapLongest = 110;

constexpr int kBreathShortest = 180;
constexpr int kBreathLongest = 620;
constexpr int kSyllablesBetweenBreaths = 7;

constexpr double kQuietestSyllable = 0.28;
constexpr double kLoudestSyllable = 1.0;

constexpr double kNearLevel = 9600.0;
constexpr double kFarLevel = 7100.0;

enum class Who { Near, Far, Nobody };

struct Turn {
    Who who{Who::Nobody};
    std::uint64_t milliseconds{0};
};

[[nodiscard]] std::vector<Turn> readThePlan(std::istream &input, std::string &complaint)
{
    std::vector<Turn> turns;
    std::string voice;
    std::uint64_t milliseconds = 0;

    while (input >> voice >> milliseconds) {
        if (voice == "near") {
            turns.push_back({Who::Near, milliseconds});
        } else if (voice == "far") {
            turns.push_back({Who::Far, milliseconds});
        } else if (voice == "quiet") {
            turns.push_back({Who::Nobody, milliseconds});
        } else {
            complaint = "no such voice: " + voice;
            return {};
        }
    }

    if (turns.empty()) {
        complaint = "the plan is empty";
    }
    return turns;
}

[[nodiscard]] std::vector<float> envelopeFor(std::uint64_t samples, double level,
                                             std::mt19937 &noise)
{
    std::uniform_int_distribution<int> syllableLength{kSyllableShortest, kSyllableLongest};
    std::uniform_int_distribution<int> gapLength{kGapShortest, kGapLongest};
    std::uniform_int_distribution<int> breathLength{kBreathShortest, kBreathLongest};
    std::uniform_int_distribution<int> untilABreath{3, kSyllablesBetweenBreaths};
    std::uniform_real_distribution<double> loudness{kQuietestSyllable, kLoudestSyllable};

    std::normal_distribution<double> drift{0.0, 0.09};
    double wandering = 1.0;

    std::vector<float> envelope(samples, 0.0F);
    std::uint64_t at = 0;
    int left = untilABreath(noise);

    const auto milliseconds = [](int value) {
        return static_cast<std::uint64_t>(value) * kRate / 1000;
    };

    while (at < samples) {
        const auto length = std::min(milliseconds(syllableLength(noise)), samples - at);
        wandering = std::clamp(wandering + drift(noise), 0.45, 1.15);
        const double peak = level * loudness(noise) * wandering;

        const auto rising = std::max<std::uint64_t>(1, length / 10);
        const auto falling = std::max<std::uint64_t>(1, length / 3);
        for (std::uint64_t one = 0; one < length; ++one) {
            double shape = 1.0;
            if (one < rising) {
                shape = static_cast<double>(one) / static_cast<double>(rising);
            } else if (one + falling > length) {
                shape = static_cast<double>(length - one) / static_cast<double>(falling);
            }
            envelope[at + one] = static_cast<float>(peak * shape);
        }

        at += length;
        if (at >= samples) {
            break;
        }

        at += std::min(milliseconds(gapLength(noise)), samples - at);
        if (--left <= 0) {
            at += std::min(milliseconds(breathLength(noise)), samples - at);
            left = untilABreath(noise);
        }
    }
    return envelope;
}

}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: screenshot-audio <file.mp3> < plan\n\n"
                     "One turn a line: `near <ms>`, `far <ms>` or `quiet <ms>`.\n"
                     "Written in order as two channels, near on the left and far on the right.\n";
        return EXIT_FAILURE;
    }

    std::string complaint;
    const auto turns = readThePlan(std::cin, complaint);
    if (turns.empty()) {
        std::cerr << "screenshot-audio: " << complaint << '\n';
        return EXIT_FAILURE;
    }

    softphone::media::Mp3Encoder encoder;
    const softphone::core::EncoderSettings settings{.sampleRate = kRate, .channels = 2};
    if (encoder.open(std::filesystem::path{argv[1]}, settings) !=
        softphone::core::EncoderStatus::Ok) {
        std::cerr << "screenshot-audio: cannot write " << argv[1] << '\n';
        return EXIT_FAILURE;
    }

    std::mt19937 nearVoice{20260902};
    std::mt19937 farVoice{781104};
    std::mt19937 hiss{55501};
    std::normal_distribution<double> room{0.0, kRoomNoise};

    std::uniform_real_distribution<double> grain{-1.0, 1.0};

    std::vector<std::int16_t> left;
    std::vector<std::int16_t> right;
    std::uint64_t written = 0;

    for (const auto &turn : turns) {
        const auto samples = turn.milliseconds * kRate / 1000;
        const bool speaking = turn.who != Who::Nobody;
        auto &voice = turn.who == Who::Far ? farVoice : nearVoice;
        const double level = turn.who == Who::Far ? kFarLevel : kNearLevel;

        const auto envelope =
            speaking ? envelopeFor(samples, level, voice) : std::vector<float>(samples, 0.0F);

        for (std::uint64_t at = 0; at < samples; at += kChunk) {
            const auto here = std::min<std::uint64_t>(kChunk, samples - at);
            left.assign(here, 0);
            right.assign(here, 0);

            for (std::uint64_t one = 0; one < here; ++one) {
                const double quiet = room(hiss);
                const double loud = static_cast<double>(envelope[at + one]) * grain(hiss);

                const auto talking =
                    static_cast<std::int16_t>(std::clamp(loud + quiet, -32000.0, 32000.0));
                const auto listening = static_cast<std::int16_t>(quiet);

                left[one] = turn.who == Who::Near ? talking : listening;
                right[one] = turn.who == Who::Far ? talking : listening;
            }

            if (encoder.write(left, right) != softphone::core::EncoderStatus::Ok) {
                std::cerr << "screenshot-audio: the encoder refused a block\n";
                return EXIT_FAILURE;
            }
            written += here;
        }
    }

    if (encoder.finish() != softphone::core::EncoderStatus::Ok) {
        std::cerr << "screenshot-audio: the encoder would not close the file\n";
        return EXIT_FAILURE;
    }

    std::cout << written * 1000 / kRate << ' ' << encoder.bytes() << '\n';
    return EXIT_SUCCESS;
}
