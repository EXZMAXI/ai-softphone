# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

add_executable(softphone_screenshot_audio main.cpp)

target_link_libraries(softphone_screenshot_audio
    PRIVATE softphone::media softphone::core softphone::warnings)

set_target_properties(softphone_screenshot_audio PROPERTIES
    OUTPUT_NAME screenshot-audio)
