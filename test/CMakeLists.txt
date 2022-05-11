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

link_directories(${STREAMFS_EXPORT_PATH}/../build)

add_executable(
        functional_tests
        test_main.cpp
)

add_executable(
        fcc_unit_tests
        unit_tests.cpp
        psi_tests.cpp
        protection_info_tests.cpp
        YouseeChannels.cpp
)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(
        fccShell
        ../src/json.cpp
        StreamFSInteractive.cpp
        YouseeChannels.cpp
)

find_library(MENU_STATIC_LIB  libmenu.a)

 target_link_libraries(
        fccShell
        ${MENU_STATIC_LIB}
        ${CURSES_LIBRARY}
        pthread
)

add_executable(
        http_functional_tests
        http_functional_tests.cpp
)

add_executable(
        tesb_tests
        MemoryAllocatorTest.cpp
)

target_link_libraries(
        tesb_tests
        gtest_main
        gtest
        gmock_main
)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)

set(CMAKE_REQUIRED_LIBRARIES "curl")

find_package(Boost REQUIRED COMPONENTS filesystem)

ADD_DEFINITIONS(-DMAX_BUFFERS)

include(../cmake/googletest.cmake)

fetch_googletest(
        ${PROJECT_SOURCE_DIR}/cmake
        ${PROJECT_BINARY_DIR}/googletest
)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^x86_64")
    target_link_libraries(functional_tests vsa_client)
    target_link_libraries(fcc_unit_tests vsa_client)

endif ()

target_link_libraries(
        functional_tests
        gtest_main
        gtest
        fcc_lib
        buqu
        ${Boost_LIBRARIES}
)

target_link_libraries(
        fcc_unit_tests
        gtest_main
        gtest
        gmock_main
        buqu
        fcc_lib
        ${Boost_LIBRARIES}
)

target_link_libraries(
        http_functional_tests
        gtest_main
        gtest
        gmock_main
        fcc_lib
        buqu
        ${Boost_LIBRARIES}
        curl
)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

include_directories(
        ../include
        ${STREAMFS_EXPORT_PATH}
)

add_test(
        NAME
        unit
        COMMAND
        ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/functional_tests
        ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/unit_tests
        ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/http_functional_tests

)

SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")

configure_file("${PROJECT_SOURCE_DIR}/test/streams/clear1.ts" "${CMAKE_BINARY_DIR}/test/clear1.ts" COPYONLY)
configure_file("${PROJECT_SOURCE_DIR}/test/streams/clear2.ts" "${CMAKE_BINARY_DIR}/test/clear2.ts" COPYONLY)
configure_file("${PROJECT_SOURCE_DIR}/test/streams/encrypted1.ts" "${CMAKE_BINARY_DIR}/test/encrypted1.ts" COPYONLY)
configure_file("${PROJECT_SOURCE_DIR}/test/streams/encrypted2.ts" "${CMAKE_BINARY_DIR}/test/encrypted2.ts" COPYONLY)
