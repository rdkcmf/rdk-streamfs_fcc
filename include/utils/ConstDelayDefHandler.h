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

#include <condition_variable>
#include <chrono>
#include <queue>
#include "ReadDefferHandler.h"


using namespace std::chrono;

class ConstDelayDefHandler : public ReadDefferHandler {
public:
    explicit ConstDelayDefHandler(milliseconds timeMs) : mWaitTimeMs(timeMs) {

    }

    DefferSignalTypes waitSignal() override;

    void signal(DefferSignalTypes signal) override;

    ConstDelayDefHandler(const ConstDelayDefHandler &) = delete;

private:
    ConstDelayDefHandler();

    std::condition_variable mCv;
    milliseconds mWaitTimeMs;
    std::queue<ReadDefferHandler::DefferSignalTypes> mSignalQ;
    static const int MAX_ERROR_COUNT = 10;
    milliseconds mLastReadMs = milliseconds(0);
};
