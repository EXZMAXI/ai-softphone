# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

if(NOT SOFTPHONE_PACKAGE)
    return()
endif()

set(CPACK_PACKAGE_NAME "ai-softphone")
set(CPACK_PACKAGE_VENDOR "AI Softphone")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/bpmkit/ai-softphone")

set(CPACK_PACKAGE_CONTACT "AI Softphone <ip@incoplax.com>")

set(CPACK_STRIP_FILES ON)

set(CPACK_GENERATOR "DEB")

set(CPACK_SET_DESTDIR ON)

set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_SECTION "comm")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS
    "/opt/ai-softphone/lib"
    "/opt/ai-softphone/plugins"
    "/opt/ai-softphone/qml")

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_SOURCE_DIR}/packaging/deb/postinst"
    "${CMAKE_SOURCE_DIR}/packaging/deb/postrm")

include(CPack)
