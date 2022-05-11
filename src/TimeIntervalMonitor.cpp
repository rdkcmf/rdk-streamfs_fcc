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

#include "utils/TimeIntervalMonitor.h"

TimeIntervalMonitor::TimeIntervalMonitor() {
    reset();
}

void TimeIntervalMonitor::reset() {
    std::lock_guard<std::mutex> mLock(mMutex);
    mRunning = false;
    mInitTime = std::chrono::high_resolution_clock::now();
    mLastTime = mInitTime;
    mDeltaTime = (std::chrono::duration_cast<std::chrono::microseconds>(mLastTime-mInitTime));
}
void TimeIntervalMonitor::stopTimeInterval() {
    std::lock_guard<std::mutex> mLock(mMutex);
    mRunning = false;
}

void TimeIntervalMonitor::updateTimeInterval() {
    std::lock_guard<std::mutex> mLock(mMutex);
    if (!mRunning) {
        mDeltaTime += (std::chrono::duration_cast<std::chrono::microseconds>(mLastTime-mInitTime));
        mInitTime = std::chrono::high_resolution_clock::now();
        mLastTime = mInitTime;
        mRunning = true;
    } else {
        mLastTime = std::chrono::high_resolution_clock::now();
    }
}

uint64_t TimeIntervalMonitor::getAccumulatedTimeInMicroSeconds() {
    std::lock_guard<std::mutex> mLock(mMutex);
    return mDeltaTime.count() + (std::chrono::duration_cast<std::chrono::microseconds>(mLastTime-mInitTime)).count();
}
