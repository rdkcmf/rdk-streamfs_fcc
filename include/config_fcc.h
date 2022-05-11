/*
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright (c) 2022 Nuuday.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <boost/predef.h>
#include <vector>
#include <string>
#include "tracing.h"

// Uncomment for enabling input buffer monitor file
// #define DEBUG_INPUT_STREAM

// TS file footer area
// blank area in case the player tries to read the file
// footer
#define TS_FILE_FOOTER_BYTES 1024 * 1024 * 1000

// Seek max time
#define SEEK_BUFFER_TIME_S 60 * 60

#ifdef DEBUG_INPUT_STREAM
#define INPUT_STREAM_DUMP_FILE "/tmp/input_stream_dump_"
#endif

// Time after a read is set to failure.
#define CHANNEL_READ_TIMEOUT_MS 1000

#define FEIP_UDP_SOCKET_PORT 37482
#define MAXLINE 1024
/**
 * TODO: Single demux queue for the moment
 */
#define FEIP 1 // FEIP id
#define FEIP_NEEDS_UNICAST_SESSION 1
#define FEIP_DEFAULT_BUFFER_COUNT  64

/**
 * QUIRK_FORCE_VBO_VM_RET_ADDRESS
 * Set to 1 to force FCC initial config
 * set to FEIP_FCC_CONFIG_STRING
 */
#define QUIRK_FORCE_VBO_VM_RET_ADDRESS 0

/**
 * Buffer size.  Nokia advices to allign the buffers
 * to 7 * 188 size
 */
#define FEIP_BUFFER_SIZE  32 * 7 * 188 //~32 kbytes buffer size
#define DEMUX_COUNT 1


#if defined(BOOST_ARCH_ARM)
#define ETHERNET_INTERFACE "eth0"
#ifndef ARCH_ARM
#define ARCH_ARM
#endif
#elif defined(BOOST_ARCH_X86_64 )
#ifndef ARCH_X86_64
#define ARCH_X86_64
#endif
#define FCC_FORCE_ETHERNET_INTERFACE
#define ETHERNET_INTERFACE "enxec086b1c203d"
#elif defined(BOOST_ARCH_X86_32)
#ifndef ARCH_X86_32
#define ARCH_X86_32
#endif
#define FCC_FORCE_ETHERNET_INTERFACE
#define ETHERNET_INTERFACE "enxec086b1c203d"
#else
#error("Architecture not supported")
#endif

#define SOCKET_MAXLINE 10240
#define DEFAULT_SOCKET_PORT 8433

#define UNUSED(expr) do { (void)(expr); } while (0)

#define CLASS_NO_COPY_OR_ASSIGN(A) \
  A(const A&) = delete;   \
  void operator=(const A&) = delete

#if defined(_MSC_VER)
#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
#define EXPORT __attribute__((visibility("default")))
#define IMPORT
#else
#define EXPORT
#define IMPORT
#pragma warning Unknown dynamic link import/export semantics.
#endif

typedef std::vector<unsigned char> ByteVectorType ;

static const  std::string TRUE_STR = "1";
static const  std::string FALSE_STR = "0";

#define BYTE_VECTOR_TRUE  ByteVectorType(TRUE_STR.begin(), TRUE_STR.end())
#define BYTE_VECTOR_FALSE  ByteVectorType(FALSE_STR.begin(), FALSE_STR.end())

