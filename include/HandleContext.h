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

// Individual context for file descriptor handles

class HandleContext {

public:
    uint64_t getCurrentReadOffset() const { return mReadOffset;}

    void incReadOffset(uint64_t increment) {
        mReadOffset += increment;
    }

    void initReadOffset(uint64_t readOffset) {
        mReadOffset = readOffset;
    }

private:

    uint64_t mReadOffset = 0;
};
