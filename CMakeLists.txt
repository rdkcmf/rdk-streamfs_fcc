#
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright (c) 2022 Nuuday.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.10)
project(fcc_lib)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(glog)

# Configure FCC implementation
if (NOT DEFINED DATA_SOURCE_IMPLEMENTATION)
    set(DATA_SOURCE_IMPLEMENTATION "3rdparty/udp")
endif(NOT DEFINED DATA_SOURCE_IMPLEMENTATION)

message("Using FCC implementation: ${DATA_SOURCE_IMPLEMENTATION}.." )

include (${DATA_SOURCE_IMPLEMENTATION}/plugin.cmake)

option(TS_PACKAGE_DUMP "Enable web player and Nokia library ts-dump over TCP stream socket" ON)

if (TS_PACKAGE_DUMP)
    add_definitions(-DTS_PACKAGE_DUMP)
    add_definitions(-DDEMUX_OUTPUT_TS_DUMP_SOCKET_PORT=9994)
    add_definitions(-DPLAYER_READ_TS_DUMP_SOCKET_PORT=9995)
endif()

option(PERFETTO_TRACING "Enable Perfetto tracer" OFF)
option(BUILD_UNIT_TESTS "Enable Unit Tests" OFF)

MESSAGE("CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

include_directories(${streamlink_INCLUDE_DIRS})

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

set(CMAKE_REQUIRED_LIBRARIES "curl")

find_package(Boost REQUIRED COMPONENTS filesystem regex)

if (PERFETTO_TRACING)
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "^x86_64")
    set(TRACING_LIB perfetto)
  else (CMAKE_SYSTEM_PROCESSOR MATCHES "^x86_64")
    set(TRACING_LIB libperfetto.a)
  endif(CMAKE_SYSTEM_PROCESSOR MATCHES "^x86_64")

  ADD_DEFINITIONS(-DWITH_PERFETTO -DMAX_BUFFERS)
else (PERFETTO_TRACING)
  ADD_DEFINITIONS(-DMAX_BUFFERS)
endif (PERFETTO_TRACING)

add_library(fcc_lib SHARED
        src/FCCPlugin.cpp
        src/MediaSourceHandler.cpp
        src/NetworkRouteMonitor.cpp
        src/externals.cpp
        src/confighandler/ChannelSelector.cpp
        src/tracing.cpp
        src/DemuxerCallbackHandler.cpp
        src/DemuxerStatusCallback.cpp
        src/confighandler/CdmStatusHandler.cpp
        src/confighandler/PlayerStateRequestHandler.cpp
        src/confighandler/SeekRequestHandler.cpp
        src/confighandler/StatsRequestHandler.cpp
        src/confighandler/StreamInfoRequestHandler.cpp
        src/confighandler/ProtectionInfoRequestHandler.cpp
        src/confighandler/TrickPlayRequestHandler.cpp
        src/ConstDelayDefHandler.cpp
        src/HttpDemuxerImpl.cpp
        src/utils/VariableMonitorDispatcher.cpp
        src/StreamParser/EcmCache.cpp
        src/StreamParser/PSIParser.cpp
        src/StreamParser/ProtectionData.hpp
        src/StreamParser/StreamConsumer.cpp
        src/StreamParser/StreamSource.cpp
        src/StreamParser/StreamProcessor.cpp
        src/StreamParser/TimeShiftBufferConsumer.cpp
        src/json.cpp
        src/TimeoutWatchdog.cpp
        src/TimeIntervalMonitor.cpp
        src/StreamProtectionConfigExport.cpp
        src/StreamParser/BufferIndexer.cpp
        src/StreamParser/ScheduledTask.cpp
        src/confighandler/ConfigCache.cpp
        src/fcc/FCCConfigHandlers.cpp
        src/HandleContext.cpp
        src/SocketServer.cpp
        src/TimeStampCorrector.cpp include/TimeStampCorrector.h
        ${PLUGIN_SOURCES}
        include/demux_impl.h
        )

include_directories(${CURL_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include/${CMAKE_SYSTEM_PROCESSOR}/)
include_directories(${PLUGIN_INCLUDES})

target_link_libraries(fcc_lib
        glog
        ${TRACING_LIB}
        ${IMPL_LIBS}
        ${Boost_LIBRARIES}
        buqu
        pthread
        curl
        )

target_compile_options(fcc_lib PRIVATE -Wall -Wextra -pedantic -Werror)

target_include_directories(fcc_lib
        PRIVATE
        ${STREAMFS_EXPORT_PATH}
        include)

if (BUILD_UNIT_TESTS)
    add_subdirectory(test)
endif ()

INSTALL(TARGETS fcc_lib
        LIBRARY DESTINATION lib/streamfs/
        )
include_directories(${C_INCLUDE_DIRS})

find_package(Git)
if (GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    include(GetGitRevisionDescription)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD OUTPUT_VARIABLE SHORT_SHA OUTPUT_STRIP_TRAILING_WHITESPACE
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            )
    set(REVISION ${SHORT_SHA} CACHE STRING "git short sha" FORCE)
    get_git_head_revision(REFSPEC COMMITHASH)
else ()
    message(WARNING "Git not found, cannot set version info")
    SET(REVISION "unknown")
endif ()

include_directories(${CMAKE_BINARY_DIR})
configure_file("${PROJECT_SOURCE_DIR}/include/version.h.in" "${CMAKE_BINARY_DIR}/version.h" @ONLY)
