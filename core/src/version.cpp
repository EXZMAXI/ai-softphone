// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/version.hpp>

#include <string>

namespace softphone::core {
namespace {

constexpr Version kVersion{
    SOFTPHONE_VERSION_MAJOR,
    SOFTPHONE_VERSION_MINOR,
    SOFTPHONE_VERSION_PATCH,
};

constexpr std::string_view kVersionString{SOFTPHONE_VERSION_STRING};

constexpr std::string_view kProductName{"ai-softphone"};

constexpr std::string_view kPlatformName{
#if defined(_WIN32)
    "windows"
#elif defined(__APPLE__)
    "macos"
#else
    "linux"
#endif
};

[[nodiscard]] std::string_view configuredPackageFormat() noexcept
{
    constexpr std::string_view configured{SOFTPHONE_PACKAGE_FORMAT};
    return configured.empty() ? std::string_view{"source"} : configured;
}

[[nodiscard]] const std::string &composedUserAgent()
{
    static const std::string text = std::string{kProductName} + '/' + std::string{kVersionString} +
                                    '-' + std::string{kPlatformName} + '-' +
                                    std::string{configuredPackageFormat()};
    return text;
}

}

Version version() noexcept
{
    return kVersion;
}

std::string_view versionString() noexcept
{
    return kVersionString;
}

std::string_view productName() noexcept
{
    return kProductName;
}

std::string_view platformName() noexcept
{
    return kPlatformName;
}

std::string_view packageFormat() noexcept
{
    return configuredPackageFormat();
}

std::string_view userAgent()
{
    return composedUserAgent();
}

}
