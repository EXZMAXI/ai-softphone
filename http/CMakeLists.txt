# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

include(FetchContent)

FetchContent_Declare(cpp_httplib
    URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.18.3.tar.gz
    URL_HASH SHA256=a0567bcd6c3fe5cef1b329b96245119047f876b49e06cc129a36a7a8dffe173e
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

set(HTTPLIB_INSTALL OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(cpp_httplib)

add_library(cpp_httplib_headers INTERFACE)
target_include_directories(cpp_httplib_headers SYSTEM INTERFACE ${cpp_httplib_SOURCE_DIR})

add_library(softphone_http ${SOFTPHONE_LIBRARY_KIND}
    src/anthropic_chat.cpp
    src/deepgram_asr.cpp
    src/elevenlabs_asr.cpp
    src/speechmatics_asr.cpp
    src/assemblyai_asr.cpp
    src/streaming_yandex.cpp
    src/soniox_asr.cpp
    src/xai_asr.cpp
    src/yandex_asr.cpp
    src/httplib_http_client.cpp
    src/httplib_server.cpp
    src/httplib_transport.cpp
    src/httplib_update_transport.cpp
    src/language_codes.cpp
    src/machine_trust.cpp
    src/openai_asr.cpp
    src/openai_chat.cpp)

add_library(softphone::http ALIAS softphone_http)

target_include_directories(softphone_http
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(softphone_http PRIVATE cpp_httplib_headers)

target_link_libraries(softphone_http
    PUBLIC softphone::core
    PRIVATE softphone::warnings softphone::coverage nlohmann_json::nlohmann_json)

if(APPLE)
    target_link_libraries(softphone_http PRIVATE
        "-framework Security" "-framework CoreFoundation")
endif()

if(WIN32)
    target_link_libraries(softphone_http PRIVATE crypt32)
endif()

find_package(OpenSSL REQUIRED)
target_compile_definitions(cpp_httplib_headers INTERFACE CPPHTTPLIB_OPENSSL_SUPPORT)
target_link_libraries(cpp_httplib_headers INTERFACE OpenSSL::SSL OpenSSL::Crypto)

target_sources(softphone_http PRIVATE src/openssl_signatures.cpp)
target_link_libraries(softphone_http PRIVATE OpenSSL::Crypto)

find_package(Threads REQUIRED)
target_link_libraries(softphone_http PRIVATE Threads::Threads)

FetchContent_Declare(ixwebsocket
    URL https://github.com/machinezone/IXWebSocket/archive/refs/tags/v11.4.6.tar.gz
    URL_HASH SHA256=c024334f8e45980836c67008979a884d6dcc5ef067dd2eb1fa7241f4c17ddc32
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

set(USE_TLS TRUE CACHE INTERNAL "")
set(USE_OPEN_SSL TRUE CACHE INTERNAL "")

set(USE_ZLIB FALSE CACHE INTERNAL "")

set(IXWEBSOCKET_INSTALL OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(ixwebsocket)

set_target_properties(ixwebsocket PROPERTIES POSITION_INDEPENDENT_CODE ON)

set_target_properties(ixwebsocket PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)

if(MSVC)
    target_compile_options(ixwebsocket PRIVATE /W0)
else()
    target_compile_options(ixwebsocket PRIVATE -w)
endif()

get_target_property(SOFTPHONE_IXWEBSOCKET_INCLUDES ixwebsocket INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(ixwebsocket PROPERTIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${SOFTPHONE_IXWEBSOCKET_INCLUDES}")

FetchContent_Declare(nghttp2
    URL https://github.com/nghttp2/nghttp2/releases/download/v1.68.0/nghttp2-1.68.0.tar.gz
    URL_HASH SHA256=2c16ffc588ad3f9e2613c3fad72db48ecb5ce15bc362fcc85b342e48daf51013
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

set(ENABLE_LIB_ONLY ON CACHE INTERNAL "")
set(ENABLE_DOC OFF CACHE INTERNAL "")
set(ENABLE_FAILMALLOC OFF CACHE INTERNAL "")
set(WITH_LIBXML2 OFF CACHE INTERNAL "")
set(WITH_JEMALLOC OFF CACHE INTERNAL "")

set(BUILD_STATIC_LIBS ON CACHE INTERNAL "")

FetchContent_MakeAvailable(nghttp2)

set_property(DIRECTORY "${nghttp2_SOURCE_DIR}" PROPERTY EXCLUDE_FROM_ALL TRUE)

set(SOFTPHONE_NGHTTP2 nghttp2_static)
set_target_properties(${SOFTPHONE_NGHTTP2} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_VISIBILITY_PRESET hidden)
if(MSVC)
    target_compile_options(${SOFTPHONE_NGHTTP2} PRIVATE /w)
else()
    target_compile_options(${SOFTPHONE_NGHTTP2} PRIVATE -w)
endif()
get_target_property(SOFTPHONE_NGHTTP2_INCLUDES ${SOFTPHONE_NGHTTP2}
    INTERFACE_INCLUDE_DIRECTORIES)
if(SOFTPHONE_NGHTTP2_INCLUDES)
    set_target_properties(${SOFTPHONE_NGHTTP2} PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${SOFTPHONE_NGHTTP2_INCLUDES}")
endif()

target_sources(softphone_http PRIVATE src/nghttp2_grpc.cpp)
target_link_libraries(softphone_http PRIVATE ${SOFTPHONE_NGHTTP2})

target_compile_definitions(softphone_http PRIVATE NGHTTP2_NO_SSIZE_T)

target_sources(softphone_http PRIVATE
    src/ix_web_socket.cpp

    src/streaming_session.cpp
    src/streaming_recognisers.cpp
    src/streaming_deepgram.cpp
    src/streaming_xai_elevenlabs.cpp
    src/streaming_openai_speechmatics.cpp
    src/streaming_soniox.cpp
    src/streaming_realtime_events.cpp
    src/streaming_local.cpp
    src/streaming_assemblyai.cpp)
target_link_libraries(softphone_http PRIVATE ixwebsocket)
