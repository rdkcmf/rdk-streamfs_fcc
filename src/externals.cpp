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



#include "externals.h"

extern "C" {

/**
 * Allocate BQ buffer of a given size
 * @param size - buffer size
 * @param context - buffer context. Must be not nullptr
 * @return - buffer on success, nullptr on failure
 */
bq_buffer *alloc_bq_buffer(size_t size, void *context) {
    std::lock_guard<std::mutex> lockGuard(mBqAllocator);

    static uint64_t counter = 0;
    counter++;
    auto result = (bq_buffer *) malloc(sizeof(struct bq_buffer));

    if (result == nullptr) {
        LOG(ERROR) << "Failed to allocate bq_buffer";
        return nullptr;
    }

    result->context = context;
    result->id = counter;
    strncpy(result->magic, NOKIA_BUFFER_MAGIC, 4);
    result->size = size;
    result->capacity = size;

    return result;
}

void free_bq_buffer(bq_buffer *buffer) {
    if (buffer == nullptr) {
        LOG(WARNING) << "Can't free null buffer";
    }

    free(buffer);
}

bq_buffer *fcc_buffer_to_bq_buffer(int8_t *buffer) {
    if (buffer == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<bq_buffer *>(
            reinterpret_cast<char *>(buffer) - offsetof(bq_buffer, buffer)
    );
}

}
