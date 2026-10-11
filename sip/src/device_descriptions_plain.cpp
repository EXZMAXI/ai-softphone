// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "device_descriptions.hpp"

#include <string>
#include <utility>
#include <vector>

namespace softphone::sip {

std::string asText(std::string reported)
{
    return reported;
}

std::vector<core::AudioDeviceInfo> described(std::vector<core::AudioDeviceInfo> devices)
{

    return devices;
}

}
