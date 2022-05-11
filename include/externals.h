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

#include <cstdint>

#include "config_fcc.h"
#include <glog/logging.h>
#include <mutex>
#include <stddef.h>     /* offsetof */

extern std::mutex mBqAllocator;
extern char NOKIA_BUFFER_MAGIC[4];

extern "C" {

struct bq_buffer {
    char magic[4] = {0, 0, 0, 0};     // magic for buffer identification
    void *context     = nullptr;      // context
    char* channelInfo = nullptr;      // Channel info
    uint64_t id;                      // buffer id
    uint32_t size;                    // buffer size (may be adjusted by producer)
    uint32_t capacity;                // maximum buffer size
    int8_t buffer[FEIP_BUFFER_SIZE];   // pointer to buffer

    ~bq_buffer() {
        free(channelInfo);
    }
} __attribute__((packed));


bq_buffer *fcc_buffer_to_bq_buffer(int8_t *buffer);

/**
 * Allocate BQ buffer of a given size
 * @param size - buffer size
 * @param context - buffer context. Must be not nullptr
 * @return - buffer on success, nullptr on failure
 */
bq_buffer *alloc_bq_buffer(size_t size, void *context);

/**
 * Free BQ buffer
 * @param buffer
 */
void free_bq_buffer(bq_buffer *buffer);

}
