// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/countries.hpp>

#include "shipped_content.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>
#include <mutex>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>

#include <array>
#include <memory>
#include <type_traits>
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string_view>
#endif

namespace softphone::core {

namespace {

#ifdef __APPLE__

struct CoreFoundationDeleter {
    void operator()(CFTypeRef value) const noexcept
    {
        if (value != nullptr) {
            ::CFRelease(value);
        }
    }
};

template<typename T>
using Owned = std::unique_ptr<std::remove_pointer_t<T>, CoreFoundationDeleter>;

#endif

constexpr std::string_view kFallback = "en";

[[nodiscard]] std::string tableName(std::string_view language)
{
    return "countries-" + std::string{language};
}

[[nodiscard]] std::vector<Country> read(std::string_view language)
{
    const auto bytes = content::shippedBytes(tableName(language));
    if (bytes.empty()) {
        return {};
    }

    const auto document = nlohmann::json::parse(bytes, nullptr, false);
    if (document.is_discarded() || !document.is_object()) {
        return {};
    }
    const auto list = document.find("countries");
    if (list == document.end() || !list->is_array()) {
        return {};
    }

    std::vector<Country> found;
    found.reserve(list->size());
    for (const auto &entry : *list) {
        if (!entry.is_object()) {
            continue;
        }
        auto code = entry.value("code", std::string{});
        auto name = entry.value("name", std::string{});
        if (code.empty() || name.empty()) {
            continue;
        }
        found.push_back({std::move(code), std::move(name)});
    }
    return found;
}

const std::vector<Country> &table(std::string_view language)
{

    static std::mutex guard;
    static std::map<std::string, std::vector<Country>, std::less<>> tables;

    const std::lock_guard held{guard};
    if (const auto found = tables.find(language); found != tables.end()) {
        return found->second;
    }
    return tables.emplace(std::string{language}, read(language)).first->second;
}

}

const std::vector<Country> &countries(std::string_view language)
{
    if (const auto &found = table(language); !found.empty()) {
        return found;
    }
    return table(kFallback);
}

bool isCountry(std::string_view code)
{
    const auto &list = countries(kFallback);
    return std::any_of(list.begin(), list.end(),
                       [code](const Country &one) { return one.code == code; });
}

std::string countryNameIn(std::string_view language, std::string_view code)
{
    const auto &list = countries(language);
    const auto found = std::find_if(list.begin(), list.end(),
                                    [code](const Country &one) { return one.code == code; });
    return found == list.end() ? std::string{} : found->name;
}

std::string countryFromLocale(std::string_view locale)
{

    bool first = true;
    while (!locale.empty()) {
        const auto end = locale.find_first_of("_-.@");
        const auto token = locale.substr(0, end);
        locale = end == std::string_view::npos ? std::string_view{} : locale.substr(end + 1);

        if (first) {
            first = false;
            continue;
        }
        if (token.size() != 2) {
            continue;
        }

        std::string code;
        code.reserve(2);
        for (const char letter : token) {
            const auto one = static_cast<unsigned char>(letter);
            if (std::isalpha(one) == 0) {
                code.clear();
                break;
            }
            code.push_back(static_cast<char>(std::toupper(one)));
        }
        if (!code.empty() && isCountry(code)) {
            return code;
        }
    }
    return {};
}

#ifdef _WIN32

std::string machineCountry()
{

    constexpr int kGeoNameMax = 45;

    wchar_t name[kGeoNameMax]{};
    if (::GetUserDefaultGeoName(name, kGeoNameMax) == 0) {
        return {};
    }

    std::string code;
    for (const wchar_t letter : std::wstring_view{name}) {
        if (letter == L'\0') {
            break;
        }
        code.push_back(static_cast<char>(letter));
    }
    return isCountry(code) ? code : std::string{};
}

#elif defined(__APPLE__)

std::string machineCountry()
{

    const Owned<CFLocaleRef> locale{::CFLocaleCopyCurrent()};
    if (!locale) {
        return {};
    }

    const auto *country =
        static_cast<CFStringRef>(::CFLocaleGetValue(locale.get(), kCFLocaleCountryCode));
    if (country == nullptr) {

        return {};
    }

    std::array<char, 8> buffer{};
    if (::CFStringGetCString(country, buffer.data(), static_cast<CFIndex>(buffer.size()),
                             kCFStringEncodingASCII) == false) {
        return {};
    }

    const std::string code{buffer.data()};
    return isCountry(code) ? code : std::string{};
}

#else

std::string machineCountry()
{

    for (const char *name : {"LC_ALL", "LC_ADDRESS", "LC_TELEPHONE", "LC_MEASUREMENT",
                             "LC_MONETARY", "LC_PAPER", "LC_TIME", "LC_NUMERIC", "LANG"}) {
        const char *told = std::getenv(name);
        if (told == nullptr || *told == '\0') {
            continue;
        }
        if (auto found = countryFromLocale(told); !found.empty()) {
            return found;
        }
    }
    return {};
}

#endif

}
