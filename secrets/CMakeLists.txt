# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

add_library(softphone_secrets ${SOFTPHONE_LIBRARY_KIND}
    src/credential_store.cpp)

add_library(softphone::secrets ALIAS softphone_secrets)

if(WIN32)
    target_sources(softphone_secrets PRIVATE src/credential_manager_store.cpp)

    target_link_libraries(softphone_secrets PRIVATE advapi32)

    message(STATUS "Credential Manager: passwords are kept by Windows")
elseif(APPLE)
    target_sources(softphone_secrets PRIVATE src/keychain_store.cpp)

    target_link_libraries(softphone_secrets PRIVATE
        "-framework Security" "-framework CoreFoundation")

    message(STATUS "Keychain: passwords are kept by macOS")
elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSECRET REQUIRED IMPORTED_TARGET libsecret-1)

    set_target_properties(PkgConfig::LIBSECRET PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${LIBSECRET_INCLUDE_DIRS}")

    target_sources(softphone_secrets PRIVATE src/secret_service_store.cpp)
    target_link_libraries(softphone_secrets PRIVATE PkgConfig::LIBSECRET)

    message(STATUS "libsecret ${LIBSECRET_VERSION}")
else()

    message(FATAL_ERROR
        "No keychain implementation for this platform. Configure with "
        "-DSOFTPHONE_BUILD_SECRETS=OFF to build what does not need one.")
endif()

find_package(OpenSSL)
if(OpenSSL_FOUND)
    target_sources(softphone_secrets PRIVATE src/sealed_file_store.cpp)
    target_link_libraries(softphone_secrets PRIVATE OpenSSL::Crypto)
    message(STATUS "Sealed credentials: available where no keyring is")
else()
    target_sources(softphone_secrets PRIVATE src/sealed_file_absent.cpp)
    message(STATUS "OpenSSL not found: with no keyring, no password can be kept")
endif()

target_include_directories(softphone_secrets
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(softphone_secrets
    PUBLIC
        softphone::core
        softphone::coverage
    PRIVATE
        softphone::warnings)
