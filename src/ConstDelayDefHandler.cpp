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


#include <chrono>
#include "utils/ConstDelayDefHandler.h"

using namespace std::chrono;

ReadDefferHandler::DefferSignalTypes ConstDelayDefHandler::waitSignal() {

    std::mutex mtx;
    std::unique_lock<std::mutex> lck(mtx);

    auto currentTimeMs = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
    );

    if (currentTimeMs - mLastReadMs < mWaitTimeMs) {
        if (!mSignalQ.empty()) {
            auto res = mSignalQ.front();
            mSignalQ.pop();
            return res;
        } else {
            return DATA_RECEIVED;
        }
    }

    while (mCv.wait_for(lck, mWaitTimeMs) == std::cv_status::timeout) {
        return GENERIC_FAILURE;
    }
    if (!mSignalQ.empty()) {
        auto res = mSignalQ.front();
        mSignalQ.pop();
        return res;
    }

    return DATA_RECEIVED;
}

void ConstDelayDefHandler::signal(ReadDefferHandler::DefferSignalTypes signal) {


    mLastReadMs = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
    );

    if (signal != DATA_RECEIVED && mSignalQ.size() < MAX_ERROR_COUNT) {
        mSignalQ.push(signal);

    }
    mCv.notify_all();
}

ConstDelayDefHandler::ConstDelayDefHandler() {
}
