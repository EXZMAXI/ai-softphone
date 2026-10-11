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

#include <softphone/container/manifest.hpp>
#include <softphone/container/plugin.hpp>

#include <string>
#include <string_view>

#define SOFTPHONE_KEY_TEXT_(x) #x
#define SOFTPHONE_KEY_TEXT(x) SOFTPHONE_KEY_TEXT_(x)

#if defined(_MSC_VER) && !defined(__clang__)
#define SOFTPHONE_KEY_COMPILER "msvc-" SOFTPHONE_KEY_TEXT(_MSC_VER)
#elif defined(__clang__)
#define SOFTPHONE_KEY_COMPILER                                                                     \
    "clang-" SOFTPHONE_KEY_TEXT(__clang_major__) "." SOFTPHONE_KEY_TEXT(__clang_minor__)
#elif defined(__GNUC__)
#define SOFTPHONE_KEY_COMPILER                                                                     \
    "gnu-" SOFTPHONE_KEY_TEXT(__GNUC__) "." SOFTPHONE_KEY_TEXT(__GNUC_MINOR__)
#else
#define SOFTPHONE_KEY_COMPILER "unknown"
#endif

#if defined(_MSVC_STL_VERSION)
#define SOFTPHONE_KEY_STDLIB                                                                       \
    "msvcstl-" SOFTPHONE_KEY_TEXT(_MSVC_STL_VERSION) "-" SOFTPHONE_KEY_TEXT(_ITERATOR_DEBUG_LEVEL)
#elif defined(_LIBCPP_VERSION)
#define SOFTPHONE_KEY_STDLIB "libc++-" SOFTPHONE_KEY_TEXT(_LIBCPP_VERSION)
#elif defined(__GLIBCXX__)
#define SOFTPHONE_KEY_STDLIB "libstdc++-" SOFTPHONE_KEY_TEXT(__GLIBCXX__)
#else
#define SOFTPHONE_KEY_STDLIB "unknown"
#endif

#if defined(_WIN32)
#define SOFTPHONE_KEY_SYSTEM "windows"
#elif defined(__APPLE__)
#define SOFTPHONE_KEY_SYSTEM "macos"
#elif defined(__linux__)
#define SOFTPHONE_KEY_SYSTEM "linux"
#else
#define SOFTPHONE_KEY_SYSTEM "unknown"
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define SOFTPHONE_KEY_MACHINE "x86_64"
#elif defined(__aarch64__) || defined(_M_ARM64)
#define SOFTPHONE_KEY_MACHINE "arm64"
#elif defined(__i386__) || defined(_M_IX86)
#define SOFTPHONE_KEY_MACHINE "x86"
#else
#define SOFTPHONE_KEY_MACHINE "unknown"
#endif

#if !defined(SOFTPHONE_CONTAINER_VERSION)
#error "SOFTPHONE_CONTAINER_VERSION is missing: link softphone::container, which defines it."
#endif

#if defined(_WIN32)
#define SOFTPHONE_PLUGIN_EXPORT __declspec(dllexport)
#else
#define SOFTPHONE_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

namespace softphone::container {

struct BuildKey {

    std::string container;
    std::string compiler;
    std::string standardLibrary;
    std::string system;
    std::string machine;

    std::string qt;

    [[nodiscard]] friend bool operator==(const BuildKey &, const BuildKey &) = default;
};

[[nodiscard]] inline BuildKey thisBuild()
{
    return BuildKey{SOFTPHONE_CONTAINER_VERSION, SOFTPHONE_KEY_COMPILER, SOFTPHONE_KEY_STDLIB,
                    SOFTPHONE_KEY_SYSTEM,        SOFTPHONE_KEY_MACHINE,  {}};
}

[[nodiscard]] std::string toString(const BuildKey &key);

[[nodiscard]] BuildKey parseBuildKey(std::string_view text);

[[nodiscard]] std::string mismatch(const BuildKey &host, const BuildKey &plugin);

#define SOFTPHONE_BUILD_KEY                                                                        \
    "container=" SOFTPHONE_CONTAINER_VERSION ";compiler=" SOFTPHONE_KEY_COMPILER                   \
    ";stdlib=" SOFTPHONE_KEY_STDLIB ";system=" SOFTPHONE_KEY_SYSTEM                                \
    ";machine=" SOFTPHONE_KEY_MACHINE

struct PluginEntry {

    int abi;

    const char *buildKey;

    const char *(*manifest)();

    IPlugin *(*create)();

    void (*destroy)(IPlugin *);
};

inline constexpr int kEntryAbi = 1;

inline constexpr const char *kEntryPoint = "softphone_plugin_entry";

using EntryPoint = const PluginEntry *(*)();

}

#define SOFTPHONE_PLUGIN(TYPE, MANIFEST)                                                           \
    extern "C" SOFTPHONE_PLUGIN_EXPORT const ::softphone::container::PluginEntry *                 \
    softphone_plugin_entry()                                                                       \
    {                                                                                              \
        static const ::softphone::container::PluginEntry entry{                                    \
            ::softphone::container::kEntryAbi, SOFTPHONE_BUILD_KEY,                                \
            []() -> const char * {                                                                 \
                static const ::std::string text = ::softphone::container::toJson(MANIFEST());      \
                return text.c_str();                                                               \
            },                                                                                     \
            []() -> ::softphone::container::IPlugin * { return new TYPE(); },                      \
            [](::softphone::container::IPlugin *plugin) { delete plugin; }};                       \
        return &entry;                                                                             \
    }
