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

#include <pjmedia/audiodev.h>

extern "C" pjmedia_aud_dev_factory *softphone_pulse_aud_factory(pj_pool_factory *pools);

extern "C" int softphone_pulse_aud_available();

extern "C" int softphone_pulse_aud_default_names(char *sink, unsigned sinkSize, char *source,
                                                 unsigned sourceSize);
